#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>   
#include <vector>     
#include "BST.h"      
#include "Record.h"
//add header files as needed

using namespace std;

// Converts a string to lowercase (used for case-insensitive searches)
static inline string toLower(string s) {
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// ================== Index Engine ==================
// Acts like a small "database engine" that manages records and two BST indexes:
// 1) idIndex: maps student_id → record index (unique key)
// 2) lastIndex: maps lowercase(last_name) → list of record indices (non-unique key)
struct Engine {
    vector<Record> heap;                  // the main data store (simulates a heap file)
    BST<int, int> idIndex;                // index by student ID
    BST<string, vector<int>> lastIndex;   // index by last name (can have duplicates)

    // Inserts a new record and updates both indexes.
    // Returns the record ID (RID) in the heap.
    int insertRecord(const Record &recIn) {
        int rid = static_cast<int>(heap.size());

        Record rec = recIn;
        rec.deleted = false;
        heap.push_back(rec);

        idIndex.insert(rec.id, rid);

        string key = toLower(rec.last);
        vector<int> *vecPtr = lastIndex.find(key);
        if (!vecPtr) {
            vector<int> v;
            v.push_back(rid);
            lastIndex.insert(key, v);
        } else {
            vecPtr->push_back(rid);      
        }

        return rid;
    }

    // Deletes a record logically (marks as deleted and updates indexes)
    // Returns true if deletion succeeded.
    bool deleteById(int id) {

        int *posPtr = idIndex.find(id);
        if (!posPtr) return false;

        int rid = *posPtr;
        if (rid < 0 || rid >= static_cast<int>(heap.size()))
            return false;

        if (heap[rid].deleted)
            return false;

        heap[rid].deleted = true;

        idIndex.erase(id);

        string key = toLower(heap[rid].last);
        vector<int> *vecPtr = lastIndex.find(key);
        if (vecPtr) {
            vector<int> &v = *vecPtr;
            
            for (size_t i = 0; i < v.size(); ++i) {
                if (v[i] == rid) {
                    v.erase(v.begin() + static_cast<long>(i));
                    break;
                }
            }
            if (v.empty()) {
                lastIndex.erase(key);
            }
        }

        return true;
    }

    // Finds a record by student ID.
    // Returns a pointer to the record, or nullptr if not found.
    // Outputs the number of comparisons made in the search.
    const Record *findById(int id, int &cmpOut) {
        idIndex.resetMetrics();
        int *posPtr = idIndex.find(id);

        cmpOut = idIndex.comparisons;

        if (!posPtr) return nullptr;

        int rid = *posPtr;
        if (rid < 0 || rid >= static_cast<int>(heap.size()))
            return nullptr;

        if (heap[rid].deleted)
            return nullptr;

        return &heap[rid];
    }

    // Returns all records with ID in the range [lo, hi].
    // Also reports the number of key comparisons performed.
    vector<const Record *> rangeById(int lo, int hi, int &cmpOut) {
        vector<const Record *> out;

        idIndex.resetMetrics();
        idIndex.rangeApply(lo, hi, [&](const int &k, int &rid) {
            
            if (rid >= 0 &&
                rid < static_cast<int>(heap.size()) &&
                !heap[rid].deleted) {
                out.push_back(&heap[rid]);
            }
        });
        cmpOut = idIndex.comparisons;

        return out;
    }

    // Returns all records whose last name begins with a given prefix.
    // Case-insensitive using lowercase comparison.
    vector<const Record *> prefixByLast(const string &prefix, int &cmpOut) {
        vector<const Record *> out;

        string low = toLower(prefix);
        
        string high = low;
        high.push_back(char(127)); 

        lastIndex.resetMetrics();
        lastIndex.rangeApply(low, high, [&](const string &key, vector<int> &rids) {
            
            if (key.size() < low.size()) return;
            for (size_t i = 0; i < low.size(); ++i) {
                if (key[i] != low[i]) return;
            }

            for (int rid : rids) {
                if (rid >= 0 &&
                    rid < static_cast<int>(heap.size()) &&
                    !heap[rid].deleted) {
                    out.push_back(&heap[rid]);
                }
            }
        });
        cmpOut = lastIndex.comparisons;

        return out;
    }
};

#endif
