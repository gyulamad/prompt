#pragma once

#include <mutex>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <stdexcept>

#include "ERROR.hpp"
#include "foreach.hpp"

using namespace std;

namespace tools::utils {

    template<typename T>
    class Factory {
    public:
        using Creator = function<T*(void* /*user_data = nullptr*/)>;

        Factory() {}

        virtual ~Factory() {
            lock_guard<mutex> lock(mtx);
            if (instances.empty()) return;
            string msg = "Factory destroyed with " + to_string(instances.size()) + " instances still alive:\n";
            foreach (instances, [&](int count, const T* instance) {
                msg += "  Instance " + to_string(reinterpret_cast<uintptr_t>(instance)) + 
                        " has " + to_string(count) + " owners\n";
                delete instance;  // Cleanup to avoid leaks, but still report
            });
            cerr << ERROR(msg).what() << endl;
        }
        
        Factory(const Factory&) = delete;
        Factory& operator=(const Factory&) = delete;
        Factory(Factory&&) = delete;
        Factory& operator=(Factory&&) = delete;

        void registry(const string& type, Creator creator) {
            lock_guard<mutex> lock(mtx);
            creators[type] = move(creator);
        }

        Creator creator(const string& type) {
            lock_guard<mutex> lock(mtx);
            Creator creator = nullptr;
            foreach (creators, [&](Creator crtr, const string& typ) {
                if (typ != type) return FE_CONTINUE;
                creator = crtr;
                return FE_BREAK;
            });
            if (!creator) throw ERROR("Unknown type: " + type);
            return creator;
        }

        T* create(const string& type, void* user_data = nullptr) {
            lock_guard<mutex> lock(mtx);
            T* instance = nullptr;
            try {
                instance = creator(type)(user_data);
                instances[instance] = 0;  // Start with 0 owners
                return instance;
            } catch (exception& e) {
                if (instance) delete instance;
                instance = nullptr;
                throw ERROR("Instance of '" + type + "' creation failed: " + e.what());
            }
        }

        typename map<T*, int>::iterator has(T* instance, bool throws) {
            lock_guard<mutex> lock(mtx);
            typename map<T*, int>::iterator it = instances.find(instance);
            if (it == instances.end()) {
                if (throws) throw ERROR("Factory called on untracked instance: " + 
                    to_string(reinterpret_cast<uintptr_t>(instance)));
                return nullptr;
            }
            return it;
        }

        int owners(T* instance) {
            return has(instance, true)->second;
        }

        Factory& hold(void* owner, T* instance) {
            lock_guard<mutex> lock(mtx);
            has(instance, true)->second++;
            //cout << "Instance " << instance << " now has " << it->second << " owners" << endl;
            return *this;
        }

        Factory& release(void* owner, T* instance) {
            lock_guard<mutex> lock(mtx);
            typename map<T*, int>::iterator it = has(instance, true);
            if (it->second <= 0)
                throw ERROR("release called on instance " + 
                            to_string(reinterpret_cast<uintptr_t>(instance)) + 
                            " with no owners");
            it->second--;
            // cout << "Instance " << instance << " now has " << it->second << " owners" << endl;
            if (it->second == 0) {
                delete instance;
                instance = nullptr;
                instances.erase(it);
            }
            return *this;
        }

        void destroy(T* instance) {
            lock_guard<mutex> lock(mtx);
            typename map<T*, int>::iterator it = has(instance, true);
            if (it->second > 0)
                throw ERROR("destroy called on instance " + 
                            to_string(reinterpret_cast<uintptr_t>(instance)) + 
                            " with " + to_string(it->second) + 
                            " owners; consider using release instead");
            delete instance;
            instance = nullptr;
            instances.erase(it);
            //cout << "Instance " << instance << " destroyed" << endl;
        }

    private:
        mutex mtx;
        map<T*, int> instances;  // instance -> owner count
        map<string, Creator> creators;
    };

}
