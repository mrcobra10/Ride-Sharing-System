// Ride Sharing Project
// Usman Joined

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctring>
using namespace std;

struct Place;
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
Place *placeHead = nullptr;
struct User
{
	int userId;
	char *name;
	int isDriver; // 1=driver,0=passenger
	int rating;
	User *left; // BST
	User *right;
};
struct RideOffer
{
	int offerId;
	int driverId;
	Place *startPlace;
	Place *endPlace;
	int departTime;
	int capacity;
	int seatsLeft;
	RideOffer *next;
};
struct RideRequest
{
	int requestId;
	int passengerId;
	Place *fromPlace;
	Place *toPlace;
	int earliest;
	int latest;
	RideRequest *next;
};

RoadLink* appendNodetoRoadList(RoadLink* head, RoadLink* new_node)

{

	if (head == NULL)
	{
		head = new_node;
		return head;
	}
	RoadLink *current = head;

	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = new_node;

	return head;
}

Place *GetOrCreatePlace(const char *name)
{
	Place *current = placeHead;
	Place *prev = nullptr;

	while (current != NULL && strcmp(current->name, name) != 0)
	{
		prev = current;
		current = current->next;
	}

	if (current != NULL)
	{
		// found
		return current;
	}

	// not found → create new
	Place *newPlace = new Place;
	newPlace->name = new char[strlen(name) + 1];
	strcpy(newPlace->name, name);
	newPlace->firstLink = NULL;
	newPlace->next = NULL;

	if (prev == NULL)
	{
		// list was empty → new head
		placeHead = newPlace;
	}
	else
	{
		prev->next = newPlace;
	}

	return newPlace;
}

void AddRoad(const char *from, const char *to, int cost)
{
	Place *fromPlace = GetOrCreatePlace(from);
	Place *toPlace = GetOrCreatePlace(to);

	RoadLink *newRoad = new RoadLink;
	newRoad->cost = cost;
	newRoad->to = toPlace;
	newRoad->next = NULL;

	fromPlace->firstLink = appendNodetoRoadList(fromPlace->firstLink, newRoad);
}

void printGraph()
{
    Place* p = placeHead;

    while (p != nullptr) {
        cout << "Place: " << p->name << endl;

        RoadLink* link = p->firstLink;

        if (link == nullptr) {
            cout << "  (no links)" << endl;
        } else {
            while (link != nullptr) {
                cout << "  -> " << link->to->name 
                     << " (cost: " << link->cost << ")" << endl;
                link = link->next;
            }
        }

        cout << endl;
        p = p->next;
    }
}


void loadRoadNetworkFromFile(fstream &roadFile)
{

	string line;

	while (getline(roadFile, line))
	{
		stringstream ss(line);
		char* from_place = new char[100];
		char* to_place = new char[100];
		int cost;
		ss >> from_place >> to_place >> cost;

		cout << "The cost to go from " << from_place << " to " << to_place << " is " << cost << endl;

		AddRoad(from_place, to_place, cost);
	}

}

User *userRoot = nullptr;
RideOffer *offerHead = nullptr;
RideRequest *requestHead = nullptr;

User *CreateUser(int userId,
				 const char *name,
				 int isDriver);
RideOffer *CreateRideOffer(int offerId, int driverId,
						   const char *start, const char *end,
						   int departTime, int capacity);
RideRequest *CreateRideRequest(int requestId, int passengerId,
							   const char *from, const char *to,
							   int earliest, int latest);
int MatchNextRequest();
void PrintUserHistory(int userId);
void PrintTopDrivers(int k);

int main()
{
	cout << "Hello world" << endl;

	fstream roadLinksFile;
	roadLinksFile.open("roads.txt");
	loadRoadNetworkFromFile(roadLinksFile);

	printGraph();

	return 0;
}
