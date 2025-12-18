// ============================
#include "ride.h"
#include <climits>
// ============================


#include <iostream>
#include <cstring>
using namespace std;

#define MAX_REQUESTS 1000

RideRequest *requestHeap[MAX_REQUESTS];

// Forward declaration


// ---------------- RIDE OFFER ----------------


// ---------------- RIDE REQUEST ----------------


// Global heads (defined in ride.cpp)

// ----------- REQUIRED FUNCTIONS -----------


int MatchNextRequest();

// Utility / placeholders
void PrintOffers();
void PrintRequests();

// Provided by graph / place module
Place* GetOrCreatePlace(const char *name);



// ============================
// ride.cpp
// ============================


// ---------------- GLOBAL HEADS ----------------


// ---------------- CREATE RIDE OFFER ----------------
RideOffer* CreateRideOffer(int offerId, int driverId,
                           const char *start, const char *end,
                           int departTime, int capacity)
{
    RideOffer *o = new RideOffer;

    o->offerId    = offerId;
    o->driverId   = driverId;
    o->startPlace = GetOrCreatePlace(start);
    o->endPlace   = GetOrCreatePlace(end);
    o->departTime = departTime;
    o->capacity   = capacity;
    o->seatsLeft  = capacity;

    // Insert at head of linked list
    o->next = offerHead;
    offerHead = o;

    return o;
}

void swapRequests(int i, int j)
{
    RideRequest *temp = requestHeap[i];
    requestHeap[i] = requestHeap[j];
    requestHeap[j] = temp;

    requestHeap[i]->heapIndex = i;
    requestHeap[j]->heapIndex = j;
}

void heapifyUp(int index)
{
    while (index > 0)
    {
        int parent = (index - 1) / 2;

        if (requestHeap[parent]->earliest <= requestHeap[index]->earliest)
            break;

        swapRequests(parent, index);
        index = parent;
    }
}


RideRequest* CreateRideRequest(int requestId, int passengerId,
                               const char *from, const char *to,
                               int earliest, int latest)
{
    // Step 4.3.1 — Validate passenger
    if (!PassengerExists(passengerId))
    {
        cout << "Error: Passenger does not exist.\n";
        return NULL;
    }

    // Step 4.3.2 — Convert place names to Place*
    Place *fromPlace = GetOrCreatePlace(from);
    Place *toPlace   = GetOrCreatePlace(to);

    // Step 4.3.3 — Create request object
    RideRequest *r = new RideRequest;
    r->requestId   = requestId;
    r->passengerId = passengerId;
    r->fromPlace   = fromPlace;
    r->toPlace     = toPlace;
    r->earliest    = earliest;
    r->latest      = latest;

    // Step 4.2 — Insert into min-heap (priority queue)
    if (requestCount >= MAX_REQUESTS)
    {
        cout << "Error: Request heap full.\n";
        delete r;
        return NULL;
    }

    int index = requestCount;
    requestHeap[index] = r;
    r->heapIndex = index;
    requestCount++;

    heapifyUp(index);

    // requestHead always points to heap root
    requestHead = requestHeap[0];

    return r;
}


// ---------------- MATCH NEXT REQUEST ----------------
/*int MatchNextRequest()
{
    if (requestHead == nullptr || offerHead == nullptr)
        return 0;

    RideRequest *req = requestHead;
    RideOffer   *off = offerHead;

    while (off != nullptr)
    {
        if (off->seatsLeft > 0 &&
            off->startPlace == req->fromPlace &&
            off->endPlace   == req->toPlace &&
            off->departTime >= req->earliest &&
            off->departTime <= req->latest)
        {
            // Match found
            off->seatsLeft--;

            // Remove request from list
            requestHead = req->next;
            delete req;

            return 1; // success
        }
        off = off->next;
    }

    return 0; // no match
}*/

// ---------------- PRINT OFFERS (DEBUG) ----------------
void PrintOffers()
{
    RideOffer *o = offerHead;
    cout << "Ride Offers:\n";
    while (o)
    {
        cout << "OfferID: " << o->offerId
             << " Driver: " << o->driverId
             << " SeatsLeft: " << o->seatsLeft
             << " Depart: " << o->departTime << endl;
        o = o->next;
    }
}

