#pragma once

#include <mutex>
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <map>
#include <stdexcept>

#include "ERROR.hpp"

using namespace std;

namespace tools::utils {

    template<typename T>
    class Factory {
    public:
        using Creator = function<T*(void* /*user_data = nullptr*/)>;

        Factory() {}

        virtual ~Factory() {
            lock_guard<mutex> lock(mtx);
            for (T* instance : instances) {
                if (!instance) continue;
                delete instance;
                instance = nullptr;
            }
        }
        
        Factory(const Factory&) = delete;
        Factory& operator=(const Factory&) = delete;
        Factory(Factory&&) = delete;
        Factory& operator=(Factory&&) = delete;

        void registry(const string& type, Creator creator) {
            lock_guard<mutex> lock(mtx);
            creators[type] = move(creator);
        }

        T* create(const string& type, void* user_data = nullptr) {
            lock_guard<mutex> lock(mtx);
            auto it = creators.find(type);
            if (it == creators.end())
                throw ERROR("Unknown type: " + type);
            T* instance = nullptr;
            try {
                instance = it->second(user_data);
                instances.push_back(instance);
                return instance;
            } catch (exception& e) {
                if (instance) delete instance;
                instance = nullptr;
                throw ERROR("Instance of '" + type + "' creation failed: " + e.what());
            }
        }

        void destroy(T*& instance) {
            lock_guard<mutex> lock(mtx);
            if (!instance) return;
            typename vector<T*>::iterator it = 
                find(instances.begin(), instances.end(), instance);
            if (it != instances.end()) {
                if (instance) delete instance;
                instance = nullptr;
                instances.erase(it);
            }
        }

        vector<T*>& getInstancesRef() { return instances; }

    private:
        mutex mtx;
        vector<T*> instances;
        map<string, Creator> creators;
    };

}
