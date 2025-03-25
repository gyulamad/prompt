#pragma once

#include <map>
#include <string>
#include <functional>

#include "Agent.hpp"
#include "Agency.hpp"

using namespace std;

namespace tools::agency {

    template<typename T>
    using AgentRoleMap = map<string, function<Agent<T>&(Agency<T>&, const string&)>>;

}