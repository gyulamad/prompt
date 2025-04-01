#pragma once

using namespace tools::utils;

class OwnsSpy: public Owns {
public:
    unordered_map<void*, ownnfo> getReserves() {
        return this->reserves;
    }

    void publicCleanup() {
        this->cleanup();
    }
};