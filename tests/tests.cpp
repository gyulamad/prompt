#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "test_Agent.hpp"
#include "test_timef.hpp"
#include "test_JSON.hpp"

int main() {
    test_Agent();
    test_timef();
    test_JSON();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
