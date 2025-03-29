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
        virtual ~Chatbot() {}
        virtual string chat(const string& sender, const string& text) = 0;
        virtual string chunk(const string& chunk) { 
            printer.print(chunk);
            return chunk;
        }
        virtual string response(const string& response) { return response; }
    protected:
        string name;
        ChatHistory& history;
        Printer& printer;
    };

}