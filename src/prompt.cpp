#include <iostream>

#include "tools/utils/ERROR.hpp"

// #include "tools/utils/utils.hpp"
// #include "tools/cmd/cmd.hpp"
// #include "tools/voice/voice.hpp"
// #include "tools/agency/agency.hpp"

#include "tools/utils/Test.hpp"

using namespace std;
// using namespace tools::utils;
// using namespace tools::cmd;
// using namespace tools::agency;
// using namespace tools::voice;



#include "../libs/K-Adam/SafeQueue/SafeQueue.hpp"

template<typename T>
class Pack {
public:
    Pack(const string& sender = "", const string& recipient = "", T item = T()): sender(sender), recipient(recipient), item(item) {}
    string sender;
    string recipient;
    T item;
};

template<typename T>
class PackQueue: public SafeQueue<Pack<T>> {

    void drop(const string& recipient) {
        unique_lock<mutex> lock(this->mtx); // Access the mutex from SafeQueue

        queue<Pack<T>> temp; // Temporary queue for filtered items
        while (!this->q.empty()) {
            Pack<T> pack = move(this->q.front());
            this->q.pop();
            if (pack.getRecipient() != recipient) {
                temp.push(move(pack)); // Keep packs that don't match
            }
        }
        this->q = move(temp); // Replace the original queue
    }

};

template<typename T>
class PackQueueHolder {
public:
    PackQueueHolder(PackQueue<T>& queue): queue(queue) {}
protected:
    PackQueue<T>& queue;
};

class Closable { // TODO: to common
public:
    void close() {
        closing = true;
    }
protected:
    bool closing = false;
};

template<typename T>
class Agent: public PackQueueHolder<T>, public Closable {
public:
    Agent(PackQueue<T>& queue, const string& name): PackQueueHolder<T>(queue), name(name) {}

    ~Agent() {
        if (t.joinable()) t.join();
    }

    void start() {
        t = thread([this]() {
            try {
                while (!closing) tick();
            } catch (exception &e) {
                cerr << "Agent '" + name + "' error: " << e.what() << endl;
            }
        });
    }

    virtual void handle(const string& sender, const T& item) UNIMP_THROWS

    virtual void tick() {}

    const string name;
protected:
    
    void send(const string& recipient, const T& item) {
        Pack<T> pack(name, recipient, item);
        this->queue.Produce(move(pack));
    }

    void send(const vector<string>& recipients, const T& item) {
        for (const string& recipient: recipients) send(recipient, item);
    }

    thread t;
};

template<typename T>
class Agency: public Agent<T> {
public:
    Agency(PackQueue<T>& queue): Agent<T>(queue, "agency") {}

    void handle(const string& sender, const T& item) override {
        if (item == "exit") {
            
            // emptying package queue
            Pack<T> pack;
            while (this->queue.Consume(pack));

            // closing agents and the agency itself
            for (Agent<T>* agent: agents) agent->close();
            this->close();
        }

        if (item == "list") {
            cout << "Agents in the agency:" << endl;
            for (Agent<T>* agent: agents) cout << agent->name << endl;
        }
    }
    
    template<typename AgentT, typename... Args>
    AgentT& spawn(Args&&... args) {
        AgentT* agent = new AgentT(this->queue, forward<Args>(args)...);
        for (const Agent<T>* a: agents)
            if (agent->name == a->name) {
                delete agent;
                throw ERROR("Agent '" + a->name + "' already exists.");
            }
        agents.push_back(agent);
        agent->start();
        return *agent;
    }

    void kill(const std::string& name) {
        for (size_t i = 0; i < agents.size(); i++)
            if (agents[i]->name == name) {                
                delete agents[i];
                agents.erase(agents.begin() + i);
                i--;  // Back up to recheck the shifted element
            }
        this->queue.drop(name);
    }
    
    void start() {
        Pack<T> pack;
        while (!this->closing)
            while (this->queue.Consume(pack)) {
                if (this->name == pack.recipient) handle(pack.sender, pack.item);
                for (Agent<T>* agent: agents)
                    if (agent->name == pack.recipient) agent->handle(pack.sender, pack.item);    
            }
    }

private:

    vector<Agent<T>*> agents;
};


template<typename T>
class UserAgent: public Agent<T> {
public:
    UserAgent(PackQueue<T>& queue): Agent<T>(queue, "user") {}

    void tick() override {
        T input;
        cin >> input;
        this->send("echo", input);
        if (input == "exit") this->send("agency", "exit");
        if (input == "list") this->send("agency", "list");
    }
};

template<typename T>
class EchoAgent: public Agent<T> {
public:
    EchoAgent(PackQueue<T>& queue): Agent<T>(queue, "echo") {}

    void handle(const string& sender, const T& item) override {
        sleep(2); // emulate some background work;
        cout << "Echo: '" << sender << "' -> " << item << endl;
    }
};


int safe_main(int argc, char *argv[]) {
    run_tests({
        
    });

    try {

        PackQueue<string> queue;
        Agency<string> agency(queue);
        agency.spawn<EchoAgent<string>>();
        agency.spawn<UserAgent<string>>();
        agency.start();

    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    return safe_main(argc, argv);
}