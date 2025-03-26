#pragma once

// #include "../Agency.hpp"

// using namespace tools::agency;

template<typename T>
class TestAgency: public Agency<T> {
public:

    using Agency<T>::Agency;

    bool isClosing() const { return this->closing; }
};