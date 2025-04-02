#pragma once

#include "PackQueue.hpp"

namespace tools::agency {

    // TODO: deprecated
    template<typename T>
    class PackQueueHolder { // TODO: once each agent get an agency reference this class may not needed anymore
    public:
        PackQueueHolder(PackQueue<T>& queue): queue(queue) {}
    // protected:
        PackQueue<T>& queue;
    };
    
}
