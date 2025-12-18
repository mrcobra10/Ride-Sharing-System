#ifndef ROADS_H
#define ROADS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

struct Place;
struct RoadLink;

struct RoadLink
{
    Place *to;
    int cost;
    RoadLink *next;
};

struct Place
{
    char *name;
    RoadLink *firstLink;
    Place *next;
};

// global head
extern Place *placeHead;

// function declarations
RoadLink* appendNodetoRoadList(RoadLink* head, RoadLink* new_node);
Place* GetOrCreatePlace(const char *name);
void AddRoad(const char *from, const char *to, int cost);
void printGraph();

#endif
