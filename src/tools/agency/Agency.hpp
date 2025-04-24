#pragma once

#include "../containers/array_key_exists.hpp"
// #include "../utils/Streamable.hpp"
// #include "chat/Chatbot.hpp"
// // #include "chat/Talkbot.hpp"

#include "Worker.hpp"
// #include "PackQueue.hpp"
#include "AgentRoleMap.hpp"

using namespace tools::utils;
// using namespace tools::agency;
// using namespace tools::agency::chat;

namespace tools::agency {

    template<typename T>
    class Agency: public Worker<T> {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:

        static const string worker_list_tpl;

        Agency(
            Owns& owns,
            AgentRoleMap& roles,
            PackQueue<T>& queue,
            const string& name
        ):
            Worker<T>(owns, nullptr, queue, name),
            owns(owns),
            roles(roles)
        {}

        virtual ~Agency() {
            lock_guard<mutex> lock(workers_mtx);
            for (Worker<T>* worker : workers) {
                owns.release(this, worker); // delete worker
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

                // TODO: save agency (or agents) on exit if it's autosave on                

                // closing workers and the agency itself
                for (Worker<T>* worker: workers) worker->close();
                this->close();

                // swallowing packages from queue (TODO: perhaps we want to proceed all - with a timeout - before close)
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
            Worker<T>::fromJSON(json);

            if (json.has("workers")) {
                vector<JSON> jworkers = json.get<vector<JSON>>("workers");
                for (const JSON& jworker: jworkers) {
                    string role = jworker.get<string>("role");
                    string name = jworker.get<string>("name");
                    if (!array_key_exists(role, roles))
                        throw ERROR("Role not exists: " + role);
                    AgentInstantiator maker = roles[role];
                    maker(name, jworker);
                }
            }
        }

        JSON toJSON() const override {
            JSON json = Worker<T>::toJSON();

            vector<JSON> jworkers;
            for (const Worker<T>* worker: workers) {
                safe(worker);
                if (worker->getName() == "user") continue;
                jworkers.push_back(worker->toJSON());
            }
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

#include "tests/TestAgency.hpp"

// Agency tests
void test_Agency_constructor_basic() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    auto actual_name = agency.getName();
    assert(actual_name == "agency" && "Agency name should be 'agency'");
}

void test_Agency_handle_exit() {
    default_test_agency_setup setup("agency");
    TestAgency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& test_worker = agency.template spawn<TestWorker<string>>(setup.owns, setup.agency, setup.queue, "test_worker");
    test_worker.fromJSON(setup.json);
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
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& test_worker1 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "worker1");
    test_worker1.fromJSON(setup.json);
    TestWorker<string>& test_worker2 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "worker2");
    test_worker2.fromJSON(setup.json);
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    auto expected_output = "Workers in the agency:\nworker1\nworker2\n";
    assert(actual_output == expected_output && "List should output all worker names");
}

void test_Agency_spawn_success() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    auto& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker.fromJSON(setup.json);
    auto actual_name = worker.getName();
    assert(actual_name == "test_worker" && "Spawned worker should have correct name");
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    assert(str_contains(actual_output, "test_worker") && "Spawned worker should appear in list");
}

void test_Agency_spawn_duplicate() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker1 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker1.fromJSON(setup.json);
    bool thrown = false;
    try {
        agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Worker 'test_worker' already exists") && "Exception should indicate duplicate name");
    }
    assert(thrown && "Spawn with duplicate name should throw");
}

void test_Agency_kill_basic() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker.fromJSON(setup.json);
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
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    auto& test_worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    test_worker.fromJSON(setup.json);
    setup.queue.Produce(Pack<string>("user", "agency", "list"));
    setup.queue.Produce(Pack<string>("user", "test_worker", "hello"));
    auto actual_output = capture_cout([&]() { agency.tick(); });
    assert(str_contains(actual_output, "test_worker") && "Tick should process agency message");
    assert(test_worker.handled && "Tick should dispatch to worker");
}

void test_Agency_type() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    auto actual_type = agency.type();
    assert(actual_type == "agency" && "Agency::type() should return 'agency'");
}

void test_Agency_hasWorker_existing() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker.fromJSON(setup.json);
    bool actual = agency.hasWorker("test_worker");
    assert(actual && "Agency should have the worker");
}

void test_Agency_hasWorker_nonExisting() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    bool actual = agency.hasWorker("non_existing_worker");
    assert(!actual && "Agency should not have the worker");
}

void test_Agency_getWorkerRef_existing() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker.fromJSON(setup.json);
    Worker<string>& actual_worker = agency.getWorkerRef("test_worker");
    assert(actual_worker.getName() == "test_worker" && "Should return the correct worker");
}

void test_Agency_getWorkerRef_nonExisting() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    bool thrown = false;
    try {
        agency.getWorkerRef("non_existing_worker");
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Requested worker 'non_existing_worker' is not found.") && "Exception should indicate worker not found");
    }
    assert(thrown && "Should throw an exception when worker is not found");
}

void test_Agency_findWorkers_empty() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    vector<string> actual = agency.findWorkers();
    assert(actual.empty() && "Should return empty vector when no workers are present");
}

void test_Agency_findWorkers_withWorkers() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker1 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker1");
    worker1.fromJSON(setup.json);
    TestWorker<string>& worker2 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker2");
    worker2.fromJSON(setup.json);
    vector<string> actual = agency.findWorkers();
    assert(actual.size() == 2 && "Should return vector with all worker names");
    assert(actual[0] == "test_worker1" && "First worker name should be correct");
    assert(actual[1] == "test_worker2" && "Second worker name should be correct");
}

