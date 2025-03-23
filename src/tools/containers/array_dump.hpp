#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template<typename T>
    void array_dump(vector<T> vec, bool dbg = true) {
        if (dbg) DEBUG("dump vector(" + to_string(vec.size()) + "):");
        size_t nth = 0;
        for (const T& elem: vec) cout << nth++ << ": " << elem << endl;
    }
    
}

#ifdef TEST

using namespace tools::containers;

void test_array_dump_basic() {
    vector<int> vec = {1, 2, 3};
    string expected = 
        "dump vector(3):\n"
        "0: 1\n"
        "1: 2\n"
        "2: 3\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Basic dump");
}

void test_array_dump_empty_vector() {
    vector<int> vec;
    string expected = 
        "dump vector(0):\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Empty vector dump");
}

void test_array_dump_strings() {
    vector<string> vec = {"hello", "world"};
    string expected = 
        "dump vector(2):\n"
        "0: hello\n"
        "1: world\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Strings dump");
}

void test_array_dump_no_debug() {
    vector<int> vec = {1, 2, 3};
    string expected = 
        "0: 1\n"
        "1: 2\n"
        "2: 3\n";
    
    string output = capture_cout([&]() { array_dump(vec, false); });
    assert(output == expected && "No debug dump");
}

TEST(test_array_dump_basic);
TEST(test_array_dump_empty_vector);
TEST(test_array_dump_strings);
TEST(test_array_dump_no_debug);
#endif
