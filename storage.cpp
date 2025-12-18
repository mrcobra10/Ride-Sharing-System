#include "storage.h"

#include "roads.h"
#include "ride.h"
#include "user.h"

#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

using namespace std;

// -------------------------
// MOVED FROM roads.cpp
// -------------------------
bool loadRoadNetworkFromFile(fstream &roadFile)
{
    string line;
    while (getline(roadFile, line))
    {
        if (line.empty()) continue;
        stringstream ss(line);
        char from_place[256];
        char to_place[256];
        int cost;
        if (!(ss >> from_place >> to_place >> cost))
            continue;
        AddRoad(from_place, to_place, cost);
    }
    return true;
}

// -------------------------
// Helpers: Users + History
// -------------------------
static void SaveHistoryInOrder(ofstream& out, HistoryNode* root, int userId)
{
    if (!root) return;
    SaveHistoryInOrder(out, root->left, userId);
    out << userId << ' ' << root->rideId << ' '
        << root->from << ' ' << root->to << ' '
        << root->time << '\n';
    SaveHistoryInOrder(out, root->right, userId);
}

static void SaveUsersInOrder(ofstream& out, User* root)
{
    if (!root) return;
    SaveUsersInOrder(out, root->left);
    out << root->userId << ' ' << root->name << ' '
        << root->isDriver << ' ' << root->rating << '\n';
    SaveUsersInOrder(out, root->right);
}

static int CountUsers(User* root)
{
    if (!root) return 0;
    return CountUsers(root->left) + 1 + CountUsers(root->right);
}

static int CountHistoryNodes(HistoryNode* root)
{
    if (!root) return 0;
    return CountHistoryNodes(root->left) + 1 + CountHistoryNodes(root->right);
}

static int CountAllHistory(User* root)
{
    if (!root) return 0;
    return CountAllHistory(root->left) + CountHistoryNodes(root->history) + CountAllHistory(root->right);
}

static void SaveAllHistory(ofstream& out, User* root)
{
    if (!root) return;
    SaveAllHistory(out, root->left);
    SaveHistoryInOrder(out, root->history, root->userId);
    SaveAllHistory(out, root->right);
}

// -------------------------
// Helpers: Roads
// -------------------------
static int CountRoadEdges()
{
    int c = 0;
    for (Place* p = placeHead; p; p = p->next)
        for (RoadLink* e = p->firstLink; e; e = e->next)
            c++;
    return c;
}

static void SaveRoads(ofstream& out)
{
    for (Place* p = placeHead; p; p = p->next)
        for (RoadLink* e = p->firstLink; e; e = e->next)
            out << p->name << ' ' << e->to->name << ' ' << e->cost << '\n';
}

// -------------------------
// Helpers: Offers
// -------------------------
static int CountOffers()
{
    int c = 0;
    for (RideOffer* o = offerHead; o; o = o->next) c++;
    return c;
}

static void SaveOffers(ofstream& out)
{
    for (RideOffer* o = offerHead; o; o = o->next)
    {
        out << o->offerId << ' ' << o->driverId << ' '
            << (o->startPlace ? o->startPlace->name : "NULL") << ' '
            << (o->endPlace ? o->endPlace->name : "NULL") << ' '
            << o->departTime << ' ' << o->capacity << ' ' << o->seatsLeft << '\n';
    }
}

// -------------------------
// Helpers: Active rides
// -------------------------
static int CountActiveRides()
{
    int total = 0;
    for (int i = 0; i < ActiveRideTableSize(); i++)
    {
        for (ActiveRide* ar = ActiveRideBucketHead(i); ar; ar = ar->next)
            total++;
    }
    return total;
}

static int CountPassengers(PassengerNode* p)
{
    int c = 0;
    while (p) { c++; p = p->next; }
    return c;
}

static void SaveActiveRides(ofstream& out)
{
    for (int i = 0; i < ActiveRideTableSize(); i++)
    {
        for (ActiveRide* ar = ActiveRideBucketHead(i); ar; ar = ar->next)
        {
            int offerId = (ar->offer ? ar->offer->offerId : -1);
            int pc = CountPassengers(ar->passengers);
            out << ar->rideId << ' ' << offerId << ' ' << pc;
            for (PassengerNode* p = ar->passengers; p; p = p->next)
                out << ' ' << p->passengerId;
            out << '\n';
        }
    }
}

