#pragma once

#include <string>
#include <vector>

#include "../utils/Owns.hpp"
#include "../abstracts/Closable.hpp"
#include "../abstracts/JSONSerializable.hpp"
#include "../containers/array_merge.hpp"
#include "../containers/array_diff.hpp"

#include "Pack.hpp"
#include "PackQueue.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::abstracts;
using namespace tools::containers;

namespace tools::agency {

    template<typename T>
    class Worker: public Closable, public JSONSerializable {
    public:
        Worker(
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const string& name
        ): 
            Closable(),
            JSONSerializable(),
            owns(owns),
            agency(agency),
            queue(queue),
            name(name)
        {}

        virtual ~Worker() {
            this->close();
            if (t.joinable()) t.join();
        }

        // -----------------------------------------------------------------
        // ---- model ------------------------------------------------------
        // -----------------------------------------------------------------

        Owns& getOwnsRef() const { return owns; }
        Worker<T>* getAgencyPtr() { return agency ? agency : this; }
        PackQueue<T>& getQueueRef() { return queue; }
        string getName() const { return name; }
        vector<string> getRecipients() const { return recipients; }


        // -----------------------------------------------------------------
        // ---- working ----------------------------------------------------
        // -----------------------------------------------------------------

        virtual void handle(const string& /*sender*/, const T& /*item*/) UNIMP_THROWS

        virtual void tick() {}

        void exit() {
            this->send("agency", "exit");
        }

        void start(long ms = 10, bool run_async = true) {
            if (run_async) async(ms);
            else sync(ms);
        }

        void async(long ms = 10) {
            t = thread([this, ms]() { sync(ms); });
        }

        void sync(long ms = 10) {
            while (!closing) {
                try {
                    if (ms) sleep_ms(ms);
                    tick();
                } catch (exception &e) {
                    hoops("Worker '" + name + "' error: " + string(e.what()));
                }
            }
        }


        // -----------------------------------------------------------------
        // ---- messaging --------------------------------------------------
        // -----------------------------------------------------------------

        void addRecipients(const vector<string>& recipients) {
            this->recipients = array_merge(this->recipients, recipients);
        }

        void setRecipients(const vector<string>& recipients) {
            this->recipients = recipients;
        }

        void removeRecipients(const vector<string>& recipients) {
            this->recipients = array_diff(this->recipients, recipients);
        }

        vector<string> findRecipients(const string& keyword = "") const {
            if (keyword.empty()) return recipients;
            vector<string> found;
            foreach(recipients, [&](const string& recipient) {
                if (str_contains(recipient, keyword)) 
                    found.push_back(recipient);
            });
            return found;
        }

        
        // -----------------------------------------------------------------
        // ---- view -------------------------------------------------------
        // -----------------------------------------------------------------

        virtual string type() const UNIMP_THROWS

        virtual string dump() const {
            string recipients = implode("', '", this->recipients);
            return tpl_replace({
                { "{{name}}" , this->name },
                { "{{type}}" , type() },
                { "{{recipients}}" , recipients.empty() ? "<nobody>" : recipients },
            }, "Worker '{{name}}' is a(n) '{{type}}' worker, talking to {{recipients}}.");
        }

        // ----- JSON serialization -----

        void fromJSON(const JSON& json) override {
            // DEBUG(json.dump());
            recipients = json.get<vector<string>>("recipients");
        }

        JSON toJSON() const override {
            JSON json;
            json.set("role", this->type());
            json.set("name", name);
            json.set("recipients", recipients);
            return json;
        }

    protected:

        void send(const T& item) {
            send(recipients, item);
        }
        
        virtual void hoops(const string& errmsg = "") {
            cerr << errmsg << endl;
        }

        Owns& owns;
        Worker<T>* agency = nullptr;
        PackQueue<T>& queue;
        string name;
        vector<string> recipients;

    private:

        void send(const string& recipient, const T& item) {
            if (name == recipient) ERROR("Can not send for itself: " + name);
            Pack<T> pack(name, recipient, item);
            queue.Produce(move(pack));
        }

        void send(const vector<string>& recipients, const T& item) {
            for (const string& recipient: recipients) send(recipient, item);
        }

        thread t;
    };

}