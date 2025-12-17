// Ride Sharing Project
// Usman Joined

#include <iostream>
#include <fstream>
#include <sstream>
#include "roads.h"
#include "ride.h"
using namespace std;


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




User *userRoot = nullptr;
RideOffer *offerHead = nullptr;
RideRequest *requestHead = nullptr;

User *CreateUser(int userId,
				 const char *name,
				 int isDriver);

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

	cout << "===== TEST START =====";


// Build road graph
AddRoad("A", "B", 5);
AddRoad("A", "C", 10);
AddRoad("B", "C", 3);
AddRoad("C", "D", 4);
AddRoad("B", "D", 8);


// Create ride offers
CreateRideOffer(1, 101, "A", "D", 10, 2);
CreateRideOffer(2, 102, "A", "C", 12, 1);


cout << "--- Offers After Creation ---";
PrintOffers();


// Create ride requests
CreateRideRequest(1, 201, "A", "D", 9, 11);
CreateRideRequest(2, 202, "A", "D", 10, 12);
CreateRideRequest(3, 203, "A", "C", 11, 13);


cout << "--- Requests After Creation ---";
PrintRequests();


// Test matching
cout << "--- Matching Requests ---";
cout << "Match 1: " << MatchNextRequest() << endl; // expect 1
cout << "Match 2: " << MatchNextRequest() << endl; // expect 1
cout << "Match 3: " << MatchNextRequest() << endl; // expect 1
cout << "Match 4: " << MatchNextRequest() << endl; // expect 0


cout << "--- Offers After Matching ---";
PrintOffers();


// Test reachable areas (Dijkstra)
cout << "-- Reachable Areas Test ---";
PrintReachableWithinCost(offerHead, 15);


cout << "===== TEST END =====";



	return 0;
}
