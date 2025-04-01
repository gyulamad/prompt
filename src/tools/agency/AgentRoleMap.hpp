#pragma once

#include <map>
#include <string>
#include <functional>

#include "Agent.hpp"
#include "agents/Agency.hpp"

using namespace std;
using namespace tools::agency::agents;

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