#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "ChatHistory.hpp"

using namespace std;

namespace tools::chat {

    class Chatbot {
    public:
        Chatbot(
            const string& name, 
            ChatHistory& history, 
            Printer& printer
        ): 
            name(name), 
            history(history),
            printer(printer)
        {}
        // virtual ~Chatbot() {
        //     delete history;
        //     history = nullptr;
        // }
        virtual string chat(const string& sender, const string& text) = 0;
        virtual string chunk(const string& chunk) { 
            printer.print(chunk);
            return chunk;
        }
        virtual string response(const string& response) { return response; }

        ChatHistory& getHistoryRef() { return history; }

        const string name;
    protected:
        ChatHistory& history;
        Printer& printer;
    };

}
