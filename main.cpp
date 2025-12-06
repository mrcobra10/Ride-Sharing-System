// Ride Sharing Project
// Usman Joined

#include <iostream>
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

int main()
{
	cout << "Hello world" << endl;

	return 0;
}
