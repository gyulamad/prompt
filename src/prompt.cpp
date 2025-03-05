#include "tools/tools.hpp"
#include "tools/cmd/cmd.hpp"
#include "tools/voice/voice.hpp"
#include "tools/events/events.hpp"

using namespace std;
using namespace tools;
using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::events;


#include <iostream>
#include <string>

// namespace Example {

// // Define some specific event types
// struct TextMessageEvent : public TypedEvent<TextMessageEvent> {
//     string message;
//     string channel;  // Allow filtering by channel
    
//     TextMessageEvent(const string& msg, const string& ch) 
//         : message(msg), channel(ch) {}
// };

// struct CommandEvent : public TypedEvent<CommandEvent> {
//     string command;
//     vector<string> arguments;
    
//     CommandEvent(const string& cmd, const vector<string>& args) 
//         : command(cmd), arguments(args) {}
// };

// struct OutputEvent : public TypedEvent<OutputEvent> {
//     string content;
//     bool isError;
    
//     OutputEvent(const string& c, bool err = false) 
//         : content(c), isError(err) {}
// };

// // A chat user that can send and receive messages
// class ChatUser : public BaseEventAgent {
// public:
//     ChatUser(const string& username, const vector<string>& subscribedChannels) 
//         : BaseEventAgent(username),
//           m_username(username),
//           m_subscribedChannels(subscribedChannels) {}
    
//     // Send a message to a specific channel
//     void sendMessage(const string& message, const string& channel) {
//         auto event = make_shared<TextMessageEvent>(message, channel);
//         publishEvent(event);
//         cout << m_username << " sent message to channel " << channel << ": " << message << endl;
//     }
    
//     // Send a direct message to another user
//     void sendDirectMessage(const string& message, const string& recipientId) {
//         auto event = make_shared<TextMessageEvent>(message, "direct");
//         event->targetId = recipientId;  // Set specific target
//         publishEvent(event);
//         cout << m_username << " sent direct message to " << recipientId << ": " << message << endl;
//     }

// protected:
//     void registerEventInterests() override {
//         // Register interest in TextMessageEvent
//         registerHandler<TextMessageEvent>([this](shared_ptr<TextMessageEvent> event) {
//             // Filter out messages from channels we haven't subscribed to
//             if (event->targetId.empty()) {  // Broadcast message
//                 if (find(m_subscribedChannels.begin(), m_subscribedChannels.end(), 
//                              event->channel) != m_subscribedChannels.end()) {
//                     cout << m_username << " received channel message from " 
//                              << event->sourceId << ": " << event->message << endl;
//                 }
//             } else if (event->targetId == getId()) {  // Direct message
//                 cout << m_username << " received direct message from " 
//                          << event->sourceId << ": " << event->message << endl;
//             }
//         });
//     }

// private:
//     string m_username;
//     vector<string> m_subscribedChannels;
// };

// // A terminal that can run commands and produce output
// class Terminal : public BaseEventAgent {
// public:
//     Terminal(const string& id) 
//         : BaseEventAgent(id) {}
    
//     // Run a command directly
//     void runCommand(const string& command, const vector<string>& args) {
//         cout << getId() << " executing: " << command;
//         for (const auto& arg : args) {
//             cout << " " << arg;
//         }
//         cout << endl;
        
//         // In a real implementation, you would actually run the command here
//         // For this example, we'll just simulate output
//         if (command == "echo") {
//             string output;
//             for (const auto& arg : args) {
//                 output += arg + " ";
//             }
            
//             auto outputEvent = make_shared<OutputEvent>(output);
//             publishEvent(outputEvent);
//         } else if (command == "error") {
//             auto errorEvent = make_shared<OutputEvent>("Unknown command", true);
//             publishEvent(errorEvent);
//         }
//     }

// protected:
//     void registerEventInterests() override {
//         // Register interest in CommandEvent
//         registerHandler<CommandEvent>([this](shared_ptr<CommandEvent> event) {
//             cout << getId() << " received command from " 
//                     << event->sourceId << ": " << event->command << endl;
            