void test_Agency_findWorkers_withKeyword() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker1 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker1");
    worker1.fromJSON(setup.json);
    TestWorker<string>& worker2 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "another_worker");
    worker2.fromJSON(setup.json);
    vector<string> actual = agency.findWorkers("test");
    assert(actual.size() == 1 && "Should return vector with matching worker names");
    assert(actual[0] == "test_worker1" && "Worker name should be correct");
}

void test_Agency_dumpWorkers_empty() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    vector<string> names;
    string actual = agency.dumpWorkers(names);
    assert(actual.empty() && "Should return empty string when no names are provided");
}

void test_Agency_dumpWorkers_withWorkers() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker1 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker1");
    worker1.fromJSON(setup.json);
    TestWorker<string>& worker2 = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker2");
    worker2.fromJSON(setup.json);
    vector<string> names = {"test_worker1", "test_worker2"};
    string actual = agency.dumpWorkers(names);
    assert(str_contains(actual, "test_worker1") && "Should contain dump of worker1");
    assert(str_contains(actual, "test_worker2") && "Should contain dump of worker2");
}

void test_Agency_dumpWorkers_nonExisting() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    vector<string> names = {"non_existing_worker"};
    string actual = agency.dumpWorkers(names);
    assert(str_contains(actual, "Worker non_existing_worker is not exists!") && "Should indicate worker not found");
}

void test_Agency_fromJSON_workers() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    setup.agency = &agency;
    setup.roles = {{ "chat", [&](const string& name, const JSON&) { agency.spawn<TestWorker<string>>(setup.owns, setup.agency, setup.queue, name); } }};
    setup.json.set("workers", JSON(R"(
        [
            {
                "role": "chat",
                "name": "talkbot"
            }
        ]
    )"));
    agency.fromJSON(setup.json);
    assert(agency.hasWorker("talkbot") && "Agency should have talkbot after fromJSON");
}

void test_Agency_toJSON_workers() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    agency.fromJSON(setup.json);
    TestWorker<string>& worker = agency.spawn<TestWorker<string>>(setup.owns, &agency, setup.queue, "test_worker");
    worker.fromJSON(setup.json);
    JSON json = agency.toJSON();
    assert(json.has("workers") && "toJSON should contain workers");
    vector<JSON> workers = json.get<vector<JSON>>("workers");
    bool found = false;
    for (const JSON& w : workers) {
        if (w.get<string>("name") == "test_worker") {
            found = true;
            break;
        }
    }
    assert(found && "toJSON should contain test_worker");
}

void test_Agency_fromJSON_throws_on_invalid_role() {
    default_test_agency_setup setup("agency");
    Agency<string> agency(setup.owns, setup.roles, setup.queue, setup.name);
    setup.agency = &agency;
    setup.roles = {{ "chat", [&](const string& name, const JSON&) { agency.spawn<TestWorker<string>>(setup.owns, setup.agency, setup.queue, name); } }};
    const string json_str = R"(
        {
            "name": "TestAgency", 
            "workers": [{"name": "TestAgent", "role": "NonExistentRole"}],
            "recipients": []
        }
    )";
    bool thrown = false;
    try {
        agency.fromJSON(json_str);
    } catch (const runtime_error& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Role not exists: NonExistentRole") && "Exception message is incorrect");
    }
    assert(thrown && "Should have thrown an exception");
}

void test_Agency_getAgencyPtr_agency_is_not_this() {
    Owns owns;
    default_test_agency_setup setup("test_worker");
    setup.agency = owns.allocate<Agency<string>>(
        owns,
        setup.roles,
        setup.queue,
        "test_agency"
    );
    TestWorker<string> worker(setup.owns, setup.agency, setup.queue, setup.name);
    worker.fromJSON(setup.json);
    Worker<string>* agencyPtr = worker.getAgencyPtr();
    assert(agencyPtr == setup.agency && "Agency pointer should be the same as agency when agency is not this");
}

// Register tests
TEST(test_Agency_constructor_basic);
TEST(test_Agency_handle_exit);
TEST(test_Agency_handle_list);
TEST(test_Agency_spawn_success);
TEST(test_Agency_spawn_duplicate);
TEST(test_Agency_kill_basic);
TEST(test_Agency_tick_dispatch);
TEST(test_Agency_type);
TEST(test_Agency_hasWorker_existing);
TEST(test_Agency_hasWorker_nonExisting);
TEST(test_Agency_getWorkerRef_existing);
TEST(test_Agency_getWorkerRef_nonExisting);
TEST(test_Agency_findWorkers_empty);
TEST(test_Agency_findWorkers_withWorkers);
TEST(test_Agency_findWorkers_withKeyword);
TEST(test_Agency_dumpWorkers_empty);
TEST(test_Agency_dumpWorkers_withWorkers);
TEST(test_Agency_dumpWorkers_nonExisting);
TEST(test_Agency_fromJSON_workers);
TEST(test_Agency_toJSON_workers);
TEST(test_Agency_fromJSON_throws_on_invalid_role);
TEST(test_Agency_getAgencyPtr_agency_is_not_this);

// TODO:
// Memory Management: Ensure ~Agency doesn’t double-delete if kill is called before destruction (current code is safe, but worth a double-check).
// Deadlock Prevention: Verify no deadlocks if handle calls back into queue (e.g., via send).

#endif
