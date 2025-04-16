#pragma once

#include <string>

#include "../../utils/Owns.hpp"
#include "../../utils/JSON.hpp"
#include "../../utils/ERROR.hpp"
#include "../PackQueue.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::agency;

struct default_test_agency_setup {
    Owns owns;
    PackQueue<string> queue;
    Worker<string>* agency = nullptr;
    string name;
    vector<string> recipients = {};
    map<string, function<void(const string& name, const JSON&)>> roles;
    JSON json;
    default_test_agency_setup(string name): name(name) {
        json.set("name", name);
        json.set("recipients", recipients);
    }
};