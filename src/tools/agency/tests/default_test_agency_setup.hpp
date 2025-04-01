#pragma once

#include <string>

#include "../../utils/Factory.hpp"
#include "../PackQueue.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::agency;

struct default_test_agency_setup {
    Owns owns;
    PackQueue<string> queue;
    void* agency = nullptr;
};