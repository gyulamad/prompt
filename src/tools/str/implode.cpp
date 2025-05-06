#include "implode.h"
#include <sstream>

using namespace std;

namespace tools::str {

    string implode(const string& delimiter, const vector<string>& elements) {
        ostringstream oss;
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i != 0) oss << delimiter;
            oss << elements[i];
        }
        return oss.str();
    }

} // namespace tools::str

#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

using namespace tools::str;

void test_implode_basic() {
    vector<string> parts = {"a", "b", "c"};
    string result = implode(",", parts);
    assert(result == "a,b,c" && "Basic implode");
}

void test_implode_single_element() {
    vector<string> parts = {"hello"};
    string result = implode("-", parts);
    assert(result == "hello" && "Single element");
}

void test_implode_empty_elements() {
    vector<string> parts = {"", "", ""};
    string result = implode(":", parts);
    assert(result == "::" && "Empty elements");
}

void test_implode_empty_delimiter() {
    vector<string> parts = {"a", "b", "c"};
    string result = implode("", parts);
    assert(result == "abc" && "Empty delimiter");
}

void test_implode_empty_vector() {
    vector<string> parts = {};
    string result = implode(",", parts);
    assert(result.empty() && "Empty vector");
}

void test_implode_mixed_elements() {
    vector<string> parts = {"a", "", "c"};
    string result = implode("-", parts);
    assert(result == "a--c" && "Mixed elements");
}

TEST(test_implode_basic);
TEST(test_implode_single_element);
TEST(test_implode_empty_elements);
TEST(test_implode_empty_delimiter);
TEST(test_implode_empty_vector);
TEST(test_implode_mixed_elements);
#endif // TEST