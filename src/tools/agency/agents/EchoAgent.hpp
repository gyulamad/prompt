#pragma once

namespace tools::agency::agents {
    
    template<typename T>
    class EchoAgent: public Agent<T> {
    public:
        EchoAgent(PackQueue<T>& queue): Agent<T>(queue, "echo") {}

        void handle(const string& sender, const T& item) override {
            sleep(2); // emulate some background work;
            cout << "Echo: '" << sender << "' -> " << item << endl;
        }
    };
    
}