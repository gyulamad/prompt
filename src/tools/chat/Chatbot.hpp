#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "ChatHistory.hpp"

using namespace std;

namespace tools::chat {

    class Chatbot {
    public:

        class cancel: public exception {};

        Chatbot(
            Owns& owns,
            const string& name,
            void* history, 
            Printer& printer
        ): 
            owns(owns),
            name(name),
            history(owns.reserve<void>(this, history, FILELN)),
            printer(printer)
        {}

        virtual ~Chatbot() {
            // cout << "Chatbot (" + this->name + ") destruction..." << endl; 
            // histories.release(this, history);
            owns.release(this, history);
        }

        void* getHistoryPtr() { return history; } // TODO: remove this

        // ----- stream chat -----

        virtual string chat(const string& sender, const string& text, bool& interrupted) = 0;

        virtual string chunk(const string& chunk) {
            STUB_VIRTUAL // TODO...
            printer.print(chunk);
            return chunk;
        }

        virtual string response(const string& response) {
            STUB_VIRTUAL // TODO...
            return response; 
        }

        // ----- completion -----

        virtual string respond(const string& sender, const string& text) = 0;

        const string name; //  TODO: remove this!
    protected:
        Owns& owns;
        void* history = nullptr;
        Printer& printer;
    };

    

}
