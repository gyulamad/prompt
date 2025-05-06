#pragma once

#include "Owns.hpp"

namespace tools::utils {
    
    class OList {
    public:
        OList(Owns& owns): owns(owns) {}
        
        virtual ~OList() {
            for (void* plug: plugs)
                owns.release(this, plug);
        }

        vector<void*> getPlugs() const { return plugs; }

        template<typename T>
        void push(void* plug) {
            owns.reserve<T>(this, plug, FILELN);
            this->plugs.push_back(plug);
        }
        
    private:
        Owns& owns;
        vector<void*> plugs;
    };
    
}

#ifdef TEST

#include "../str/str_contains.h"
#include "tests/OwnsSpy.hpp"

using namespace tools::utils;

// Test: Constructor initializes OList with Owns reference
void test_OList_constructor_basic() {
    Owns owns;
    OList olist(owns);
    vector<void*> plugs = olist.getPlugs();
    assert(plugs.empty() && "New OList should have empty plugs");
}

// Test: Destructor releases all plugs
void test_OList_destructor_releases_plugs() {
    OwnsSpy owns;
    int* ptr = owns.allocate<int>(42);
    void* plug = ptr;
    {
        OList olist(owns);
        olist.push<int>(plug);
        unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
        assert(reserves.count(plug) == 1 && "Plug should be reserved before destructor");
    }
    unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
    assert(reserves.empty() && "Destructor failed to release plugs");
}

// Test: Push a single valid pointer
void test_OList_push_single() {
    OwnsSpy owns;
    OList olist(owns);
    int* ptr = owns.allocate<int>(42);
    olist.push<int>(ptr);
    vector<void*> plugs = olist.getPlugs();
    unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
    assert(plugs.size() == 1 && "Push failed to add plug");
    assert(plugs[0] == ptr && "Pushed plug does not match");
    assert(reserves.count(ptr) == 1 && "Push failed to reserve pointer");
    assert(reserves[ptr].owners.count(&olist) == 1 && "OList not registered as owner");
}

// Test: Push multiple valid pointers
void test_OList_push_multiple() {
    OwnsSpy owns;
    OList olist(owns);
    int* ptr1 = owns.allocate<int>(42);
    int* ptr2 = owns.allocate<int>(43);
    olist.push<int>(ptr1);
    olist.push<int>(ptr2);
    vector<void*> plugs = olist.getPlugs();
    unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
    assert(plugs.size() == 2 && "Multiple pushes failed to add plugs");
    assert(plugs[0] == ptr1 && plugs[1] == ptr2 && "Pushed plugs order or content mismatch");
    assert(reserves.count(ptr1) == 1 && reserves.count(ptr2) == 1 && "Pushes failed to reserve pointers");
    assert(reserves[ptr1].owners.count(&olist) == 1 && reserves[ptr2].owners.count(&olist) == 1 && "OList not registered as owner for all plugs");
}

// Test: Push unallocated pointer throws exception
void test_OList_push_unallocated_throws() {
    Owns owns;
    OList olist(owns);
    int* ptr = new int(42); // Not allocated by Owns
    bool thrown = false;
    string what;
    try {
        olist.push<int>(ptr);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Cannot reserve unallocated pointer") && "Exception message incorrect for unallocated pointer");
    }
    assert(thrown && "Push should throw for unallocated pointer");
    assert(olist.getPlugs().empty() && "Unallocated push should not add plug");
    delete ptr;
    ptr = nullptr;
}

// Test: getPlugs returns empty vector for empty OList
void test_OList_getPlugs_empty() {
    Owns owns;
    OList olist(owns);
    vector<void*> plugs = olist.getPlugs();
    assert(plugs.empty() && "Empty OList should return empty plugs vector");
}

// Test: getPlugs returns single plug correctly
void test_OList_getPlugs_single_plug() {
    Owns owns;
    OList olist(owns);
    int* ptr = owns.allocate<int>(42);
    olist.push<int>(ptr);
    vector<void*> plugs = olist.getPlugs();
    assert(plugs.size() == 1 && "Single plug not returned");
    assert(plugs[0] == ptr && "Returned plug does not match pushed pointer");
}

// Test: getPlugs returns multiple plugs in correct order
void test_OList_getPlugs_multiple_plugs() {
    Owns owns;
    OList olist(owns);
    int* ptr1 = owns.allocate<int>(42);
    int* ptr2 = owns.allocate<int>(43);
    olist.push<int>(ptr1);
    olist.push<int>(ptr2);
    vector<void*> plugs = olist.getPlugs();
    assert(plugs.size() == 2 && "Multiple plugs count mismatch");
    assert(plugs[0] == ptr1 && plugs[1] == ptr2 && "Plugs order or content mismatch");
}

// Register all tests
TEST(test_OList_constructor_basic);
TEST(test_OList_destructor_releases_plugs);
TEST(test_OList_push_single);
TEST(test_OList_push_multiple);
TEST(test_OList_push_unallocated_throws);
TEST(test_OList_getPlugs_empty);
TEST(test_OList_getPlugs_single_plug);
TEST(test_OList_getPlugs_multiple_plugs);

#endif