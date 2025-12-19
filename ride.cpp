#include "ride.h"
#include <iostream>
#include <cstring>
#include <climits>

using namespace std;

#define MAX_REQUESTS 1000
#define ACTIVE_RIDE_TABLE_SIZE 101

RideRequest *requestHeap[MAX_REQUESTS];
ActiveRide *activeRideTable[ACTIVE_RIDE_TABLE_SIZE] = {nullptr};

static int HashRideId(int rideId);

int ActiveRideTableSize()
{
    return ACTIVE_RIDE_TABLE_SIZE;
}

ActiveRide* ActiveRideBucketHead(int idx)
{
    if (idx < 0 || idx >= ACTIVE_RIDE_TABLE_SIZE) return nullptr;
    return activeRideTable[idx];
}

static void FreePassengerList(PassengerNode* p)
{
    while (p)
    {
        PassengerNode* nxt = p->next;
        delete p;
        p = nxt;
    }
}

void ClearActiveRides()
{
    for (int i = 0; i < ACTIVE_RIDE_TABLE_SIZE; i++)
    {
        ActiveRide* cur = activeRideTable[i];
        while (cur)
        {
            ActiveRide* nxt = cur->next;
            FreePassengerList(cur->passengers);
            delete cur;
            cur = nxt;
        }
        activeRideTable[i] = nullptr;
    }
}

static RideOffer* FindOfferById(int offerId)
{
    RideOffer* o = offerHead;
    while (o)
    {
        if (o->offerId == offerId) return o;
        o = o->next;
    }
    return nullptr;
}

void StorageInsertActiveRide(int rideId, int offerId, const int* passengerIds, int passengerCount)
{
    RideOffer* offer = FindOfferById(offerId);
    if (!offer) return;

    int idx = HashRideId(rideId);

    ActiveRide* ar = new ActiveRide;
    ar->rideId = rideId;
    ar->offer = offer;
    ar->passengers = nullptr;

    for (int i = passengerCount - 1; i >= 0; i--)
    {
        PassengerNode* p = new PassengerNode;
        p->passengerId = passengerIds[i];
        p->next = ar->passengers;
        ar->passengers = p;
    }

    ar->next = activeRideTable[idx];
    activeRideTable[idx] = ar;
}

static int HashRideId(int rideId)
{
    return rideId % ACTIVE_RIDE_TABLE_SIZE;
}

ActiveRide *FindActiveRide(int rideId)
{
    int idx = HashRideId(rideId);
    ActiveRide *cur = activeRideTable[idx];
    while (cur)
    {
        if (cur->rideId == rideId)
            return cur;
        cur = cur->next;
    }
    return nullptr;
}

void InsertActiveRide(RideOffer *offer, int passengerId)
{
    int idx = HashRideId(offer->offerId);

    ActiveRide *ar = new ActiveRide;
    ar->rideId = offer->offerId;
    ar->offer = offer;

    PassengerNode *p = new PassengerNode;
    p->passengerId = passengerId;
    p->next = nullptr;
    ar->passengers = p;

    ar->next = activeRideTable[idx];
    activeRideTable[idx] = ar;
}

void AddPassengerToActiveRide(ActiveRide *ar, int passengerId)
{
    PassengerNode *p = new PassengerNode;
    p->passengerId = passengerId;
    p->next = ar->passengers;
    ar->passengers = p;
}

// Forward declaration

// ---------------- RIDE OFFER ----------------

// ---------------- RIDE REQUEST ----------------

// Global heads (defined in ride.cpp)

// ----------- REQUIRED FUNCTIONS -----------

// ============================
// ride.cpp
// ============================

// ---------------- GLOBAL HEADS ----------------

// ---------------- CREATE RIDE OFFER ----------------
RideOffer *CreateRideOffer(int offerId, int driverId,
                           const char *start, const char *end,
                           int departTime, int capacity)
{
    RideOffer *o = new RideOffer;

    o->offerId = offerId;
    o->driverId = driverId;
    o->startPlace = GetOrCreatePlace(start);
    o->endPlace = GetOrCreatePlace(end);
    o->departTime = departTime;
    o->capacity = capacity;
    o->seatsLeft = capacity;

    o->next = offerHead;
    offerHead = o;

    return o;
}

