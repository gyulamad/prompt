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
    class Worker: public Closable, public JSONSerializable { // TODO: use JsonFileStorable instead of JSONSerializable
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

        virtual void handle(const string& /*sender*/, const T& /*item*/) = 0;

        virtual void tick() {}

        void exit() {
            this->send("agency", "exit");
            this->close();
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

        virtual string type() const = 0;

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
        
        // LCOV_EXCL_START
        virtual void hoops(const string& errmsg = "") {
            cerr << errmsg << endl;
        }
        // LCOV_EXCL_STOP

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

#ifdef TEST

#include "../str/str_contains.hpp"
#include "chat/Chatbot.hpp"
#include "chat/ChatHistory.hpp"
#include "tests/helpers.hpp"
#include "tests/TestWorker.hpp"
#include "tests/MockWorker.hpp"
#include "tests/default_test_agency_setup.hpp"
#include "PackQueue.hpp" // Needed for queue operations

using namespace tools::agency;
using namespace tools::str;
using namespace tools::agency::chat;

// Test constructor
void test_Worker_constructor_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    auto actual_name = worker.getName();
    assert(actual_name == "test_worker" && "Agent name should be set correctly");
    // Can't directly test queue ref, but we'll use it in send tests
}

// Test single send
void test_Worker_send_single() {
    default_test_agency_setup setup("alice");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    worker.testSend("bob", "hello");
    auto actual_contents = queue_to_vector(setup.queue);
    assert(actual_contents.size() == 1 && "Send should produce one pack");
    assert(actual_contents[0].sender == "alice" && "Sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "Item should be 'hello'");
}

// Test multiple sends
void test_Worker_send_multiple() {
    default_test_agency_setup setup("alice");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    vector<string> recipients = {"bob", "charlie"};
    worker.testSend(recipients, "hello");
    auto actual_contents = queue_to_vector(setup.queue);
    assert(actual_contents.size() == 2 && "Send should produce two packs");
    assert(actual_contents[0].sender == "alice" && "First sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "First recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "First item should be 'hello'");
    assert(actual_contents[1].sender == "alice" && "Second sender should be 'alice'");
    assert(actual_contents[1].recipient == "charlie" && "Second recipient should be 'charlie'");
    assert(actual_contents[1].item == "hello" && "Second item should be 'hello'");
}

// Test tick default does nothing
void test_Worker_tick_default() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    // No output or state to check, just ensure it runs without crashing
    worker.tick();
    // If we reach here, it’s fine—no assert needed for empty default
}

// Test sync runs until closed
void test_Worker_sync_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    worker.close(); // Set closing first
    worker.sync(1); // Should exit immediately
    auto actual_closed = worker.isClosing();
    assert(actual_closed && "Agent should remain closed after sync");
}

// Test async starts and stops
void test_Worker_async_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    worker.start(1, true); // Async with 1ms sleep
    sleep_ms(10); // Let it run briefly
    worker.close(); // Signal to stop
    // Destructor joins thread, so if it exits cleanly, test passes
}

void test_Worker_getAgencyPtr_agency_is_this() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, &worker, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    Worker<string>* agencyPtr = worker.getAgencyPtr();
    assert(agencyPtr == &worker && "Agency pointer should be the same as this when agency is this");
}

// Test exit
void test_Worker_exit_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    worker.exit();
    auto actual_closed = worker.isClosing();
    assert(actual_closed && "Agent should be closed after exit");
}

void test_Worker_start_sync() {
    default_test_agency_setup setup("test_worker");
    MockWorker worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    worker.throwInTick = true;
    worker.start(1, false);
    assert(worker.hoopsCalled);
    assert(str_contains(worker.hoopsErrorMessage, "Worker 'test_worker' error"));
    assert(str_contains(worker.hoopsErrorMessage, "Tick failed"));
}

void test_Worker_addRecipients_basic() {
    default_test_agency_setup setup("test_worker");
    MockWorker worker(setup.owns, setup.agency, setup.queue, setup.name);
    
    vector<string> recipients = {"bob", "charlie"};
    worker.addRecipients(recipients);
    
    vector<string> expected = {"bob", "charlie"};
    vector<string> actual = worker.getRecipients();
    
    assert(vector_equal(actual, expected) && "addRecipients should add the given recipients to the worker");
}

void test_Worker_setRecipients_basic() {
    default_test_agency_setup setup("test_worker");
    MockWorker worker(setup.owns, setup.agency, setup.queue, setup.name);
    
    vector<string> recipients = {"bob", "charlie"};
    worker.setRecipients(recipients);
    
    vector<string> expected = {"bob", "charlie"};
    vector<string> actual = worker.getRecipients();
    
    assert(vector_equal(actual, expected) && "setRecipients should set the recipients to the given recipients");
}

void test_Worker_removeRecipients_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    
    worker.addRecipients({"bob", "charlie", "david"});
    
    vector<string> recipientsToRemove = {"bob", "charlie"};
    worker.removeRecipients(recipientsToRemove);
    
    vector<string> expected = {"david"};
    vector<string> actual = worker.getRecipients();
    
    assert(vector_equal(actual, expected) && "removeRecipients should remove the given recipients from the worker");
}

void test_Worker_findRecipients_basic() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    
    worker.addRecipients({"bob", "charlie", "david"});
    
    vector<string> expected = {"charlie"};
    vector<string> actual = worker.findRecipients("charlie");
    
    assert(vector_equal(actual, expected) && "findRecipients should find the recipients that contain the given keyword");
}

void test_Worker_findRecipients_emptyKeyword() {
    default_test_agency_setup setup("test_worker");
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    
    worker.addRecipients({"bob", "charlie", "david"});
    
    vector<string> expected = {"bob", "charlie", "david"};
    vector<string> actual = worker.findRecipients();
    
    assert(vector_equal(actual, expected) && "findRecipients should return all recipients when the keyword is empty");
}

// Register tests
TEST(test_Worker_constructor_basic);
TEST(test_Worker_send_single);
TEST(test_Worker_send_multiple);
TEST(test_Worker_tick_default);
TEST(test_Worker_sync_basic);
TEST(test_Worker_async_basic);
TEST(test_Worker_getAgencyPtr_agency_is_this);
TEST(test_Worker_exit_basic);
TEST(test_Worker_start_sync);
TEST(test_Worker_addRecipients_basic);
TEST(test_Worker_setRecipients_basic);
TEST(test_Worker_removeRecipients_basic);
TEST(test_Worker_findRecipients_basic);
TEST(test_Worker_findRecipients_emptyKeyword);

#endif
