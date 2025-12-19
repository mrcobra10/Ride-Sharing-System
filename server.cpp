#include "httplib.h"

#include "roads.h"
#include "ride.h"
#include "storage.h"
#include "user.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// --------- GLOBALS (same as main.cpp demo) ----------
Place *placeHead = nullptr;
User *userRoot = nullptr;
RideOffer *offerHead = nullptr;
RideRequest *requestHead = nullptr;
int requestCount = 0;

// ---------------- JSON helpers (minimal, for our limited shapes) ----------------
static string JsonEscape(const string &s)
{
    string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

static void AddCors(httplib::Response &res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static bool ParseIntField(const string &body, const string &key, int &out)
{
    // very small JSON parser: expects "key":123
    string pat = "\"" + key + "\"";
    size_t p = body.find(pat);
    if (p == string::npos) return false;
    p = body.find(':', p);
    if (p == string::npos) return false;
    p++;
    while (p < body.size() && (body[p] == ' ' || body[p] == '\n' || body[p] == '\r' || body[p] == '\t')) p++;
    size_t end = p;
    if (end < body.size() && body[end] == '-') end++;
    while (end < body.size() && isdigit((unsigned char)body[end])) end++;
    if (end == p) return false;
    out = stoi(body.substr(p, end - p));
    return true;
}

static bool ParseStringField(const string &body, const string &key, string &out)
{
    // expects "key":"value" (no escaped quotes inside value; good enough for our UI tokens)
    string pat = "\"" + key + "\"";
    size_t p = body.find(pat);
    if (p == string::npos) return false;
    p = body.find(':', p);
    if (p == string::npos) return false;
    p = body.find('"', p);
    if (p == string::npos) return false;
    size_t start = p + 1;
    size_t end = body.find('"', start);
    if (end == string::npos) return false;
    out = body.substr(start, end - start);
    return true;
}

// ---------------- Places: coordinates support ----------------
struct LatLng
{
    double lat;
    double lng;
};

static unordered_map<string, LatLng> placeCoords;

static bool LoadPlaceCoordsCSV(const string &path)
{
    ifstream in(path);
    if (!in) return false;
    string line;
    while (getline(in, line))
    {
        if (line.empty()) continue;
        if (line.rfind("#", 0) == 0) continue;
        stringstream ss(line);
        string name, latStr, lngStr;
        if (!getline(ss, name, ',')) continue;
        if (!getline(ss, latStr, ',')) continue;
        if (!getline(ss, lngStr, ',')) continue;
        LatLng ll;
        ll.lat = stod(latStr);
        ll.lng = stod(lngStr);
        placeCoords[name] = ll;
    }
    return true;
}

static LatLng FallbackCoordFor(const string &name)
{
    // Deterministic pseudo-coordinates (roughly around Islamabad) so map works without places.csv.
    // This is purely for demo; provide places.csv for real placement.
    unsigned long h = 5381;
    for (char c : name) h = ((h << 5) + h) + (unsigned char)c;
    double lat = 33.6844 + ((h % 1000) / 1000.0) * 0.08;      // ~33.6844 .. +0.08
    double lng = 73.0479 + (((h / 1000) % 1000) / 1000.0) * 0.10; // ~73.0479 .. +0.10
    return {lat, lng};
}

static LatLng CoordForPlaceName(const string &name)
{
    auto it = placeCoords.find(name);
    if (it != placeCoords.end()) return it->second;
    return FallbackCoordFor(name);
}

static int EdgeCost(Place *a, Place *b)
{
    if (!a || !b) return -1;
    for (RoadLink *e = a->firstLink; e; e = e->next)
        if (e->to == b) return e->cost;
    return -1;
}

int main()
{
    // Optional: load initial roads from roads.txt if present
    {
        fstream f("roads.txt");
        if (f.is_open())
            loadRoadNetworkFromFile(f);
    }

    // Optional coordinates file (recommended)
    LoadPlaceCoordsCSV("places.csv");

    httplib::Server srv;

    // Preflight CORS
    srv.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                {
                    AddCors(res);
                    res.status = 204;
                });

    // Health
    srv.Get("/api/health", [](const httplib::Request &, httplib::Response &res)
            {
                AddCors(res);
                res.set_content("{\"ok\":true}", "application/json");
            });

    // Graph: places + roads
    srv.Get("/api/graph", [](const httplib::Request &, httplib::Response &res)
            {
                AddCors(res);

                string json = "{\"places\":[";
                bool firstP = true;
                for (Place *p = placeHead; p; p = p->next)
                {
                    if (!firstP) json += ",";
                    firstP = false;
                    string name = p->name ? p->name : "";
                    LatLng ll = CoordForPlaceName(name);
                    json += "{\"name\":\"" + JsonEscape(name) + "\",\"lat\":" + to_string(ll.lat) + ",\"lng\":" + to_string(ll.lng) + "}";
                }
                json += "],\"roads\":[";

                bool firstR = true;
                for (Place *p = placeHead; p; p = p->next)
                {
                    for (RoadLink *e = p->firstLink; e; e = e->next)
                    {
                        if (!firstR) json += ",";
                        firstR = false;
                        string from = p->name ? p->name : "";
                        string to = (e->to && e->to->name) ? e->to->name : "";
                        json += "{\"from\":\"" + JsonEscape(from) + "\",\"to\":\"" + JsonEscape(to) + "\",\"cost\":" + to_string(e->cost) + "}";
                    }
                }
                json += "]}";

                res.set_content(json, "application/json");
            });

    // Register user
    srv.Post("/api/users/register", [](const httplib::Request &req, httplib::Response &res)
             {
                 AddCors(res);
                 int userId = 0;
                 string name, role;
                 if (!ParseIntField(req.body, "userId", userId) ||
                     !ParseStringField(req.body, "name", name) ||
                     !ParseStringField(req.body, "role", role))
                 {
                     res.status = 400;
                     res.set_content("{\"error\":\"invalid_body\"}", "application/json");
                     return;
                 }
                 int isDriver = (role == "driver") ? 1 : 0;
                 userRoot = CreateUser(userRoot, userId, name.c_str(), isDriver);
                 res.set_content("{\"ok\":true}", "application/json");
             });

    // Top-K drivers
    srv.Get("/api/users/top", [](const httplib::Request &req, httplib::Response &res)
            {
                AddCors(res);
                int k = 10;
                if (req.has_param("k")) k = stoi(req.get_param_value("k"));

                // Reuse PrintTopDrivers for console, but for UI we return JSON summary.
                // Collect drivers locally here (same logic as user.cpp but JSON-shaped).
                vector<User *> drivers;
                function<void(User *)> collect = [&](User *r)
                {
                    if (!r) return;
                    collect(r->left);
                    if (r->isDriver == 1) drivers.push_back(r);
                    collect(r->right);
                };
                collect(userRoot);

                sort(drivers.begin(), drivers.end(), [](const User *a, const User *b)
                     {
                         if (a->completedRides != b->completedRides) return a->completedRides > b->completedRides;
                         if (a->rating != b->rating) return a->rating > b->rating;
                         return a->userId < b->userId;
                     });

                if (k < 0) k = 0;
                if (k > (int)drivers.size()) k = (int)drivers.size();

                string json = "{\"drivers\":[";
                for (int i = 0; i < k; i++)
                {
                    if (i) json += ",";
                    User *u = drivers[i];
                    json += "{\"userId\":" + to_string(u->userId) +
                            ",\"name\":\"" + JsonEscape(u->name ? u->name : "") + "\"" +
                            ",\"rating\":" + to_string(u->rating) +
                            ",\"completedRides\":" + to_string(u->completedRides) +
                            "}";
                }
                json += "]}";
                res.set_content(json, "application/json");
            });

    // Create offer
    srv.Post("/api/offers", [](const httplib::Request &req, httplib::Response &res)
             {
                 AddCors(res);
                 int offerId, driverId, departTime, capacity;
                 string start, end;
                 if (!ParseIntField(req.body, "offerId", offerId) ||
                     !ParseIntField(req.body, "driverId", driverId) ||
                     !ParseStringField(req.body, "start", start) ||
                     !ParseStringField(req.body, "end", end) ||
                     !ParseIntField(req.body, "departTime", departTime) ||
                     !ParseIntField(req.body, "capacity", capacity))
                 {
                     res.status = 400;
                     res.set_content("{\"error\":\"invalid_body\"}", "application/json");
                     return;
                 }
                 RideOffer *o = CreateRideOffer(offerId, driverId, start.c_str(), end.c_str(), departTime, capacity);
                 if (!o)
                 {
                     res.status = 400;
                     res.set_content("{\"error\":\"offer_failed\"}", "application/json");
                     return;
                 }
                 res.set_content("{\"ok\":true}", "application/json");
             });

    // Create request
    srv.Post("/api/requests", [](const httplib::Request &req, httplib::Response &res)
             {
                 AddCors(res);
                 int requestId, passengerId, earliest, latest;
                 string from, to;
                 if (!ParseIntField(req.body, "requestId", requestId) ||
                     !ParseIntField(req.body, "passengerId", passengerId) ||
                     !ParseStringField(req.body, "from", from) ||
                     !ParseStringField(req.body, "to", to) ||
                     !ParseIntField(req.body, "earliest", earliest) ||
                     !ParseIntField(req.body, "latest", latest))
                 {
                     res.status = 400;
                     res.set_content("{\"error\":\"invalid_body\"}", "application/json");
                     return;
                 }
                 RideRequest *r = CreateRideRequest(requestId, passengerId, from.c_str(), to.c_str(), earliest, latest);
                 if (!r)
                 {
                     res.status = 400;
                     res.set_content("{\"error\":\"request_failed\"}", "application/json");
                     return;
                 }
                 res.set_content("{\"ok\":true}", "application/json");
             });

    // List all offers
    srv.Get("/api/offers", [](const httplib::Request &, httplib::Response &res)
            {
                AddCors(res);
                string json = "{\"offers\":[";
                bool first = true;
                for (RideOffer *o = offerHead; o; o = o->next)
                {
                    if (!first) json += ",";
                    first = false;
                    json += "{\"offerId\":" + to_string(o->offerId) +
                            ",\"driverId\":" + to_string(o->driverId) +
                            ",\"start\":\"" + JsonEscape(o->startPlace && o->startPlace->name ? o->startPlace->name : "") + "\"" +
                            ",\"end\":\"" + JsonEscape(o->endPlace && o->endPlace->name ? o->endPlace->name : "") + "\"" +
                            ",\"departTime\":" + to_string(o->departTime) +
                            ",\"capacity\":" + to_string(o->capacity) +
                            ",\"seatsLeft\":" + to_string(o->seatsLeft) +
                            "}";
                }
                json += "]}";
                res.set_content(json, "application/json");
            });

    // Reachable areas within cost bound
    srv.Get("/api/reachable", [](const httplib::Request &req, httplib::Response &res)
            {
                AddCors(res);
                if (!req.has_param("offerId") || !req.has_param("costBound"))
                {
                    res.status = 400;
                    res.set_content("{\"error\":\"missing_params\"}", "application/json");
                    return;
                }
                int offerId = stoi(req.get_param_value("offerId"));
                int costBound = stoi(req.get_param_value("costBound"));

                RideOffer *offer = nullptr;
                for (RideOffer *o = offerHead; o; o = o->next)
                {
                    if (o->offerId == offerId)
                    {
                        offer = o;
                        break;
                    }
                }
                if (!offer || !offer->startPlace)
                {
                    res.status = 404;
                    res.set_content("{\"error\":\"offer_not_found\"}", "application/json");
                    return;
                }

                // Reuse PrintReachableWithinCost logic but return JSON
                struct DistEntry
                {
                    Place *place;
                    int dist;
                };
                DistEntry dist[100];
                int distCount = 0;

                auto getIndex = [&](Place *p)
                {
                    for (int i = 0; i < distCount; i++)
                        if (dist[i].place == p)
                            return i;
                    dist[distCount].place = p;
                    dist[distCount].dist = INT_MAX;
                    return distCount++;
                };

                // Simple min-pq (array-based)
                Place *pq[100];
                int pqDist[100];
                int pqSize = 0;

                auto pqPush = [&](Place *p, int d)
                {
                    int i = pqSize++;
                    pq[i] = p;
                    pqDist[i] = d;
                    while (i > 0 && pqDist[(i - 1) / 2] > pqDist[i])
                    {
                        swap(pq[i], pq[(i - 1) / 2]);
                        swap(pqDist[i], pqDist[(i - 1) / 2]);
                        i = (i - 1) / 2;
                    }
                };

                auto pqPop = [&](Place *&p, int &d)
                {
                    p = pq[0];
                    d = pqDist[0];
                    pq[0] = pq[--pqSize];
                    pqDist[0] = pqDist[pqSize];
                    int i = 0;
                    while (true)
                    {
                        int l = 2 * i + 1, r = 2 * i + 2, s = i;
                        if (l < pqSize && pqDist[l] < pqDist[s]) s = l;
                        if (r < pqSize && pqDist[r] < pqDist[s]) s = r;
                        if (s == i) break;
                        swap(pq[i], pq[s]);
                        swap(pqDist[i], pqDist[s]);
                        i = s;
                    }
                };

                int startIdx = getIndex(offer->startPlace);
                dist[startIdx].dist = 0;
                pqPush(offer->startPlace, 0);

                vector<pair<string, int>> reachable;

                while (pqSize > 0)
                {
                    Place *u;
                    int d_u;
                    pqPop(u, d_u);

                    if (d_u > costBound)
                        break;

                    if (u && u->name)
                        reachable.push_back({u->name, d_u});

                    RoadLink *edge = u->firstLink;
                    while (edge)
                    {
                        Place *v = edge->to;
                        int newDist = d_u + edge->cost;

                        if (newDist <= costBound)
                        {
                            int vIdx = getIndex(v);
                            if (newDist < dist[vIdx].dist)
                            {
                                dist[vIdx].dist = newDist;
                                pqPush(v, newDist);
                            }
                        }
                        edge = edge->next;
                    }
                }

                string json = "{\"reachable\":[";
                for (size_t i = 0; i < reachable.size(); i++)
                {
                    if (i) json += ",";
                    json += "{\"place\":\"" + JsonEscape(reachable[i].first) + "\",\"cost\":" + to_string(reachable[i].second) + "}";
                }
                json += "]}";
                res.set_content(json, "application/json");
            });

    // Match next - returns match details if successful
    srv.Post("/api/match/next", [](const httplib::Request &, httplib::Response &res)
             {
                 AddCors(res);
                 
                 // Record seatsLeft for all offers before matching
                 unordered_map<int, int> seatsBefore;
                 for (RideOffer* o = offerHead; o; o = o->next)
                 {
                     seatsBefore[o->offerId] = o->seatsLeft;
                 }
                 
                 // Execute the match
                 int matched = MatchNextRequest();
                 
                 if (matched)
                 {
                     // Find which offer's seatsLeft decreased (that's the matched offer)
                     RideOffer* matchedOffer = nullptr;
                     for (RideOffer* o = offerHead; o; o = o->next)
                     {
                         auto it = seatsBefore.find(o->offerId);
                         if (it != seatsBefore.end() && o->seatsLeft < it->second)
                         {
                             matchedOffer = o;
                             break;
                         }
                     }
                     
                     if (matchedOffer)
                     {
                         User* driver = SearchUser(userRoot, matchedOffer->driverId);
                         string driverName = driver && driver->name ? driver->name : "";
                         
                         string startName = matchedOffer->startPlace && matchedOffer->startPlace->name ? matchedOffer->startPlace->name : "";
                         string endName = matchedOffer->endPlace && matchedOffer->endPlace->name ? matchedOffer->endPlace->name : "";
                         
                         string json = "{\"matched\":true,\"driverName\":\"" + JsonEscape(driverName) +
                                      "\",\"driverId\":" + to_string(matchedOffer->driverId) +
                                      ",\"offerId\":" + to_string(matchedOffer->offerId) +
                                      ",\"start\":\"" + JsonEscape(startName) +
                                      "\",\"end\":\"" + JsonEscape(endName) +
                                      "\",\"departTime\":" + to_string(matchedOffer->departTime) + "}";
                         res.set_content(json, "application/json");
                         return;
                     }
                 }
                 
                 res.set_content("{\"matched\":false}", "application/json");
             });

    // Route (shortest path)
    srv.Get("/api/route", [](const httplib::Request &req, httplib::Response &res)
            {
                AddCors(res);
                if (!req.has_param("from") || !req.has_param("to"))
                {
                    res.status = 400;
                    res.set_content("{\"error\":\"missing_params\"}", "application/json");
                    return;
                }
                string from = req.get_param_value("from");
                string to = req.get_param_value("to");

                Place *start = GetOrCreatePlace(from.c_str());
                Place *end = GetOrCreatePlace(to.c_str());

                Place *path[200];
                int pathLen = 0;
                bool ok = ComputeShortestPath(start, end, path, pathLen);
                if (!ok || pathLen <= 0)
                {
                    res.status = 404;
                    res.set_content("{\"error\":\"no_path\"}", "application/json");
                    return;
                }

                int totalCost = 0;
                for (int i = 0; i + 1 < pathLen; i++)
                {
                    int c = EdgeCost(path[i], path[i + 1]);
                    if (c < 0) { totalCost = -1; break; }
                    totalCost += c;
                }

                string json = "{\"path\":[";
                for (int i = 0; i < pathLen; i++)
                {
                    if (i) json += ",";
                    string nm = (path[i] && path[i]->name) ? path[i]->name : "";
                    json += "\"" + JsonEscape(nm) + "\"";
                }
                json += "],\"totalCost\":" + to_string(totalCost) + "}";
                res.set_content(json, "application/json");
            });

    // Storage save/load
    srv.Post("/api/storage/save", [](const httplib::Request &, httplib::Response &res)
             {
                 AddCors(res);
                 bool ok = SaveAll(".");
                 res.set_content(string("{\"ok\":") + (ok ? "true" : "false") + "}", "application/json");
             });

    srv.Post("/api/storage/load", [](const httplib::Request &, httplib::Response &res)
             {
                 AddCors(res);
                 // reset heads; demo backend keeps process alive
                 placeHead = nullptr;
                 userRoot = nullptr;
                 offerHead = nullptr;
                 requestHead = nullptr;
                 requestCount = 0;
                 ClearActiveRides();

                 bool ok = LoadAll(".");
                 res.set_content(string("{\"ok\":") + (ok ? "true" : "false") + "}", "application/json");
             });

    printf("C++ API listening on http://0.0.0.0:8080 (accessible from network)\\n");
    srv.listen("0.0.0.0", 8080);
    return 0;
}


