#include "roads.h"



RoadLink* appendNodetoRoadList(RoadLink* head, RoadLink* new_node)
{
    if (head == nullptr)
        return new_node;

    RoadLink *current = head;
    while (current->next != nullptr)
        current = current->next;

    current->next = new_node;
    return head;
}

Place *GetOrCreatePlace(const char *name)
{
    Place *current = placeHead;
    Place *prev = nullptr;

    while (current != nullptr && strcmp(current->name, name) != 0)
    {
        prev = current;
        current = current->next;
    }

    if (current != nullptr)
        return current;

    Place *newPlace = new Place;
    newPlace->name = new char[strlen(name) + 1];
    strcpy(newPlace->name, name);
    newPlace->firstLink = nullptr;
    newPlace->next = nullptr;

    if (prev == nullptr)
        placeHead = newPlace;
    else
        prev->next = newPlace;

    return newPlace;
}

void AddRoad(const char *from, const char *to, int cost)
{
    Place *fromPlace = GetOrCreatePlace(from);
    Place *toPlace = GetOrCreatePlace(to);

    RoadLink *newRoad = new RoadLink;
    newRoad->cost = cost;
    newRoad->to = toPlace;
    newRoad->next = nullptr;

    fromPlace->firstLink =
        appendNodetoRoadList(fromPlace->firstLink, newRoad);
}

void printGraph()
{
    Place *p = placeHead;

    while (p != nullptr)
    {
        cout << "Place: " << p->name << endl;
        RoadLink *link = p->firstLink;

        while (link != nullptr)
        {
            cout << "  -> " << link->to->name
                 << " (cost: " << link->cost << ")" << endl;
            link = link->next;
        }

        p = p->next;
        cout << endl;
    }
}

// loadRoadNetworkFromFile moved to storage.cpp (Phase 10 — File Storage)
