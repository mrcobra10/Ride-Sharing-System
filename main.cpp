// Ride Sharing System — Demo CLI (Phase 10 compatible)

#include <iostream>
#include <fstream>
#include <limits>
#include <string>

#include "roads.h"
#include "ride.h"
#include "user.h"
#include "storage.h"

using namespace std;

// --------- GLOBALS (declared extern in headers) ----------
Place *placeHead = nullptr;
User *userRoot = nullptr;
RideOffer *offerHead = nullptr;
RideRequest *requestHead = nullptr;
int requestCount = 0;

static void ClearCinLine()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static int ReadTime(const char *prompt)
{

    cin.ignore();
    while (true)
    {
        cout << prompt;
        string input;
        getline(cin, input);

        int hour, minute;
        char sep;

        // Use stringstream to parse "HH:MM"
        stringstream ss(input);
        if (ss >> hour >> sep >> minute && sep == ':' &&
            hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59)
        {
            return hour * 100 + minute;
        }
        ClearCinLine();

        cout << "Invalid time. Please enter in HH:MM 24-hour format.\n";
    }
}

static int ReadInt(const char *prompt)
{
    while (true)
    {
        cout << prompt;
        int x;
        if (cin >> x)
            return x;
        ClearCinLine();
        cout << "Invalid number, try again.\n";
    }
}

static string ReadToken(const char *prompt)
{
    cout << prompt;
    string s;
    cin >> s;
    return s;
}

static void ResetInMemoryState()
{
    // NOTE: This intentionally does not free all allocated memory (demo program).
    // It resets heads/counters so LoadAll() rebuilds cleanly.
    placeHead = nullptr;
    userRoot = nullptr;
    offerHead = nullptr;
    requestHead = nullptr;
    requestCount = 0;
    ClearActiveRides();
}

static void Menu()
{
    cout << "\n============================\n";
    cout << " Ride Sharing System (Demo)\n";
    cout << "============================\n";
    cout << "1) Load road network from roads.txt\n";
    cout << "2) Add road (from, to, cost)\n";
    cout << "3) Print road graph\n";
    cout << "4) Create user (driver/passenger)\n";
    cout << "5) Print all users (BST inorder)\n";
    cout << "6) Create ride offer\n";
    cout << "7) Print ride offers\n";
    cout << "8) Create ride request\n";
    cout << "9) Print ride requests\n";
    cout << "10) Match next request (creates active ride + history)\n";
    cout << "11) Print user ride history\n";
    cout << "12) Reachable areas within cost (from an offer)\n";
    cout << "13) Top-K drivers (by completed rides)\n";
    cout << "14) SAVE ALL\n";
    cout << "15) LOAD ALL\n";
    cout << "16) Reset in-memory state (for testing load)\n";
    cout << "0) Exit\n";
}

int main()
{
    cout << "Ride Sharing System — interactive demo\n";
    cout << "Tip: Create users first (drivers + passengers), then roads, then offers/requests.\n";

    while (true)
    {
        Menu();
        int choice = ReadInt("Select: ");

        if (choice == 0)
        {
            cout << "Goodbye.\n";
            break;
        }

        switch (choice)
        {
        case 1:
        {
            fstream f("roads.txt");
            if (!f.is_open())
            {
                cout << "Failed to open roads.txt\n";
                break;
            }
            loadRoadNetworkFromFile(f);
            cout << "Loaded road network from roads.txt\n";
            break;
        }
        case 2:
        {
            string from = ReadToken("From: ");
            string to = ReadToken("To: ");
            int cost = ReadInt("Cost: ");
            AddRoad(from.c_str(), to.c_str(), cost);
            cout << "Road added.\n";
            break;
        }
        case 3:
            printGraph();
            break;
        case 4:
        {
            int id = ReadInt("User ID (int): ");
            string name = ReadToken("Name (no spaces): ");
            int isDriver = ReadInt("Driver? (1=yes, 0=no): ");
            userRoot = CreateUser(userRoot, id, name.c_str(), isDriver);
            cout << "User created.\n";
            break;
        }
        case 5:
            PrintAllUsers(userRoot);
            break;
        case 6:
        {
            int offerId = ReadInt("Offer ID: ");
            int driverId = ReadInt("Driver ID: ");
            string start = ReadToken("Start place: ");
            string end = ReadToken("End place: ");
            int depart = ReadTime("Depart time (int): ");
            int cap = ReadInt("Capacity: ");
            RideOffer *o = CreateRideOffer(offerId, driverId, start.c_str(), end.c_str(), depart, cap);
            if (!o)
                cout << "Offer creation failed.\n";
            break;
        }
        case 7:
            PrintOffers();
            break;
        case 8:
        {
            int requestId = ReadInt("Request ID: ");
            int passengerId = ReadInt("Passenger ID: ");
            string from = ReadToken("From place: ");
            string to = ReadToken("To place: ");
            int earliest = ReadTime("Earliest depart time: ");
            int latest = ReadTime("Latest depart time: ");
            RideRequest *r = CreateRideRequest(requestId, passengerId, from.c_str(), to.c_str(), earliest, latest);
            if (!r)
                cout << "Request creation failed (passenger missing or invalid).\n";
            break;
        }
        case 9:
            PrintRequests();
            break;
        case 10:
        {
            int matched = MatchNextRequest();
            cout << (matched ? "Matched!\n" : "No match.\n");
            break;
        }
        case 11:
        {
            int userId = ReadInt("User ID: ");
            PrintUserHistory(userId);
            break;
        }
        case 12:
        {
            int offerId = ReadInt("Offer ID to use as start point: ");
            int bound = ReadInt("Cost bound: ");
            RideOffer *o = offerHead;
            while (o && o->offerId != offerId)
                o = o->next;
            if (!o)
            {
                cout << "Offer not found.\n";
                break;
            }
            PrintReachableWithinCost(o, bound);
            break;
        }
        case 13:
        {
            int k = ReadInt("K: ");
            PrintTopDrivers(k);
            break;
        }
        case 14:
        {
            bool ok = SaveAll(".");
            cout << (ok ? "Saved: users.dat, roads.dat, offers.dat, active_rides.dat, history.dat\n"
                        : "Save failed.\n");
            break;
        }
        case 15:
        {
            ResetInMemoryState();
            bool ok = LoadAll(".");
            cout << (ok ? "Loaded all data from .dat files.\n" : "Load failed.\n");
            break;
        }
        case 16:
            ResetInMemoryState();
            cout << "State reset.\n";
            break;
        default:
            cout << "Unknown option.\n";
            break;
        }
    }

    return 0;
}
