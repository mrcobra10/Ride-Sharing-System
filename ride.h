#ifndef RIDE_H
#define RIDE_H


#include "roads.h"
#include "user.h"
#include <iostream>
#include <climits>

// Forward declarations


// =======================
// RIDE OFFER
// =======================
struct RideOffer {
    int offerId;
    int driverId;
    Place* startPlace;
    Place* endPlace;
    int departTime;
    int capacity;
    int seatsLeft;
    RideOffer* next;
};

// =======================
// RIDE REQUEST
// =======================
struct RideRequest
{
    int requestId;
    int passengerId;
    Place *fromPlace;
    Place *toPlace;
    int earliest;
    int latest;

    // heap support
    int heapIndex;
};

struct PassengerNode {
    int passengerId;
    PassengerNode* next;
};

struct ActiveRide {
    int rideId;
    RideOffer* offer;
    PassengerNode* passengers;
    ActiveRide* next;
};





// =======================
// GLOBAL LIST HEADS
// (defined in ride.cpp)
// =======================
extern RideOffer* offerHead;
extern RideRequest* requestHead;
extern int requestCount;

// =======================
// STORAGE HOOKS (Phase 10)
// =======================
int ActiveRideTableSize();
ActiveRide* ActiveRideBucketHead(int idx);
void ClearActiveRides();
void StorageInsertActiveRide(int rideId, int offerId, const int* passengerIds, int passengerCount);

// =======================
// CORE FUNCTIONS
// =======================
RideOffer* CreateRideOffer(
    int offerId,
    int driverId,
    const char* start,
    const char* end,
    int departTime,
    int capacity
);

RideRequest* CreateRideRequest(
    int requestId,
    int passengerId,
    const char* from,
    const char* to,
    int earliest,
    int latest
);

int MatchNextRequest();

// =======================
// DEBUG / UTILITY
// =======================
void PrintOffers();
void PrintRequests();

// =======================
// GRAPH / PLACE INTERFACE
// (implemented in roads.cpp)
// =======================


// =======================
// STEP 1.4 – DIJKSTRA
// =======================
void PrintReachableWithinCost(RideOffer* offer, int costBound);

int MatchNextRequest();
RideRequest* ExtractMinRequest();
bool IsSubPath(
    Place* driverPath[], int dLen,
    Place* passengerPath[], int pLen
);
bool ComputeShortestPath(
    Place* start,
    Place* end,
    Place* path[],
    int& pathLen
);


#endif // RIDE_H
