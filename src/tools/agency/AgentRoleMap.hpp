#pragma once

#include <map>
#include <string>
#include <functional>

#include "Agent.hpp"
#include "Agency.hpp"

using namespace std;

namespace tools::agency {

    template<typename T>
    using AgentInstantiator = function<Agent<T>&(
        Agency<T>&, // agency // TODO PackQueue???
        const string&, // name
        const vector<string>& // recipients
    )>;

    template<typename T>
    using AgentRoleMap = map<string, AgentInstantiator<T>>;

}