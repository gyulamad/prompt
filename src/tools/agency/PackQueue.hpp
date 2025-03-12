#pragma once

#include "../../../libs/K-Adam/SafeQueue/SafeQueue.hpp"

#include "Pack.hpp"

using namespace std;

namespace tools::agency {

    template<typename T>
    class PackQueue: public SafeQueue<Pack<T>> {
    public:

        void drop(const string& recipient) {
            unique_lock<mutex> lock(this->mtx); // Access the mutex from SafeQueue

            queue<Pack<T>> temp; // Temporary queue for filtered items
            while (!this->q.empty()) {
                Pack<T> pack = move(this->q.front());
                this->q.pop();
                if (pack.getRecipient() != recipient) {
                    temp.push(move(pack)); // Keep packs that don't match
                }
            }
            this->q = move(temp); // Replace the original queue
        }

    };

}
