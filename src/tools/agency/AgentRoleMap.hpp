#pragma once

#include <map>
#include <string>
#include <functional>

#include "../utils/JSON.hpp"
#include "Agent.hpp"
// #include "Agency.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency {

    // template<typename T>
    using AgentInstantiator = function<void(
        //void*, // agency // TODO PackQueue???
        // const string&, // name
        // const vector<string>&, // recipients
        const JSON& // json
    )>;

    // template<typename T>
    using AgentRoleMap = map<string, AgentInstantiator>;

}