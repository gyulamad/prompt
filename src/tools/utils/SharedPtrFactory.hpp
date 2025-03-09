#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <string>
#include <thread>
#include <atomic>

#include "ERROR.hpp"

using namespace std;

namespace tools::utils {

    #define SP_CREATE(factory, type, ...) \
        (factory).create<type>(__FILE__, __LINE__, ##__VA_ARGS__)
    
    class SharedPtrFactory {
    public:
        explicit SharedPtrFactory() {}
    
        template<typename T, typename... Args>
        shared_ptr<T> create(const char* file, int line, Args&&... args) {
            lock_guard<mutex> lock(mtx); // mtx is a member mutex
            auto ptr = make_shared<T>(forward<Args>(args)...);
            pointers.push_back(PointerEntry{ptr, file, line, typeid(T).name()});
            return ptr;
        }

        ~SharedPtrFactory() noexcept(false) {
            lock_guard<mutex> lock(mtx);
            string errs = "";
            for (const auto& entry : pointers)
                if (entry.ptr.use_count() > 1) {
                    if (errs.empty()) errs = "SharedPtrFactory destruction failed due to unreleased shared_ptrs:\n";
                    errs += " - Pointer at " + to_string(reinterpret_cast<uintptr_t>(entry.ptr.get())) +
                            ", use_count = " + to_string(entry.ptr.use_count()) +
                            ", type = " + entry.type +
                            ", created at " + entry.file + ":" + to_string(entry.line) + "\n";
                }
                
            if (!errs.empty()) throw ERROR(errs);
        }
    
    private:
        struct PointerEntry {
            shared_ptr<void> ptr;
            string file;
            int line;
            string type;
    
            PointerEntry(shared_ptr<void> p, const char* f, int l, const char* t)
                : ptr(p), file(f), line(l), type(t) {}
        };
    
        vector<PointerEntry> pointers;
        mutex mtx;
        bool strict;
    };
    
} // namespace tools::utils

#ifdef TEST

#include "Test.hpp"

#include "tests/TestObj.hpp"

using namespace tools::utils;

// Positive case: Create and destroy with no external holders (strict mode)
void test_SharedPtrFactory_create_clean_destruction_strict() {
    SharedPtrFactory factory;  // Strict mode
    shared_ptr<TestObj> ptr = SP_CREATE(factory, TestObj, "A");
    string actual = ptr->getId();
    assert(actual == "A" && "SharedPtrFactory should create TestObj with correct ID");
    // Factory destructs here, no external holders, no throw expected
}

// Positive case: Multiple creations, all released cleanly
void test_SharedPtrFactory_create_multiple_clean() {
    SharedPtrFactory factory;
    auto ptr1 = SP_CREATE(factory, TestObj, "C");
    auto ptr2 = SP_CREATE(factory, TestObj, "D");
    auto id1 = ptr1->getId();
    auto id2 = ptr2->getId();
    assert(id1 == "C" && "First TestObj ID should match");
    assert(id2 == "D" && "Second TestObj ID should match");
    // Factory destructs, no external holders, no throw
}

// Negative case: External holder causes throw in strict mode
void test_SharedPtrFactory_create_strict_with_holder() {
    static shared_ptr<TestObj> sneaky;  // External holder
    bool thrown = false;
    string what;

    try {
        SharedPtrFactory factory;  // Strict mode
        auto ptr = SP_CREATE(factory, TestObj, "E");
        sneaky = ptr;  // Create external reference
        // Factory destructs here, should throw
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "SharedPtrFactory destruction failed") && "Exception message should indicate factory failure");
        assert(str_contains(what, "TestObj") && "Exception should mention TestObj type");
        assert(str_contains(what, "use_count = 2") && "Exception should report correct use_count");
        assert(str_contains(what, __FILE__) && "Exception should include source file");
    }
    assert(thrown && "test_SharedPtrFactory_create_strict_with_holder should throw due to unreleased shared_ptr");
}

// Negative case: Multiple pointers, one held externally
void test_SharedPtrFactory_create_multiple_one_held() {
    static shared_ptr<TestObj> sneaky;
    bool thrown = false;
    string what;

    try {
        SharedPtrFactory factory;
        auto ptr1 = SP_CREATE(factory, TestObj, "G");
        auto ptr2 = SP_CREATE(factory, TestObj, "H");
        sneaky = ptr1;  // Hold only one
        // Factory destructs, should throw for ptr1
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "SharedPtrFactory destruction failed") && "Exception should indicate failure");
        assert(str_contains(what, "use_count = 2") && "Exception should report use_count of held pointer");
        assert(str_contains(what, "TestObj") && "Exception should mention TestObj type");
        assert(!str_contains(what, "H") && "Exception should not mention unheld TestObj 'H'");
    }
    assert(thrown && "test_SharedPtrFactory_create_multiple_one_held should throw due to one unreleased shared_ptr");
}

void test_SharedPtrFactory_create_strict_stderr_output() {
    static shared_ptr<TestObj> sneaky;
    string output;
    try {
        SharedPtrFactory factory;
        auto ptr = SP_CREATE(factory, TestObj, "I");
        sneaky = ptr;
    } catch (const runtime_error& e) {
        output = e.what(); // Get the exception message
    }
    cout << "Captured exception message: [" << output << "]\n"; // Breakpoint here will be reached
    assert(str_contains(output, "SharedPtrFactory destruction failed") && "Exception should contain factory failure message");
    assert(str_contains(output, "use_count = 2") && "Exception should show use_count");
    assert(str_contains(output, __FILE__) && "Exception should include source file");
}

void test_SharedPtrFactory_concurrent_create_strict_with_holders() {
    SharedPtrFactory factory; // Strict mode enabled
    vector<thread> threads;
    vector<shared_ptr<TestObj>> holders;
    mutex holders_mutex; // Declare the mutex here

    // Lambda to capture stderr or perform concurrent operations
    auto concurrent_task = [&]() {
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&factory, &holders, &holders_mutex, i]() {
                // Create a shared pointer using the factory
                auto ptr = SP_CREATE(factory, TestObj, string("ThreadObj") + to_string(i));
                {
                    // Lock the mutex to safely modify the holders vector
                    lock_guard<mutex> lock(holders_mutex);
                    holders.push_back(ptr);
                }
            });
        }
        // Join all threads
        for (auto& t : threads) {
            t.join();
        }
    };

    // Execute the concurrent task (assuming capture_cerr runs the lambda)
    string output = capture_cerr(concurrent_task);

    // Example assertions (adjust based on your test requirements)
    assert(holders.size() == 5 && "All objects should be created");
    // Additional checks on output if needed
}

// Register tests
TEST(test_SharedPtrFactory_create_clean_destruction_strict);
TEST(test_SharedPtrFactory_create_multiple_clean);
TEST(test_SharedPtrFactory_create_strict_with_holder);
TEST(test_SharedPtrFactory_create_multiple_one_held);
TEST(test_SharedPtrFactory_create_strict_stderr_output);
TEST(test_SharedPtrFactory_concurrent_create_strict_with_holders);

#endif