void swapRequests(int i, int j)
{
    RideRequest *tmp = requestHeap[i];
    requestHeap[i] = requestHeap[j];
    requestHeap[j] = tmp;

    requestHeap[i]->heapIndex = i;
    requestHeap[j]->heapIndex = j;
}

void heapifyUp(int i)
{
    while (i > 0)
    {
        int p = (i - 1) / 2;
        if (requestHeap[p]->earliest <= requestHeap[i]->earliest)
            break;
        swapRequests(p, i);
        i = p;
    }
}

RideRequest *CreateRideRequest(int requestId, int passengerId,
                               const char *from, const char *to,
                               int earliest, int latest)
{
    if (!PassengerExists(passengerId))
        return nullptr;

    RideRequest *r = new RideRequest;
    r->requestId = requestId;
    r->passengerId = passengerId;
    r->fromPlace = GetOrCreatePlace(from);
    r->toPlace = GetOrCreatePlace(to);
    r->earliest = earliest;
    r->latest = latest;

    int idx = requestCount++;
    requestHeap[idx] = r;
    r->heapIndex = idx;

    heapifyUp(idx);
    requestHead = requestHeap[0];

    return r;
}

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

void PrintRequests()
{
    cout << "Ride Requests (heap order):\n";
    for (int i = 0; i < requestCount; i++)
    {
        RideRequest* r = requestHeap[i];
        if (!r) continue;
        cout << "RequestID: " << r->requestId
             << " Passenger: " << r->passengerId
             << " From: " << (r->fromPlace ? r->fromPlace->name : "NULL")
             << " To: " << (r->toPlace ? r->toPlace->name : "NULL")
             << " Earliest: " << r->earliest
             << " Latest: " << r->latest
             << '\n';
    }
}



    // ---------- Helper: store distance to a place ----------
    struct DistEntry
{
    Place *place;
    int dist;
};

// ---------- Simple Min Priority Queue (array-based) ----------
struct MinPQ
{
    Place *p[100];
    int d[100];
    int size;

    MinPQ() { size = 0; }

    void push(Place *place, int dist)
    {
        int i = size++;
        p[i] = place;
        d[i] = dist;
        while (i > 0 && d[(i - 1) / 2] > d[i])
        {
            swap(d[i], d[(i - 1) / 2]);
            swap(p[i], p[(i - 1) / 2]);
            i = (i - 1) / 2;
        }
    }

    void pop(Place *&place, int &dist)
    {
        place = p[0];
        dist = d[0];
        p[0] = p[--size];
        d[0] = d[size];
        int i = 0;
        while (true)
        {
            int l = 2 * i + 1, r = 2 * i + 2, s = i;
            if (l < size && d[l] < d[s])
                s = l;
            if (r < size && d[r] < d[s])
                s = r;
            if (s == i)
                break;
            swap(d[i], d[s]);
            swap(p[i], p[s]);
            i = s;
        }
    }

    bool empty() { return size == 0; }
};

// ---------- MAIN FUNCTION ----------
void PrintReachableWithinCost(RideOffer *offer, int costBound)
{
    if (!offer || !offer->startPlace)
        return;

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

    MinPQ pq;

    int startIdx = getIndex(offer->startPlace);
    dist[startIdx].dist = 0;
    pq.push(offer->startPlace, 0);

    cout << "Reachable areas within cost " << costBound << ":";

    while (!pq.empty())
    {
        Place *u;
        int d_u;
        pq.pop(u, d_u);

        if (d_u > costBound)
            break;

        cout << "- " << u->name << " (cost=" << d_u << ")" << endl;

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
                    pq.push(v, newDist);
                }
            }
            edge = edge->next;
        }
    }
}

