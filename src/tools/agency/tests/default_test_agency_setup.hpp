#pragma once

#include <string>

#include "../../utils/Owns.hpp"
#include "../PackQueue.hpp"
#include "../Worker.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::agency;

struct default_test_agency_setup {
    Owns owns;
    PackQueue<string> queue;
    Worker<string>* agency = nullptr;
    string worker_name = "test_worker";
    string agency_name = "agency";
    vector<string> recipients = {};
};