//             runCommand(event->command, event->arguments);
//         });
//     }
// };

// // An AI assistant that can monitor both chat and terminal interactions
// class AIAssistant : public BaseEventAgent {
// public:
//     AIAssistant(const string& id, const vector<string>& monitoredChannels) 
//         : BaseEventAgent(id),
//           m_monitoredChannels(monitoredChannels) {}
    
//     // Respond to a message
//     void respond(const string& message, const string& targetId) {
//         auto response = make_shared<TextMessageEvent>(message, "direct");
//         response->targetId = targetId;
//         publishEvent(response);
//         cout << getId() << " responded to " << targetId << ": " << message << endl;
//     }
    
//     // Execute a command
//     void executeCommand(const string& command, const vector<string>& args, 
//                         const string& terminalId) {
//         auto cmdEvent = make_shared<CommandEvent>(command, args);
//         cmdEvent->targetId = terminalId;  // Target a specific terminal
//         publishEvent(cmdEvent);
//         cout << getId() << " sent command to terminal " << terminalId << ": " << command << endl;
//     }

// protected:
//     void registerEventInterests() override {
//         // Listen for text messages in monitored channels
//         registerHandler<TextMessageEvent>([this](shared_ptr<TextMessageEvent> event) {
//             // Only process messages in monitored channels or direct messages to us
//             if ((event->targetId.empty() && 
//                  find(m_monitoredChannels.begin(), m_monitoredChannels.end(), 
//                           event->channel) != m_monitoredChannels.end()) ||
//                 event->targetId == getId()) {
                
//                 cout << getId() << " processing message from " 
//                          << event->sourceId << ": " << event->message << endl;
                
//                 // Simple response logic - in a real implementation this would be more sophisticated
//                 if (event->message.find("help") != string::npos) {
//                     respond("I'm here to help! What do you need?", event->sourceId);
//                 }
//             }
//         });
        
//         // Listen for terminal output to learn and assist
//         registerHandler<OutputEvent>([this](shared_ptr<OutputEvent> event) {
//             cout << getId() << " observed output from " 
//                      << event->sourceId << ": " << event->content << endl;
            
//             if (event->isError) {
//                 // Offer help for errors
//                 respond("I noticed an error. Can I help?", event->sourceId);
//             }
//         });
//     }

// private:
//     vector<string> m_monitoredChannels;
// };

// // Example of how to use the system
// void RunExample() {
//     // Create the central event bus (with async delivery for thread safety)
//     auto eventBus = make_shared<EventBus>(true);
    
//     // Create some users, terminals, and AI
//     auto user1 = make_shared<ChatUser>("Alice", vector<string>{"general", "dev"});
//     auto user2 = make_shared<ChatUser>("Bob", vector<string>{"general"});
//     auto user3 = make_shared<ChatUser>("Charlie", vector<string>{"dev"});
    
//     auto terminal1 = make_shared<Terminal>("Terminal1");
//     auto terminal2 = make_shared<Terminal>("Terminal2");
    
//     auto assistant = make_shared<AIAssistant>("Assistant", vector<string>{"help"});
    
//     // Register all components with the event bus
//     user1->registerWithEventBus(eventBus);
//     user2->registerWithEventBus(eventBus);
//     user3->registerWithEventBus(eventBus);
//     terminal1->registerWithEventBus(eventBus);
//     terminal2->registerWithEventBus(eventBus);
//     assistant->registerWithEventBus(eventBus);
    
//     // Simulate some interactions
//     user1->sendMessage("Hello everyone!", "general");  // Both user2 and user1 will receive this
//     user1->sendMessage("Any devs online?", "dev");     // Only user3 and user1 will receive this
//     user1->sendDirectMessage("Hey Bob, got a minute?", "Bob");  // Only user2 will receive this
    
//     user2->sendMessage("I need help with something", "help");  // The AI assistant monitors this channel
    
//     // AI responds and interacts with a terminal
//     assistant->executeCommand("echo", {"Hello", "from", "AI"}, "Terminal1");
    
