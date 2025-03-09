#pragma once

#include <string>

using namespace std;

class TestObj {
public:
    TestObj(const string& id) : id(id) {}
    ~TestObj() {}
    string getId() const { return id; }
private:
    string id;
};