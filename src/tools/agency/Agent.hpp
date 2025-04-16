#pragma once

#include "../str/str_contains.hpp"
#include "../str/tpl_replace.hpp"
#include "../str/implode.hpp"
#include "../utils/system.hpp"
#include "../utils/ERROR.hpp"

#include "Worker.hpp"
#include "PackQueueHolder.hpp"

using namespace tools::str;
using namespace tools::utils;
using namespace tools::containers;

namespace tools::agency {

    template<typename T>
    class Agent: public Worker<T> {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:

        using Worker<T>::Worker;
        
        virtual ~Agent() {}
    };
    
}