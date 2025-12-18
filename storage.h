#ifndef STORAGE_H
#define STORAGE_H

// Phase 10 — File Storage
// Save order:
//   Users (BST traversal)
//   Places & roads (list traversal)
//   Ride offers (linked list)
//   Active rides (hash table)
//   History (per-user lists/BST)
//
// Load order: same as save; rebuild all data structures.

// Moved from roads.cpp (Step 10.2 requirement)
#include <fstream>
bool loadRoadNetworkFromFile(std::fstream &roadFile);

bool SaveAll(const char* baseDir = ".");
bool LoadAll(const char* baseDir = ".");

#endif