// ---------------- PRINT REQUESTS (DEBUG) ----------------
/*void PrintRequests()
{
    RideRequest *r = requestHead;
    cout << "Ride Requests:\n";
    while (r)
    {
        cout << "RequestID: " << r->requestId
             << " Passenger: " << r->passengerId
             << " Window: [" << r->earliest
             << ", " << r->latest << "]" << endl;
        r = r->next;
    }
}*/

/*
---------------- TEST EXAMPLE ----------------
int main()
{
    CreateRideOffer(1, 101, "A", "B", 10, 2);
    CreateRideOffer(2, 102, "A", "C", 11, 1);

    CreateRideRequest(1, 201, "A", "B", 9, 11);
    CreateRideRequest(2, 202, "A", "B", 10, 12);

    PrintOffers();
    PrintRequests();

    cout << "Match: " << MatchNextRequest() << endl;
    cout << "Match: " << MatchNextRequest() << endl;
    cout << "Match: " << MatchNextRequest() << endl;

    PrintOffers();
}
*/

// =======================================================
// STEP 1.4: Reachable Areas Within Cost (DIJKSTRA)
// Computes and prints all areas reachable from a ride offer's
// start place within a given cost bound.
// =======================================================



// ---------- Helper: store distance to a place ----------
struct DistEntry {
    Place* place;
    int dist;
};

// ---------- Simple Min Priority Queue (array-based) ----------
struct MinPQ {
    Place* p[100];
    int    d[100];
    int size;

    MinPQ() { size = 0; }

    void push(Place* place, int dist) {
        int i = size++;
        p[i] = place;
        d[i] = dist;
        while (i > 0 && d[(i-1)/2] > d[i]) {
            swap(d[i], d[(i-1)/2]);
            swap(p[i], p[(i-1)/2]);
            i = (i-1)/2;
        }
    }

    void pop(Place*& place, int& dist) {
        place = p[0];
        dist  = d[0];
        p[0] = p[--size];
        d[0] = d[size];
        int i = 0;
        while (true) {
            int l = 2*i + 1, r = 2*i + 2, s = i;
            if (l < size && d[l] < d[s]) s = l;
            if (r < size && d[r] < d[s]) s = r;
            if (s == i) break;
            swap(d[i], d[s]);
            swap(p[i], p[s]);
            i = s;
        }
    }

    bool empty() { return size == 0; }
};

// ---------- MAIN FUNCTION ----------
void PrintReachableWithinCost(RideOffer* offer, int costBound)
{
    if (!offer || !offer->startPlace) return;

    DistEntry dist[100];
    int distCount = 0;

    auto getIndex = [&](Place* p) {
        for (int i = 0; i < distCount; i++)
            if (dist[i].place == p)
                return i;
        dist[distCount].place = p;
        dist[distCount].dist  = INT_MAX;
        return distCount++;
    };

    MinPQ pq;

    int startIdx = getIndex(offer->startPlace);
    dist[startIdx].dist = 0;
    pq.push(offer->startPlace, 0);

    cout << "Reachable areas within cost " << costBound << ":";

    while (!pq.empty())
    {
        Place* u;
        int d_u;
        pq.pop(u, d_u);

        if (d_u > costBound) break;

        cout << "- " << u->name << " (cost=" << d_u << ")" << endl;

        RoadLink* edge = u->firstLink;
        while (edge)
        {
            Place* v = edge->to;
            int newDist = d_u + edge->cost;

            if (newDist <= costBound)
            {
                int vIdx = getIndex(v);
                if (newDist < dist[vIdx].dist)
                {
                    dist[vIdx].dist = newDist;
                    pq.push(v, newDist);
                }
            }
            edge = edge->next;
        }
    }
}

