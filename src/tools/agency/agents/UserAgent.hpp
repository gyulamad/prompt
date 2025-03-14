#pragma once

namespace tools::agency::agents {
    
    template<typename T>
    class UserAgent: public Agent<T> {
    public:
        UserAgent(PackQueue<T>& queue): Agent<T>(queue, "user") {}

        void tick() override {
            T input;
            cin >> input;
            if (input == "exit") this->send("agency", "exit");
            else if (input == "list") this->send("agency", "list");
            else this->send("echo", input);
        }
    };
    
}