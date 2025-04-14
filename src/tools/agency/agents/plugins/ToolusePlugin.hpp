#pragma once

#include <string>

#include "../../chat/ChatPlugin.hpp"

using namespace std;
using namespace tools::agency::chat;

namespace tools::agency::agents::plugins {

    class ToolusePlugin: public ChatPlugin {
    public:
        ToolusePlugin(): ChatPlugin() {}
        virtual ~ToolusePlugin() {}

    };

}
