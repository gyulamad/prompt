#pragma once

#include <mutex>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <stdexcept>
#include <unordered_set>

#include "../str/implode.hpp"
#include "ERROR.hpp"
#include "foreach.hpp"

using namespace std;
using namespace tools::str;

namespace tools::utils {

    class Owns { // TODO: make it thread safe
    public:
        Owns() {}

        virtual ~Owns() {
            cleanup();
        }

        template<typename T, typename... Args>
        T* allocate(Args&&... args) {
            T* pointer = new T(/*forward<Args>(args)*/args...);
            reserves[(void*)pointer].deleter = [](void* p) { 
                delete static_cast<T*>(p); // TODO: delete pointer? (from lambda [&])
                p = nullptr; 
            };
            return pointer;
        }

        // TODO: shall we be able to adapt non managed pointers? (configurable??)
        template<typename T>
        T* reserve(void* owner, void* pointer, string dbgnfo /*= "<untracked>"*/) {
            if (dbgnfo.empty()) dbgnfo = "<untracked>";
            if (!managed(pointer))
                throw ERROR("Cannot reserve unallocated pointer: " + dbgnfo);
            reserves[pointer].owners.insert(owner);
            reserves[pointer].dbgnfos.push_back("Reserved at " + dbgnfo);
            return (T*)pointer;
        }

        void release(void* owner, void* pointer) {
            if (!managed(pointer))
                throw ERROR("Cannot release unallocated pointer");
            auto& at = reserves.at(pointer);
            unordered_set<void*>& owners = at.owners;
            owners.erase(owner);
            if (!owners.empty()) return;
            at.deleter(pointer);
            reserves.erase(pointer);
        }

        struct ownnfo {
            function<void(void*)> deleter;
            unordered_set<void*> owners;
            vector<string> dbgnfos;
        };

        unordered_map<
            void*, 
            ownnfo
            // pair<
            //     function<void(void*)>, 
            //     unordered_set<void*>
            // >
        > reserves; // TODO: back to protected, it's just for debug!!

    protected:
        void cleanup() {
            for (auto& [ptr, data] : reserves) {
                string nfo = "Unrelesed pointer by " 
                    + to_string(data.owners.size()) 
                    + " owner(s) detected:\n";
                if (!data.owners.empty())
                    cerr << ERROR(nfo + implode("\n", data.dbgnfos)).what() << endl;
                else data.deleter(ptr);
            }
            reserves.clear();
        }

        bool managed(void* pointer) {
            if (reserves.find(pointer) == reserves.end()) return false;
            return true;
        }
    };

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

} // namespace tools::utils

#ifdef TEST

#include "../str/str_contains.hpp"
#include "tests/OwnsSpy.hpp"

using namespace tools::utils;

void test_Owns_allocate_basic() {
    Owns owns;
    // void* owner = reinterpret_cast<void*>(1);
    int* ptr = owns.allocate<int>(42);
    assert(ptr != nullptr && "Allocation failed");
    assert(*ptr == 42 && "Constructor args not forwarded");
}

// Test: Basic reserve functionality after allocation
void test_Owns_reserve_basic() {
    Owns owns;
    void* owner1 = reinterpret_cast<void*>(1);
    void* owner2 = reinterpret_cast<void*>(2);
    int* ptr = owns.allocate<int>(42);
    owns.reserve<void>(owner1, ptr, FILELN);
    unordered_map<void*, Owns::ownnfo> reserves = static_cast<OwnsSpy&>(owns).getReserves();
    bool is_tracked = reserves.count(ptr) == 1;
    assert(is_tracked && "Reserve failed to track after allocation");
    owns.reserve<void>(owner2, ptr, FILELN);
    reserves = static_cast<OwnsSpy&>(owns).getReserves(); // Refresh the copy after second reserve
    int owner_count = reserves[ptr].owners.size();
    assert(owner_count == 2 && "Owner count mismatch after second reserve");
    owns.release(owner1, ptr);
    owns.release(owner2, ptr);
}

// Test: Reserve throws exception for unallocated pointer
void test_Owns_reserve_unallocated() {
    Owns owns;
    void* owner = reinterpret_cast<void*>(1);
    int* ptr = new int(42); // Not allocated by Owns
    bool thrown = false;
    string what;
    try {
        owns.reserve<void>(owner, ptr, FILELN);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Cannot reserve unallocated pointer") && "Exception message incorrect for unallocated pointer");
    }
    assert(thrown && "Reserve should throw for unallocated pointer but didn’t");
    delete ptr;
    ptr = nullptr;
}

