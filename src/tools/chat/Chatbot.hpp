#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "ChatHistory.hpp"

using namespace std;

namespace tools::chat {

    class Chatbot {
    public:
        Chatbot(
            Owns& owns,
            const string& name,
            void* history, 
            Printer& printer
        ): 
            owns(owns),
            name(name),
            history(owns.reserve(this, history, FILELN)),
            printer(printer)
        {}

        virtual ~Chatbot() {
            // cout << "Chatbot (" + this->name + ") destruction..." << endl; 
            // histories.release(this, history);
            owns.release(this, history);
        }

        virtual string chat(const string& sender, const string& text) = 0;

        virtual string chunk(const string& chunk) { 
            printer.print(chunk);
            return chunk;
        }

        virtual string response(const string& response) { return response; }

        void* getHistoryPtr() { return history; } // TODO: remove this


        const string name; //  TODO: remove this!
    protected:
        Owns& owns;
        void* history = nullptr;
        Printer& printer;
    };

}
