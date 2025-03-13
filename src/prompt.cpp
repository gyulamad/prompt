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
    UserAgent(PackQueueHolder<T>& agency): Agent<T>(agency, "user") {}

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
    EchoAgent(PackQueueHolder<T>& agency): Agent<T>(agency, "echo") {}

    bool handle(const string& sender, const T& item) override {
        sleep(2); // emulate some background work;
        cout << "Echo: '" << sender << "' -> " << item << endl;
        return true;
    }
};


int safe_main(int argc, char *argv[]) {
    // run_tests();

    try {
        // vector<Agent<string>*> agents;

        PackQueue<string> queue;
        Agency<string> agency(queue);
        agency.spawn<EchoAgent<string>>()->start();
        agency.spawn<UserAgent<string>>()->start();
        agency.run();

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