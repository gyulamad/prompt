#pragma once

#include "../utils/Streamable.hpp"
#include "../chat/Chatbot.hpp"
#include "../chat/Talkbot.hpp"

#include "Worker.hpp"
#include "PackQueue.hpp"
#include "AgentRoleMap.hpp"

using namespace tools::utils;
using namespace tools::chat;
using namespace tools::agency;

namespace tools::agency {

    template<typename T>
    class Agency: public Worker<T>/*: public Worker<T>*/ {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:

        static const string worker_list_tpl;

        // using Worker<T>::Worker;
        Agency(
            Owns& owns,
            AgentRoleMap& roles,
            PackQueue<T>& queue,
            JSON& json 
            // const string& name,
            // vector<string> recipients
        ):
            owns(owns),
            roles(roles),
            Worker<T>(owns, nullptr, queue, json/*, name, recipients*/)
        {}

        virtual ~Agency() {
            lock_guard<mutex> lock(workers_mtx);
            for (Worker<T>* worker : workers) {
                owns.release(this, worker); // delete worker;
                worker = nullptr;
            }
            workers.clear();
        }

        string type() const { return "agency"; }

        // void setVoiceOutput(bool state) { voice = state; }

        // bool isVoiceOutput() const { return voice; }

        void handle(const string& sender, const T& item) override {

            // TODO: these are deprecated: (commands and workers has/can have access to the agency so they can do it by themself)
            if (item == "exit") {
                cout << "Exit indicated by worker '" + sender + "'..." << endl;

                // closing workers and the agency itself
                for (Worker<T>* worker: workers) worker->close();
                this->close();

                // emptying package queue
                while (this->queue.Consume(pack));
            }

            if (item == "list") {
                cout << "Workers in the agency:" << endl;
                for (Worker<T>* worker: workers) cout << worker->getName() << endl;
            }
        }
        
        template<typename WorkerT, typename... Args>
        WorkerT& spawn(Args&&... args) { // TODO: forward Args
            lock_guard<mutex> lock(workers_mtx);
            // WorkerT* worker = new WorkerT(forward<Args>(args)...); // Direct construction
            WorkerT* worker = owns.allocate<WorkerT>(forward<Args>(args)...);
            owns.reserve<void>(this, worker, FILELN);
            // WorkerT* worker = new WorkerT(/*this->queue, name, recipients,*/ forward<Args>(args)...);
            for (const Worker<T>* a: workers)
                if (worker->getName() == a->getName()) {
                    owns.release(this, worker); // delete worker;
                    throw ERROR("Worker '" + a->getName() + "' already exists.");
                }
            workers.push_back(worker);
            // worker->start();
            this->send("Worker '" + worker->getName() + "' created as '" + worker->type() + "'.");
            return *(WorkerT*)worker;
        }

        [[nodiscard]]
        bool kill(const string& name) {        
            lock_guard<mutex> lock(workers_mtx);
            bool found = false;   
            for (size_t i = 0; i < workers.size(); i++)
                if (workers[i]->getName() == name) {
                    found = true;
                    //workers[i]->close();
                    owns.release(this, workers[i]); // delete workers[i];
                    workers[i] = nullptr;
                    workers.erase(workers.begin() + i);
                    i--;  // Back up to recheck the shifted element
                }
            this->queue.drop(name);
            return found;
        }
        
        void tick() {
            while (this->queue.Consume(pack)) {
                if (this->name == pack.recipient) 
                    handle(pack.sender, pack.item);
                else {
                    lock_guard<mutex> lock(workers_mtx);
                    for (Worker<T>* worker: workers)
                        if (worker && worker->getName() == pack.recipient) 
                            worker->handle(pack.sender, pack.item);    
                }
            }
        }

        bool hasWorker(const string& name) const {
            for (Worker<T>* worker: workers)
                if (worker->getName() == name) return true;
            return false;
        }

        // template<typename WorkerT>
        Worker<T>& getWorkerRef(const string& name) const {
            for (Worker<T>* worker: workers)
                if (safe(worker)->getName() == name) return *(Worker<T>*)worker;
            throw ERROR("Requested worker '" + name + "' is not found.");
        }

        // const vector<Worker<T>*>& getWorkersCRef() const { return workers; }

        vector<string> findWorkers(const string& keyword = "") const {
            vector<string> found;
            for (const Worker<T>* worker: workers) {
                string name = safe(worker)->getName();
                if (keyword.empty() || str_contains(name, keyword))
                    found.push_back(name);
            };
            return found;
        }

        string dumpWorkers(const vector<string>& names) const {
            vector<string> dumps;
            for (const string& name: names) {
                if (hasWorker(name)) dumps.push_back(getWorkerRef(name).dump());
                else dumps.push_back("Worker " + name + " is not exists!");
            };
            return implode("\n", dumps);
        }

        // ---- JSON serialization ----

