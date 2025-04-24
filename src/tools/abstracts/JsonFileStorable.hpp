#pragma once

#include <string>

#include "JSONSerializable.hpp"

using namespace std;

namespace tools::abstracts {

    class JsonFileStorable: public JSONSerializable {
    public:
        JsonFileStorable() {}
        virtual ~JsonFileStorable() {}
        virtual void toJsonFile(const string& fname) = 0;
        virtual void fromJsonFile(const string& fname) = 0;
    };

}
