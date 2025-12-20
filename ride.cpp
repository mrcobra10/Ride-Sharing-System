#include "ride.h"
#include "user.h"
#include "roads.h"
#include "storage.h"
#include <iostream>
#include <cstring>
#include <climits>

using namespace std;

#define MAX_REQUESTS 1000
#define ACTIVE_RIDE_TABLE_SIZE 101

// ---------------- GLOBAL STORAGE ----------------
RideRequest *requestHeap[MAX_REQUESTS];

ActiveRide *activeRideTable[ACTIVE_RIDE_TABLE_SIZE] = {nullptr};


//Function to output time in 24 hour format made by Umer Faisal
static void PrintTime(int time) {
    int hour = time / 100;
    int minute = time % 100;

    // Print in HH:MM format with leading zeros
    cout << setw(2) << setfill('0') << hour 
         << ":" 
         << setw(2) << setfill('0') << minute;
}

// ---------------- HASH ----------------
static int HashRideId(int rideId)
{
    return rideId % ACTIVE_RIDE_TABLE_SIZE;
}

// ---------------- PASSENGER LIST UTIL ----------------
static void FreePassengerList(PassengerNode *p)
{
    while (p)
    {
        PassengerNode *nxt = p->next;
        delete p;
        p = nxt;
    }
}

// ---------------- ACTIVE RIDE UTILS ----------------
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

void ClearActiveRides()
{
    for (int i = 0; i < ACTIVE_RIDE_TABLE_SIZE; i++)
    {
        ActiveRide *cur = activeRideTable[i];
        while (cur)
        {
            ActiveRide *nxt = cur->next;
            FreePassengerList(cur->passengers);
            delete cur;
            cur = nxt;
        }
        activeRideTable[i] = nullptr;
    }
}

int ActiveRideTableSize()
{
    return ACTIVE_RIDE_TABLE_SIZE;
}