//     // Give time for async processing
//     this_thread::sleep_for(chrono::milliseconds(100));
// }

// // Run the example with the filtered event bus
// void RunFilteredExample() {
//     // Create a logger for the event system
//     auto logger = make_shared<tools::Logger>("EventSystem", "event_system.log");
//     logger->setMinLogLevel(tools::Logger::Level::DEBUG);
    
//     // Create the filtered event bus (with async delivery for thread safety)
//     auto eventBus = make_shared<FilteredEventBus>(true, logger);
    
//     // Get the self-message filter and configure it
//     auto selfFilter = eventBus->getSelfMessageFilter();
    
//     // Create some users
//     auto user1 = make_shared<ChatUser>("Alice", vector<string>{"general", "dev"});
//     auto user2 = make_shared<ChatUser>("Bob", vector<string>{"general"});
//     auto user3 = make_shared<ChatUser>("Charlie", vector<string>{"dev"});
    
//     // Register all components with the event bus
//     user1->registerWithEventBus(eventBus);
//     user2->registerWithEventBus(eventBus);
//     user3->registerWithEventBus(eventBus);
    
//     cout << "\n=== With self-message filtering DISABLED ===\n" << endl;
    
//     // Initially, self-message filtering is disabled (default)
//     selfFilter->setFilterSelfMessages(false);
    
//     // Simulate some interactions (Alice will see her own messages)
//     user1->sendMessage("Hello everyone!", "general");
//     user1->sendMessage("Any devs online?", "dev");
    
//     // Give time for async processing
//     this_thread::sleep_for(chrono::milliseconds(100));
    
//     cout << "\n=== With self-message filtering ENABLED ===\n" << endl;
    
//     // Enable self-message filtering
//     selfFilter->setFilterSelfMessages(true);
    
//     // Simulate the same interactions (Alice will NOT see her own messages)
//     user1->sendMessage("Hello everyone again!", "general");
//     user1->sendMessage("Any devs responding?", "dev");
    
//     // Give time for async processing
//     this_thread::sleep_for(chrono::milliseconds(100));
    
//     // Create a custom filter (for example purposes)
//     class ChannelFilter : public EventFilter {
//     public:
//         ChannelFilter(const string& blockedChannel) : m_blockedChannel(blockedChannel) {}
        
//         bool shouldDeliverEvent(const ComponentId& consumerId, 
//                                shared_ptr<Event> event) override {
//             // Try to cast to TextMessageEvent
//             auto textEvent = dynamic_pointer_cast<TextMessageEvent>(event);
//             if (textEvent) {
//                 // Filter out messages from the blocked channel
//                 return textEvent->channel != m_blockedChannel;
//             }
//             return true; // Allow other event types
//         }
//     private:
//         string m_blockedChannel;
//     };
    
//     cout << "\n=== With custom channel filtering ===\n" << endl;
    
//     // Add a custom filter to block the "general" channel
//     auto channelFilter = make_shared<ChannelFilter>("general");
//     eventBus->addEventFilter(channelFilter);
    
//     // Simulate messages (general channel messages will be filtered)
//     user1->sendMessage("This is from general channel", "general");
//     user1->sendMessage("This is from dev channel", "dev");
    
//     // Give time for async processing
//     this_thread::sleep_for(chrono::milliseconds(100));
    
//     cout << "\n=== After clearing all filters ===\n" << endl;
    
//     // Clear all filters
//     eventBus->clearFilters();
//     selfFilter->setFilterSelfMessages(false);
    
//     // Simulate messages (all messages should be delivered)
//     user1->sendMessage("After clearing filters (general)", "general");
//     user1->sendMessage("After clearing filters (dev)", "dev");
    
//     // Give time for async processing
//     this_thread::sleep_for(chrono::milliseconds(100));
// }

// } // namespace Example


// ---------------------------------------



#ifdef TEST
using namespace tools;

#endif

int main(int argc, char *argv[]) {
    run_tests();

    // Example::RunExample();
    // Example::RunFilteredExample();



    return 0;
}