// -------------------------
// Public API
// -------------------------
static string JoinPath(const char* baseDir, const char* file)
{
    string b = baseDir ? baseDir : ".";
    if (!b.empty() && b.back() != '/')
        b += '/';
    return b + file;
}

bool SaveAll(const char* baseDir)
{
    // 10.1 Save — in required order

    // Users → BST traversal
    {
        ofstream out(JoinPath(baseDir, "users.dat"));
        if (!out) return false;
        out << CountUsers(userRoot) << '\n';
        SaveUsersInOrder(out, userRoot);
    }

    // Places & roads → list traversal
    {
        ofstream out(JoinPath(baseDir, "roads.dat"));
        if (!out) return false;
        out << CountRoadEdges() << '\n';
        SaveRoads(out);
    }

    // Ride offers → linked list
    {
        ofstream out(JoinPath(baseDir, "offers.dat"));
        if (!out) return false;
        out << CountOffers() << '\n';
        SaveOffers(out);
    }

    // Active rides → hash table
    {
        ofstream out(JoinPath(baseDir, "active_rides.dat"));
        if (!out) return false;
        out << CountActiveRides() << '\n';
        SaveActiveRides(out);
    }

    // History → per-user lists/BST
    {
        ofstream out(JoinPath(baseDir, "history.dat"));
        if (!out) return false;
        out << CountAllHistory(userRoot) << '\n';
        SaveAllHistory(out, userRoot);
    }

    return true;
}

bool LoadAll(const char* baseDir)
{
    // 10.2 Load — read in same order; rebuild structures

    // Users
    {
        ifstream in(JoinPath(baseDir, "users.dat"));
        if (!in) return false;
        int n = 0;
        in >> n;
        for (int i = 0; i < n; i++)
        {
            int id, isDriver, rating;
            string name;
            in >> id >> name >> isDriver >> rating;
            userRoot = CreateUser(userRoot, id, name.c_str(), isDriver);
            User* u = SearchUser(userRoot, id);
            if (u) u->rating = rating;
        }
    }

    // Roads
    {
        ifstream in(JoinPath(baseDir, "roads.dat"));
        if (!in) return false;
        int m = 0;
        in >> m;
        for (int i = 0; i < m; i++)
        {
            string from, to;
            int cost;
            in >> from >> to >> cost;
            AddRoad(from.c_str(), to.c_str(), cost);
        }
    }

    // Offers
    {
        ifstream in(JoinPath(baseDir, "offers.dat"));
        if (!in) return false;
        int n = 0;
        in >> n;
        for (int i = 0; i < n; i++)
        {
            int offerId, driverId, departTime, capacity, seatsLeft;
            string start, end;
            in >> offerId >> driverId >> start >> end >> departTime >> capacity >> seatsLeft;
            RideOffer* o = CreateRideOffer(offerId, driverId, start.c_str(), end.c_str(), departTime, capacity);
            if (o) o->seatsLeft = seatsLeft;
        }
    }

    // Active rides
    {
        ClearActiveRides();
        ifstream in(JoinPath(baseDir, "active_rides.dat"));
        if (!in) return false;
        int n = 0;
        in >> n;
        for (int i = 0; i < n; i++)
        {
            int rideId, offerId, pc;
            in >> rideId >> offerId >> pc;
            int* passengers = (pc > 0) ? new int[pc] : nullptr;
            for (int j = 0; j < pc; j++)
                in >> passengers[j];
            StorageInsertActiveRide(rideId, offerId, passengers, pc);
            delete[] passengers;
        }
    }

    // History
    {
        ifstream in(JoinPath(baseDir, "history.dat"));
        if (!in) return false;
        int n = 0;
        in >> n;
        for (int i = 0; i < n; i++)
        {
            int userId, rideId, time;
            string from, to;
            in >> userId >> rideId >> from >> to >> time;
            AddHistory(userId, rideId, from.c_str(), to.c_str(), time);
        }
    }

    return true;
}


