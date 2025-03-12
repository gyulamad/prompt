#include "tools/utils/utils.hpp"
// #include "tools/cmd/cmd.hpp"
// #include "tools/voice/voice.hpp"
#include "tools/agency/agency.hpp"

#include "tools/utils/Test.hpp"

using namespace std;
using namespace tools::utils;
// using namespace tools::cmd;
using namespace tools::agency;
// using namespace tools::voice;

template<typename T>
class UserAgent: public Agent<T> {
public:
    UserAgent(PackQueue<string>& queue): Agent<T>(queue, "user") {}

    bool tick() override {
        T input;
        cin >> input;
        this->send(input, "echo");
        if (input == "exit") this->exit();
        return true;
    }
};

template<typename T>
class EchoAgent: public Agent<T> {
public:
    EchoAgent(PackQueue<T>& queue): Agent<T>(queue, "echo") {}

    bool handle(const string& sender, const T& item) override {
        sleep(2); // emulate some background work;
        cout << "Echo: '" << sender << "' -> " << item << endl;
        return true;
    }
};

template<typename T>
class Agency: public Agent<T> {
public:
    Agency(PackQueue<T> queue = PackQueue<T>(), long ms = 100): Agent<T>(queue, "agency", ms) {}

    // virtual ~Agency() {
    //     for (const Agent<T>* agent: agents) agent->die();
    //     if (t.joinable()) t.join();
    // }

    // void run() {
    //     t = thread([this]() {
    //         while(true) {
    //             vector<Agent<T>*> result;
    //             copy_if(agents.begin(), agents.end(), back_inserter(result), [this](const Agent<T>* agent) {
    //                 if (agent->isExited()) {
    //                     // Remove packs from queue which addressed to the agent but never will be delivered
    //                     queue.drop(agent->getName());
    //                     delete agent;
    //                     return false;
    //                 }
    //                 return true;
    //             });
    //             agents = result;
    //             sleep_ms(ms);
    //         }
    //     });
    // }
    
    // template<typename AgentT, typename... Args>
    // AgentT* spawn(Args&&... args) {
    //     Agent<T>* agent = new AgentT(queue, forward<Args>(args)...);
    //     // agent->name should be unique! check for agent name in agents vector first!
    //     string name = agent->getName();
    //     for (const Agent<T>* a: agents)
    //         if (a->getName() == name) {
    //             delete agent;
    //             return nullptr;
    //         }
    //     agents.push_back(agent);
    //     return (AgentT*)agent;
    // }
    
    // template<typename AgentT, typename... Args>
    // void kill(AgentT* agent) {
    //     agent->die();
    // }

    // void kill(const string& name) {
    //     for (const Agent<T>* agent: agents)
    //         if (agent->getName() == name) agent->die();
    // }

protected:
    // PackQueue<T> queue;
    // long ms;
    // thread t;
};


int safe_main(int argc, char *argv[]) {
    run_tests();

    try {
        // vector<Agent<string>*> agents;

        Agency<string> agency;
        agency.spawn<EchoAgent<string>>()->start();
        agency.spawn<UserAgent<string>>()->start();
        // agency.run();

        // EchoAgent echo(queue);
        // agents.push_back(&echo);
        // echo.start();

        // UserAgent user(queue);
        // agents.push_back(&user);
        // user.start();

        // while (true) {
        //     bool exit = true;
        //     for (const Agent<string>* agent: agents) 
        //         if (!agent->isExited()) {
        //             exit = false;
        //             break;
        //         }
        //     if (exit) break;
        //     sleep_ms(100);
        // }

    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    return safe_main(argc, argv);
}