void test_Owns_release_single_owner() {
    OwnsSpy owns;
    void* owner = reinterpret_cast<void*>(1);
    int* ptr = owns.allocate<int>(42);
    owns.reserve<void>(owner, ptr, FILELN);
    
    owns.release(owner, ptr);
    assert(owns.getReserves().empty() && "Reserve not cleaned up");
}

void test_Owns_release_multi_owner() {
    OwnsSpy owns;
    void* owner1 = reinterpret_cast<void*>(1);
    void* owner2 = reinterpret_cast<void*>(2);
    int* ptr = owns.allocate<int>(42);
    
    owns.reserve<void>(owner1, ptr, FILELN);
    owns.reserve<void>(owner2, ptr, FILELN);
    
    owns.release(owner1, ptr);
    unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
    int owner_count = reserves[ptr].owners.size();
    assert(owner_count == 1 && "Owner count mismatch after first release");
    
    owns.release(owner2, ptr);
    assert(owns.getReserves().empty() && "Reserves not empty after final release");
}

void test_Owns_cleanup() {
    OwnsSpy owns;
    void* owner = reinterpret_cast<void*>(1);
    int* ptr = owns.allocate<int>(42);
    owns.reserve<void>(owner, ptr, FILELN);
    
    // Before cleanup, reserves should have entries
    bool has_entries = !owns.getReserves().empty();
    assert(has_entries && "Reserves should have entries before cleanup");
    
    string err = capture_cerr([&]() {
        // Perform cleanup
        owns.publicCleanup();
    });
    
    // After cleanup, reserves should be empty
    bool cleanup_happened = owns.getReserves().empty();
    assert(cleanup_happened && "Destructor didn't cleanup"); // Message kept for consistency
    assert(str_contains(err, "Unrelesed pointer by 1 owner(s) detected"));
}

void test_Owns_reserve_unmanaged_throws() {
    Owns owns;
    void* owner = reinterpret_cast<void*>(1);
    int* ptr = new int(42);
    
    bool thrown = false;
    string what;
    try {
        owns.reserve<void>(owner, ptr, FILELN);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Cannot reserve unallocated pointer") && "Wrong exception message");
    }
    assert(thrown && "Should throw for unmanaged pointer");
    
    delete ptr;
    ptr = nullptr; // Per your conventions
}

void test_Owns_release_unmanaged_throws() {
    Owns owns;
    void* owner = reinterpret_cast<void*>(1);
    int* ptr = new int(42);
    
    bool thrown = false;
    string what;
    try {
        owns.release(owner, ptr);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Cannot release unallocated pointer") && "Wrong exception message");
    }
    assert(thrown && "Should throw for unmanaged pointer");
    
    delete ptr;
    ptr = nullptr;
}

void test_Owns_multi_owner_release() {
    OwnsSpy owns;
    void* owner1 = reinterpret_cast<void*>(1);
    void* owner2 = reinterpret_cast<void*>(2);
    int* ptr = owns.allocate<int>(42);
    
    owns.reserve<void>(owner1, ptr, FILELN);
    owns.reserve<void>(owner2, ptr, FILELN);
    
    owns.release(owner1, ptr);
    unordered_map<void*, Owns::ownnfo> reserves = owns.getReserves();
    int owner_count = reserves[ptr].owners.size();
    assert(owner_count == 1 && "Owner count mismatch after first release");
    
    owns.release(owner2, ptr);
    assert(owns.getReserves().empty() && "Reserves not empty after final release");
}

void test_Owns_multi_owner_reserve() {
    int owner_count = 0;

    string err = capture_cerr([&]() {
        Owns owns;
        void* owner1 = reinterpret_cast<void*>(1);
        void* owner2 = reinterpret_cast<void*>(2);
        int* ptr = owns.allocate<int>(42);
    
        owns.reserve<void>(owner1, ptr, FILELN);
        owns.reserve<void>(owner2, ptr, FILELN); // Should not throw

        unordered_map<void*, Owns::ownnfo> reserves = static_cast<OwnsSpy&>(owns).getReserves();
        owner_count = reserves[ptr].owners.size();
    });
    
    assert(owner_count == 2 && "Multiple owners not registered");
    assert(str_contains(err, "Unrelesed pointer by 2 owner(s) detected"));
}

// Register all tests
TEST(test_Owns_allocate_basic);
TEST(test_Owns_reserve_basic);
TEST(test_Owns_reserve_unallocated);
TEST(test_Owns_release_single_owner);
TEST(test_Owns_release_multi_owner);
TEST(test_Owns_cleanup);
TEST(test_Owns_reserve_unmanaged_throws);
TEST(test_Owns_release_unmanaged_throws);
TEST(test_Owns_multi_owner_release);
TEST(test_Owns_multi_owner_reserve);
#endif
