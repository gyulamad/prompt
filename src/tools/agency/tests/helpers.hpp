#pragma once

#include <vector>

#include "../Pack.hpp"

using namespace std;
using namespace tools::agency;

// Helper to get queue contents
template<typename T>
vector<Pack<T>> queue_to_vector(PackQueue<T>& pq) {
    vector<Pack<T>> result;
    Pack<T> pack;
    while (pq.Consume(pack)) {
        result.push_back(move(pack));
    }
    return result;
};