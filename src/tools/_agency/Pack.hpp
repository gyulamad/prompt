#pragma once

#include <string>

using namespace std;

namespace tools::agency {

    template<typename T>
    class Pack {
    public:
        Pack(
            T item = T(), 
            string sender = "", 
            string recipient = ""
        ): 
            item(item),
            sender(sender),
            recipient(recipient)
        {}

        T getItem() const { return item; }
        string getSender() const { return sender; }
        string getRecipient() const { return recipient; }

    private:
        T item;
        string sender;
        string recipient;
    };

}