ActiveRide *ActiveRideBucketHead(int idx)
{
    if (idx < 0 || idx >= ACTIVE_RIDE_TABLE_SIZE)
        return nullptr;
    return activeRideTable[idx];
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

// ---------------- FIND OFFER ----------------
static RideOffer *FindOfferById(int offerId)
{
    RideOffer *o = offerHead;
    while (o)
    {
        if (o->offerId == offerId)
            return o;
        o = o->next;
    }
    return nullptr;
}

// ---------------- STORAGE INSERT ----------------
void StorageInsertActiveRide(int rideId, int offerId, const int *passengerIds, int passengerCount)
{
    RideOffer *offer = FindOfferById(offerId);
    if (!offer)
        return;

    int idx = HashRideId(rideId);
    ActiveRide *ar = new ActiveRide;
    ar->rideId = rideId;
    ar->offer = offer;
    ar->passengers = nullptr;

    for (int i = passengerCount - 1; i >= 0; i--)
    {
        PassengerNode *p = new PassengerNode;
        p->passengerId = passengerIds[i];
        p->next = ar->passengers;
        ar->passengers = p;
    }

    ar->next = activeRideTable[idx];
    activeRideTable[idx] = ar;
}

// ---------------- PRINT FUNCTIONS ----------------
void PrintOffers()
{
    RideOffer *o = offerHead;
    cout << "Ride Offers:\n";
    while (o)
    {
        cout << "OfferID: " << o->offerId
             << " Driver: " << o->driverId
             << " SeatsLeft: " << o->seatsLeft
             << " Depart: " << o->departTime << "\n";
        o = o->next;
    }
}

void PrintRequests()
{
    cout << "Ride Requests (heap order):\n";
    for (int i = 0; i < requestCount; i++)
    {
        RideRequest *r = requestHeap[i];
        if (!r)
            continue;
        cout << "RequestID: " << r->requestId
             << " Passenger: " << r->passengerId
             << " From: " << (r->fromPlace ? r->fromPlace->name : "NULL")
             << " To: " << (r->toPlace ? r->toPlace->name : "NULL")
             << " Earliest: " << r->earliest
             << " Latest: " << r->latest << '\n';
    }
}

// ---------------- CREATE OFFER ----------------
RideOffer *CreateRideOffer(int offerId, int driverId, const char *start, const char *end, int departTime, int capacity, bool silent)
{
    if (!DriverExists(driverId))
    {
        if (!silent)
            cout << "Error: Driver with ID " << driverId << " does not exist or is not a driver.\n";
        return nullptr;
    }

    Place *startPlace = GetOrCreatePlace(start);
    Place *endPlace = GetOrCreatePlace(end);
    
    if (!startPlace || !endPlace)
    {
        if (!silent)
            cout << "Error: Invalid places.\n";
        return nullptr;
    }

    RideOffer *o = new RideOffer;
    o->offerId = offerId;
    o->driverId = driverId;
    o->startPlace = startPlace;
    o->endPlace = endPlace;
    o->departTime = departTime;
    o->capacity = capacity;
    o->seatsLeft = capacity;
    o->next = offerHead;
    offerHead = o;

    // Display shortest path (only if not silent)
    if (!silent)
    {
        Place *path[100];
        int pathLen = 0;
        if (ComputeShortestPath(startPlace, endPlace, path, pathLen))
        {
            cout << "Ride offer created successfully.\n";
            cout << "Shortest path from " << start << " to " << end << ":\n";
            int totalCost = 0;
            for (int i = 0; i < pathLen; i++)
            {
                cout << path[i]->name;
                if (i < pathLen - 1)
                {
                    // Calculate cost to next place
                    bool found = false;
                    for (RoadLink *link = path[i]->firstLink; link; link = link->next)
                    {
                        if (link->to == path[i + 1])
                        {
                            totalCost += link->cost;
                            cout << " --(" << link->cost << ")--> ";
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        cout << " --(?)--> ";
                    }
                }
            }
            cout << "\nTotal cost: " << totalCost << "\n";
        }
        else
        {
            cout << "Ride offer created, but no path found from " << start << " to " << end << ".\n";
        }
    }

    return o;
}

// ---------------- REQUEST HEAP ----------------
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

RideRequest *CreateRideRequest(int requestId, int passengerId, const char *from, const char *to, int earliest, int latest, bool silent)
{
    if (!PassengerExists(passengerId))
    {
        if (!silent)
            cout << "Error: Passenger with ID " << passengerId << " does not exist or is not a passenger.\n";
        return nullptr;
    }

    Place *fromPlace = GetOrCreatePlace(from);
    Place *toPlace = GetOrCreatePlace(to);
    
    if (!fromPlace || !toPlace)
    {
        if (!silent)
            cout << "Error: Invalid places.\n";
        return nullptr;
    }

    RideRequest *r = new RideRequest;
    r->requestId = requestId;
    r->passengerId = passengerId;
    r->fromPlace = fromPlace;
    r->toPlace = toPlace;
    r->earliest = earliest;
    r->latest = latest;

    int idx = requestCount++;
    requestHeap[idx] = r;
    r->heapIndex = idx;

    heapifyUp(idx);
    requestHead = requestHeap[0];

    // Display shortest path (only if not silent)
    if (!silent)
    {
        Place *path[100];
        int pathLen = 0;
        if (ComputeShortestPath(fromPlace, toPlace, path, pathLen))
        {
            cout << "Ride request created successfully.\n";
            cout << "Shortest path from " << from << " to " << to << ":\n";
            int totalCost = 0;
            for (int i = 0; i < pathLen; i++)
            {
                cout << path[i]->name;
                if (i < pathLen - 1)
                {
                    // Calculate cost to next place
                    bool found = false;
                    for (RoadLink *link = path[i]->firstLink; link; link = link->next)
                    {
                        if (link->to == path[i + 1])
                        {
                            totalCost += link->cost;
                            cout << " --(" << link->cost << ")--> ";
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        cout << " --(?)--> ";
                    }
                }
            }
            cout << "\nTotal cost: " << totalCost << "\n";
        }
        else
        {
            cout << "Ride request created, but no path found from " << from << " to " << to << ".\n";
        }
    }

    return r;
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
            if (l < requestCount && requestHeap[l]->earliest < requestHeap[s]->earliest)
                s = l;
            if (r < requestCount && requestHeap[r]->earliest < requestHeap[s]->earliest)
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

// ---------------- MIN PQ (FOR DIJKSTRA) ----------------
struct MinPQ
{
    Place *p[100];
    int d[100];
    int size = 0;

    void push(Place *pl, int dist)
    {
        int i = size++;
        p[i] = pl;
        d[i] = dist;
        while (i > 0 && d[(i - 1) / 2] > d[i])
        {
            swap(d[i], d[(i - 1) / 2]);
            swap(p[i], p[(i - 1) / 2]);
            i = (i - 1) / 2;
        }
    }

    void pop(Place *&pl, int &dist)
    {
        pl = p[0];
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

// ---------------- SHORTEST PATH ----------------
bool ComputeShortestPath(Place *start, Place *end, Place *path[], int &pathLen)
{
    if (start == end)
    {
        path[0] = start;
        pathLen = 1;
        return true;
    }

    Place *places[100];
    int dist[100];
    Place *parent[100];
    bool visited[100] = {false};
    int count = 0;

    auto idxOf = [&](Place *p)
    {
        for (int i = 0; i < count; i++)
            if (places[i] == p)
                return i;
        return -1;
    };

    auto addPlace = [&](Place *p)
    {
        places[count] = p;
        dist[count] = INT_MAX;
        parent[count] = nullptr;
        visited[count] = false;
        return count++;
    };

    MinPQ pq;
    int s = addPlace(start);
    dist[s] = 0;
    parent[s] = nullptr;  // Start has no parent
    pq.push(start, 0);

    while (!pq.empty())
    {
        Place *u;
        int d;
        pq.pop(u, d);

        int uIdx = idxOf(u);
        if (uIdx == -1)
            continue;  // Should not happen, but handle gracefully
        if (visited[uIdx])
            continue;  // Already processed this node
        if (d > dist[uIdx])
            continue;  // Found a shorter path to this node, skip
        
        visited[uIdx] = true;
        
        if (u == end)
            break;  // Reached destination

        // Explore neighbors
        for (RoadLink *e = u->firstLink; e; e = e->next)
        {
            Place *v = e->to;
            if (!v)
                continue;  // Safety check
            
            int vIdx = idxOf(v);
            if (vIdx == -1)
                vIdx = addPlace(v);
            
            if (visited[vIdx])
                continue;  // Already processed
                
            int nd = d + e->cost;
            if (nd < dist[vIdx])
            {
                dist[vIdx] = nd;
                parent[vIdx] = u;
                pq.push(v, nd);
            }
        }
    }

    int endIdx = idxOf(end);
    if (endIdx == -1 || dist[endIdx] == INT_MAX)
        return false;

    // Reconstruct path backwards from end to start
    Place *tempPath[100];
    int tempLen = 0;
    Place *current = end;
    
    // Build path from end to start by following parent pointers
    while (current != nullptr)
    {
        tempPath[tempLen++] = current;
        int currIdx = idxOf(current);
        if (currIdx == -1)
        {
            // This shouldn't happen if algorithm worked correctly, but handle it
            return false;
        }
        if (current == start)
            break;  // Reached start, stop
        current = parent[currIdx];
    }

    // Ensure start is in path (it should be, but double-check)
    if (tempLen == 0 || tempPath[tempLen - 1] != start)
    {
        // Add start if missing
        tempPath[tempLen++] = start;
    }

    // Reverse to get path from start to end
    pathLen = 0;
    for (int i = tempLen - 1; i >= 0; i--)
    {
        path[pathLen++] = tempPath[i];
    }

    return true;
}

// ---------------- SUBPATH CHECK ----------------
bool IsSubPath(Place *driverPath[], int dLen, Place *passengerPath[], int pLen)
{
    if (pLen > dLen)
        return false;

    for (int i = 0; i <= dLen - pLen; i++)
    {
        bool ok = true;
        for (int j = 0; j < pLen; j++)
            if (driverPath[i + j] != passengerPath[j])
                ok = false;
        if (ok)
            return true;
    }
    return false;
}

// ---------------- MATCH REQUEST ----------------
int MatchNextRequest()
{
    RideRequest *req = ExtractMinRequest();
    if (!req)
        return 0;

    for (RideOffer *off = offerHead; off; off = off->next)
    {
        // Check basic constraints
        if (off->seatsLeft <= 0)
            continue;
        if (off->departTime < req->earliest || off->departTime > req->latest)
            continue;

        // Compute paths
        Place *dp[100], *pp[100];
        int dl = 0, pl = 0;

        bool driverPathOk = ComputeShortestPath(off->startPlace, off->endPlace, dp, dl);
        bool passengerPathOk = ComputeShortestPath(req->fromPlace, req->toPlace, pp, pl);

        if (!driverPathOk || !passengerPathOk)
            continue;

        // Check if passenger path is a subpath of driver path
        if (IsSubPath(dp, dl, pp, pl))
        {
            off->seatsLeft--;
            ActiveRide *ar = FindActiveRide(off->offerId);
            if (!ar)
                InsertActiveRide(off, req->passengerId);
            else
                AddPassengerToActiveRide(ar, req->passengerId);

            AddHistory(off->driverId, off->offerId, req->fromPlace->name, req->toPlace->name, off->departTime);
            AddHistory(req->passengerId, off->offerId, req->fromPlace->name, req->toPlace->name, off->departTime);

            // Increment completed rides for driver
            User* driver = SearchUser(userRoot, off->driverId);
            if (driver && driver->isDriver == 1)
            {
                driver->completedRides++;
            }

            cout << "Matched!\n";
            cout << "Ride offer: \n";
            cout << "  Offer ID: " << off->offerId
                 << ", Driver ID: " << off->driverId
                 << ", From: " << off->startPlace->name
                 << ", To: " << off->endPlace->name
                 << ", Departure: ";
                  PrintTime(off->departTime);
                  cout<<endl;

            cout << "Ride request: \n";
            cout << "  Request ID: " << req->requestId
                 << ", Passenger ID: " << req->passengerId
                 << ", From: " << req->fromPlace->name
                 << ", To: " << req->toPlace->name;
                 cout<< ", Departure: ";
                 PrintTime(off->departTime);

                 cout<<endl;
                 
                 
                

            delete req;
            return 1;
        }
    }

    // If no match found, just delete the request (don't recreate it)
    delete req;
    return 0;
}

// ---------------- REACHABLE WITHIN COST ----------------
void PrintReachableWithinCost(RideOffer *offer, int costBound)
{
    if (!offer || !offer->startPlace)
        return;

    Place *places[100];
    int dist[100];
    int count = 0;

    auto idxOf = [&](Place *p)
    {
        for (int i = 0; i < count; i++)
            if (places[i] == p)
                return i;
        return -1;
    };

    auto addPlace = [&](Place *p)
    {
        places[count] = p;
        dist[count] = INT_MAX;
        return count++;
    };

    MinPQ pq;
    int s = addPlace(offer->startPlace);
    dist[s] = 0;
    pq.push(offer->startPlace, 0);

    cout << "Reachable areas within cost " << costBound << ":\n";

    while (!pq.empty())
    {
        Place *u;
        int d;
        pq.pop(u, d);

        int ui = idxOf(u);
        if (d > dist[ui] || d > costBound)
            continue;

        cout << "- " << u->name << " (cost=" << d << ")\n";

        for (RoadLink *e = u->firstLink; e; e = e->next)
        {
            Place *v = e->to;
            int vi = idxOf(v);
            if (vi == -1)
                vi = addPlace(v);
            int nd = d + e->cost;
            if (nd < dist[vi] && nd <= costBound)
            {
                dist[vi] = nd;
                pq.push(v, nd);
            }
        }
    }
}
