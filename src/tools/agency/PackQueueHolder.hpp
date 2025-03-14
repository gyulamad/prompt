#pragma once

#include "PackQueue.hpp"

namespace tools::agency {

    template<typename T>
    class PackQueueHolder {
    public:
        PackQueueHolder(PackQueue<T>& queue): queue(queue) {}
    protected:
        PackQueue<T>& queue;
    };
    
}