bool ComputeShortestPath(
    Place* start,
    Place* end,
    Place* path[],
    int& pathLen
)
{
    Place* places[100];
    int dist[100];
    Place* parent[100];
    int count = 0;

    auto getIndex = [&](Place* p) {
        for (int i = 0; i < count; i++)
            if (places[i] == p)
                return i;
        places[count] = p;
        dist[count] = INT_MAX;
        parent[count] = nullptr;
        return count++;
    };

    MinPQ pq;

    int s = getIndex(start);
    dist[s] = 0;
    pq.push(start, 0);

    while (!pq.empty())
    {
        Place* u;
        int d;
        pq.pop(u, d);

        int uIdx = getIndex(u);
        if (u == end) break;

        RoadLink* e = u->firstLink;
        while (e)
        {
            Place* v = e->to;
            int vIdx = getIndex(v);
            int nd = d + e->cost;

            if (nd < dist[vIdx])
            {
                dist[vIdx] = nd;
                parent[vIdx] = u;
                pq.push(v, nd);
            }
            e = e->next;
        }
    }

    int endIdx = getIndex(end);
    if (dist[endIdx] == INT_MAX)
        return false;

    // reconstruct path (reverse)
    pathLen = 0;
    Place* cur = end;
    while (cur)
    {
        path[pathLen++] = cur;
        int i = getIndex(cur);
        cur = parent[i];
    }

    // reverse
    for (int i = 0; i < pathLen / 2; i++)
        swap(path[i], path[pathLen - 1 - i]);

    return true;
}

bool IsSubPath(
    Place* driverPath[], int dLen,
    Place* passengerPath[], int pLen
)
{
    if (pLen > dLen) return false;

    for (int i = 0; i <= dLen - pLen; i++)
    {
        bool match = true;
        for (int j = 0; j < pLen; j++)
        {
            if (driverPath[i + j] != passengerPath[j])
            {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

RideRequest* ExtractMinRequest()
{
    if (requestCount == 0)
        return nullptr;

    RideRequest* minReq = requestHeap[0];

    requestHeap[0] = requestHeap[--requestCount];
    if (requestCount > 0)
    {
        requestHeap[0]->heapIndex = 0;

        int i = 0;
        while (true)
        {
            int l = 2*i + 1, r = 2*i + 2, s = i;
            if (l < requestCount &&
                requestHeap[l]->earliest < requestHeap[s]->earliest)
                s = l;
            if (r < requestCount &&
                requestHeap[r]->earliest < requestHeap[s]->earliest)
                s = r;
            if (s == i) break;
            swapRequests(i, s);
            i = s;
        }
    }

    requestHead = (requestCount > 0) ? requestHeap[0] : nullptr;
    return minReq;
}

int MatchNextRequest()
{
    RideRequest* req = ExtractMinRequest();
    if (!req) return 0;

    RideOffer* off = offerHead;

    while (off)
    {
        if (off->seatsLeft > 0 &&
            off->departTime >= req->earliest &&
            off->departTime <= req->latest)
        {
            Place* driverPath[100];
            Place* passengerPath[100];
            int dLen, pLen;

            bool dOK = ComputeShortestPath(
                off->startPlace,
                off->endPlace,
                driverPath, dLen
            );

            bool pOK = ComputeShortestPath(
                req->fromPlace,
                req->toPlace,
                passengerPath, pLen
            );

            if (dOK && pOK &&
                IsSubPath(driverPath, dLen, passengerPath, pLen))
            {
                off->seatsLeft--;

                AddHistory(off->driverId,
                           off->offerId,
                           req->fromPlace->name,
                           req->toPlace->name,
                           off->departTime);

                AddHistory(req->passengerId,
                           off->offerId,
                           req->fromPlace->name,
                           req->toPlace->name,
                           off->departTime);

                delete req;
                return 1;
            }
        }
        off = off->next;
    }

    // No match → reinsert request
    CreateRideRequest(req->requestId,
                      req->passengerId,
                      req->fromPlace->name,
                      req->toPlace->name,
                      req->earliest,
                      req->latest);

    delete req;
    return 0;
}



// =======================================================
// TEST CODE: Verify Ride Creation, Matching, and Reachability
// File: test_ride.cpp
// =======================================================

/*
Compile example:
 g++ ride.cpp test_ride.cpp -o test
*/


// ---------- MAIN TEST ----------