        void fromJSON(const JSON& json) override {
            vector<JSON> jworkers = json.get<vector<JSON>>("workers");
            for (const JSON& jworker: jworkers) {
                roles[jworker.get<string>("role")](
                    // jworker.get<string>("name"),
                    // jworker.get<vector<string>>("recipients"),
                    jworker
                );
            }
        }

        JSON toJSON() const override {
            JSON json;
            vector<JSON> jworkers;
            for (const Worker<T>* worker: workers)
                jworkers.push_back(safe(worker)->toJSON());
            json.set("workers", jworkers);
            return json;
        }

    private:
        Owns& owns;
        AgentRoleMap& roles;
        // bool voice = false;
        vector<Worker<T>*> workers; // TODO: Replace vector<Worker<T>*> with unordered_map<string, Worker<T>*>, O(1) lookup vs. O(n), huge win with many workers.
        
        mutex workers_mtx;
        Pack<T> pack;
    };

    template<typename T>
    const string Agency<T>::worker_list_tpl = R"(List of workers in agency '{{agency}}':
{{workers}}
{{found}} of {{total}} worker(s) found.)";
    
}

#ifdef TEST

#include "../utils/Test.hpp"
#include "tests/TestWorker.hpp"
#include "tests/default_test_agency_setup.hpp"
#include "PackQueue.hpp"

// Previous helpers (e.g., queue_to_vector) ...

using namespace tools::agency;

#include "tests/TestAgency.hpp"

// Agency tests
void test_Agency_constructor_basic() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    auto actual_name = agency.getName();
    assert(actual_name == "agency" && "Agency name should be 'agency'");
}

void test_Agency_handle_exit() {
    default_test_agency_setup setup("agency");
    TestAgency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "test_worker");
    TestWorker<string>& test_worker = agency.template spawn<TestWorker<string>>(setup.owns, setup.agency, setup.queue, setup.json);
    auto actual_output = capture_cout([&]() { agency.handle("user", "exit"); });
    auto actual_closed = agency.isClosing();
    auto actual_worker_closed = test_worker.isClosing();
    auto actual_queue_contents = queue_to_vector(setup.queue);
    assert(actual_closed && "Agency should be closed after exit");
    assert(actual_worker_closed && "Spawned worker should be closed after exit");
    assert(actual_queue_contents.empty() && "Queue should be empty after exit");
    assert(str_contains(actual_output, "Exit indicated by worker 'user'...") && "Exit message should be printed");
}

void test_Agency_handle_list() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "worker1");
    agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    setup.json.set("name", "worker2");
    agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    auto expected_output = "Workers in the agency:\nworker1\nworker2\n";
    assert(actual_output == expected_output && "List should output all worker names");
}

void test_Agency_spawn_success() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "test_worker");
    auto& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    auto actual_name = worker.getName();
    assert(actual_name == "test_worker" && "Spawned worker should have correct name");
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    assert(str_contains(actual_output, "test_worker") && "Spawned worker should appear in list");
}

void test_Agency_spawn_duplicate() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "test_worker");
    agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    bool thrown = false;
    try {
        agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Worker 'test_worker' already exists") && "Exception should indicate duplicate name");
    }
    assert(thrown && "Spawn with duplicate name should throw");
}

void test_Agency_kill_basic() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "test_worker");
    agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    setup.queue.Produce(Pack<string>("user", "test_worker", "hello"));
    agency.tick();  // Process the queue before killing
    assert(agency.kill("test_worker") && "Worker should be found");
    auto actual_output = capture_cout([&]() { agency.handle("user", "/list"); });
    auto actual_queue = queue_to_vector(setup.queue);
    assert(!str_contains(actual_output, "test_worker") && "Killed worker should not appear in list");
    assert(actual_queue.empty() && "Queue should have no items for killed worker");
}

void test_Agency_tick_dispatch() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.json);
    setup.json.set("name", "test_worker");
    auto& test_worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, setup.json);
    setup.queue.Produce(Pack<string>("user", "agency", "list"));
    setup.queue.Produce(Pack<string>("user", "test_worker", "hello"));
    auto actual_output = capture_cout([&]() { agency.tick(); });
    assert(str_contains(actual_output, "test_worker") && "Tick should process agency message");
    assert(test_worker.handled && "Tick should dispatch to worker");
}

// Register tests
TEST(test_Agency_constructor_basic);
TEST(test_Agency_handle_exit);
TEST(test_Agency_handle_list);
TEST(test_Agency_spawn_success);
TEST(test_Agency_spawn_duplicate);
TEST(test_Agency_kill_basic);
TEST(test_Agency_tick_dispatch);

// TODO:
// Memory Management: Ensure ~Agency doesn’t double-delete if kill is called before destruction (current code is safe, but worth a double-check).
// Deadlock Prevention: Verify no deadlocks if handle calls back into queue (e.g., via send).

#endif