bool ComputeShortestPath(
    Place *start,
    Place *end,
    Place *path[],
    int &pathLen)
{
    Place *places[100];
    int dist[100];
    Place *parent[100];
    int count = 0;

    auto getIndex = [&](Place *p)
    {
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
        Place *u;
        int d;
        pq.pop(u, d);

        int uIdx = getIndex(u);
        if (u == end)
            break;

        RoadLink *e = u->firstLink;
        while (e)
        {
            Place *v = e->to;
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
    Place *cur = end;
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
    Place *driverPath[], int dLen,
    Place *passengerPath[], int pLen)
{
    if (pLen == 0 || dLen == 0)
        return false;
    
    // Passenger path must have at least start and end
    if (pLen < 2)
        return false;
    
    Place *passengerStart = passengerPath[0];
    Place *passengerEnd = passengerPath[pLen - 1];
    
    // Find passenger start in driver path
    int startIdx = -1;
    for (int i = 0; i < dLen; i++)
    {
        if (driverPath[i] == passengerStart)
        {
            startIdx = i;
            break;
        }
    }
    
    if (startIdx == -1)
        return false; // Passenger start not in driver path
    
    // Find passenger end in driver path, must be after start
    int endIdx = -1;
    for (int i = startIdx; i < dLen; i++)
    {
        if (driverPath[i] == passengerEnd)
        {
            endIdx = i;
            break;
        }
    }
    
    if (endIdx == -1)
        return false; // Passenger end not in driver path after start
    
    // If passenger path is just start->end, it's valid if both are in driver path in order
    if (pLen == 2)
        return true;
    
    // For longer passenger paths, check if all intermediate places exist in driver path in order
    int driverIdx = startIdx;
    for (int p = 1; p < pLen - 1; p++)
    {
        bool found = false;
        // Look for passengerPath[p] in driver path after current position
        for (int d = driverIdx + 1; d <= endIdx; d++)
        {
            if (driverPath[d] == passengerPath[p])
            {
                driverIdx = d;
                found = true;
                break;
            }
        }
        if (!found)
            return false; // Intermediate place not found in correct order
    }
    
    return true; // All passenger places found in driver path in correct order
}

RideRequest *ExtractMinRequest()
{
    if (requestCount == 0)
        return nullptr;

    RideRequest *minReq = requestHeap[0];
    requestHeap[0] = requestHeap[--requestCount];

    if (requestCount > 0)
    {
        requestHeap[0]->heapIndex = 0;
        int i = 0;
        while (true)
        {
            int l = 2 * i + 1, r = 2 * i + 2, s = i;
            if (l < requestCount &&
                requestHeap[l]->earliest < requestHeap[s]->earliest)
                s = l;
            if (r < requestCount &&
                requestHeap[r]->earliest < requestHeap[s]->earliest)
                s = r;
            if (s == i)
                break;
            swapRequests(i, s);
            i = s;
        }
    }

    requestHead = (requestCount > 0) ? requestHeap[0] : nullptr;
    return minReq;
}

int MatchNextRequest()
{
    RideRequest *req = ExtractMinRequest();
    if (!req)
        return 0;

    RideOffer *off = offerHead;

    while (off)
    {
        if (off->seatsLeft > 0 &&
            off->departTime >= req->earliest &&
            off->departTime <= req->latest)
        {
            Place *dp[100], *pp[100];
            int dl, pl;

            if (ComputeShortestPath(off->startPlace, off->endPlace, dp, dl) &&
                ComputeShortestPath(req->fromPlace, req->toPlace, pp, pl) &&
                IsSubPath(dp, dl, pp, pl))
            {
                off->seatsLeft--;

                ActiveRide *ar = FindActiveRide(off->offerId);
                if (!ar)
                    InsertActiveRide(off, req->passengerId);
                else
                    AddPassengerToActiveRide(ar, req->passengerId);

                // Phase 9 — Track completed rides (treat successful match as completion)
                {
                    User* driver = SearchUser(userRoot, off->driverId);
                    if (driver && driver->isDriver == 1)
                        driver->completedRides++;
                }

                AddHistory(off->driverId, off->offerId,
                           req->fromPlace->name, req->toPlace->name,
                           off->departTime);

                AddHistory(req->passengerId, off->offerId,
                           req->fromPlace->name, req->toPlace->name,
                           off->departTime);

                delete req;
                return 1;
            }
        }
        off = off->next;
    }

    // no match → reinsert
    CreateRideRequest(req->requestId, req->passengerId,
                      req->fromPlace->name, req->toPlace->name,
                      req->earliest, req->latest);
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
