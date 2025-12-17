#ifndef RIDE_H
#define RIDE_H


#include "roads.h"
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
struct RideRequest {
    int requestId;
    int passengerId;
    Place* fromPlace;
    Place* toPlace;
    int earliest;
    int latest;
    RideRequest* next;
};

// =======================
// GLOBAL LIST HEADS
// (defined in ride.cpp)
// =======================
extern RideOffer*   offerHead;
extern RideRequest* requestHead;

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

#endif // RIDE_H
