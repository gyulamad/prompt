#pragma once

namespace tools::agency::agents {
    
    template<typename T>
    class UserAgent: public Agent<T> {
    public:

        UserAgent(PackQueue<T>& queue, Agency<T>& agency, Commander* commander = nullptr): 
            Agent<T>(queue, "user"), agency(agency), commander(commander) {}

        void tick() override {
            T input;
            if (commander) {
                CommandLine& cline = commander->get_command_line_ref();
                input = cline.readln();
                if (cline.is_exited()) this->exit();
            } else cin >> input;
            if (trim(input).empty()) return;
            else if (str_starts_with(input, "/")) commander->run_command(&agency, input); // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/" 
            else this->send("echo", input); // TODO: forward to default agent (now it's echo agent but should be parametized)
        }

        Commander* getCommanderPtr() { return commander; }
        
    private:
        Agency<T>& agency;
        Commander* commander = nullptr;
    };
    
}