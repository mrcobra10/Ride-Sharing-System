// Ride Sharing Project
// Usman Joined

#include <iostream>
#include <fstream>
#include <sstream>
#include "user.h"
//#include <ctring>

using namespace std;


User* CreateUser(User* root, int userId, const char *name, int isDriver) {
    if (!root) {
        User* newUser = new User;
        newUser->userId = userId;
        newUser->name = new char[strlen(name)+1];
        strcpy(newUser->name, name);
        newUser->isDriver = isDriver;
        newUser->rating = 5; // default rating
        newUser->history = nullptr;
        newUser->left = newUser->right = nullptr;
        return newUser;
    }

    if (userId < root->userId) {
        root->left = CreateUser(root->left, userId, name, isDriver);
    } else {
        root->right = CreateUser(root->right, userId, name, isDriver);
    }

    return root;
}

// Search user by ID
User* SearchUser(User* root, int userId) {
    if (!root)
	 return nullptr;
    if (root->userId == userId)
	 return root;
    if (userId < root->userId)
	 return SearchUser(root->left, userId);
    else
	 return SearchUser(root->right, userId);
}

void PrintUser(User* u) {
    if (!u) return;
    cout << "ID: " << u->userId
         << " | Name: " << u->name
         << " | ";
    if (u->isDriver == 1)
        cout << "Driver";
    else
        cout << "Passenger";
    cout << " | Rating: " << u->rating << endl;
}

// Inorder print all users
void PrintAllUsers(User* root) {
    if (!root) return;
    PrintAllUsers(root->left);
    PrintUser(root);
    PrintAllUsers(root->right);
}

HistoryNode* InsertHistoryBST(HistoryNode* root, int rideId,
                              const char* from, const char* to, int time)
{
    if (root == nullptr) {
        // Create a new node
        HistoryNode* newNode = new HistoryNode;
        newNode->rideId = rideId;

        newNode->from = new char[strlen(from)+1];
        strcpy(newNode->from, from);

        newNode->to = new char[strlen(to)+1];
        strcpy(newNode->to, to);

        newNode->time = time;
        newNode->left = newNode->right = nullptr;

        return newNode;
    }

    // Compare by time
    if (time < root->time) {
        root->left = InsertHistoryBST(root->left, rideId, from, to, time);
    } else {
        root->right = InsertHistoryBST(root->right, rideId, from, to, time);
    }

    return root;
}

void AddHistory(int userId, int rideId,
                const char* from, const char* to, int time)
{
    User* u = SearchUser(userRoot, userId);
    if (!u) {
        cout << "User not found!" << endl;
        return;
    }

    u->history = InsertHistoryBST(u->history, rideId, from, to, time);
}

void PrintHistoryBST(HistoryNode* root)
{
    if (!root) return;

    PrintHistoryBST(root->left);

    cout << "  Ride ID: " << root->rideId
         << " | From: " << root->from
         << " | To: " << root->to
         << " | Time: " << root->time << endl;

    PrintHistoryBST(root->right);
}

void PrintUserHistory(int userId)
{
    User* u = SearchUser(userRoot, userId);
    if (!u) {
        cout << "User not found!" << endl;
        return;
    }

    cout << "Ride History of " << u->name
         << " (ID " << u->userId << "):" << endl;

    if (!u->history) {
        cout << "  No ride history available." << endl;
        return;
    }

    PrintHistoryBST(u->history);
}

bool PassengerExists(int passengerId)
{
    User* u = SearchUser(userRoot, passengerId);

    if (u == nullptr)
        return false;

    // passenger = isDriver == 0
    return (u->isDriver == 0);
}


