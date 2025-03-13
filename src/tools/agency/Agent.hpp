#pragma once

#include <functional>

#include "../utils/ERROR.hpp"
#include "../utils/strings.hpp"
#include "../utils/datetime.hpp"

#include "Pack.hpp"
#include "PackQueue.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::agency {

#define AGENT_SENDER_EXIT_SIGNALER "***__EXIT_SIGNALER__***"

    template<typename T>
    class Agent {
    public:
        Agent(
            PackQueue<T>& queue, 
            const string& name = "",
            long ms = 0
        ):
            queue(queue), 
            name(name),
            ms(ms)
        {}
    
        virtual ~Agent() {
            if (t.joinable()) t.join();
        }

        virtual void start() {
            t = thread([this]() { loop(); });
        }

        [[nodiscard]]
        bool isExited() const {
            return exited;
        }

        string getName() const {
            return name;
        }

    protected:

        [[nodiscard]]
        virtual bool handle(const string& sender, const T& item) UNIMP_THROWS

        void send(T item, string recipient) {
            Pack<string> pack(item, name, recipient);
            queue.Produce(move(pack));
        }

        void send(T item, vector<string> recipients) {
            for (const string& recipient: recipients) send(item, recipient);
        }

        void exit() {
            T item;
            Pack<string> pack(item, AGENT_SENDER_EXIT_SIGNALER);
            queue.Produce(move(pack));
        }

        void loop() {
            Pack<T> pack;
            while (true) {
                try {
                    if (ms) sleep_ms(ms);
                    if (!tick()) break;

                    queue.Hold();

                    Pack<T>* packPtr = queue.Peek();
                    if (!packPtr) { // empty
                        queue.Release();
                        continue;
                    }

                    string sender = packPtr->getSender();
                    if (sender == AGENT_SENDER_EXIT_SIGNALER) { // exiting
                        queue.Release();
                        break;
                    }

                    string recipient = packPtr->getRecipient();
                    if (recipient != name) { // skip
                        queue.Release();
                        continue;
                    }

                    if (!queue.Consume(pack)) {
                        queue.Release();
                        ouch("queue error");
                        continue;
                    }

                    queue.Release();

                    if (!handle(sender, pack.getItem())) break; // leaving 

                    if (dying) break;
                } catch (exception& e) {
                    string what(e.what());
                    ouch(what);
                }
            }
            exited = true;
        }

        void ouch(const string& what) {
            throw ERROR("Error in agent '" + name + "'" + (what.empty() ? "" : ": " + what));
        }

        void die() {
            dying = true;
        }

        [[nodiscard]]
        virtual bool tick() { return true; }

        atomic<bool> exited = false;
        atomic<bool> dying = false;
        PackQueue<T>& queue;
        string name;
        long ms;
        thread t;
    };

}
