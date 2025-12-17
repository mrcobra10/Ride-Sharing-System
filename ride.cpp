// ============================
#include "ride.h"
#include <climits>
// ============================


#include <iostream>
#include <cstring>
using namespace std;

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

// ---------------- CREATE RIDE REQUEST ----------------
RideRequest* CreateRideRequest(int requestId, int passengerId,
                               const char *from, const char *to,
                               int earliest, int latest)
{
    RideRequest *r = new RideRequest;

    r->requestId  = requestId;
    r->passengerId = passengerId;
    r->fromPlace  = GetOrCreatePlace(from);
    r->toPlace    = GetOrCreatePlace(to);
    r->earliest   = earliest;
    r->latest     = latest;

    // Insert at head of linked list
    r->next = requestHead;
    requestHead = r;

    return r;
}

// ---------------- MATCH NEXT REQUEST ----------------
int MatchNextRequest()
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

// ---------------- PRINT REQUESTS (DEBUG) ----------------
void PrintRequests()
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
}

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


// =======================================================
// TEST CODE: Verify Ride Creation, Matching, and Reachability
// File: test_ride.cpp
// =======================================================

/*
Compile example:
 g++ ride.cpp test_ride.cpp -o test
*/


// ---------- MAIN TEST ----------


