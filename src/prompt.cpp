#include "tools/Test.hpp"
// #include "tools/Tasks.hpp"
#include "tools/RingBuffer.hpp"
#include "tools/cmd/Commander.hpp"

using namespace tools;


#include "EventSystem.hpp"
#include <iostream>
#include <string>

namespace Example {

// Define some specific event types
struct TextMessageEvent : public EventSystem::TypedEvent<TextMessageEvent> {
    std::string message;
    std::string channel;  // Allow filtering by channel
    
    TextMessageEvent(const std::string& msg, const std::string& ch) 
        : message(msg), channel(ch) {}
};

struct CommandEvent : public EventSystem::TypedEvent<CommandEvent> {
    std::string command;
    std::vector<std::string> arguments;
    
    CommandEvent(const std::string& cmd, const std::vector<std::string>& args) 
        : command(cmd), arguments(args) {}
};

struct OutputEvent : public EventSystem::TypedEvent<OutputEvent> {
    std::string content;
    bool isError;
    
    OutputEvent(const std::string& c, bool err = false) 
        : content(c), isError(err) {}
};

// A chat user that can send and receive messages
class ChatUser : public EventSystem::BaseEventAgent {
public:
    ChatUser(const std::string& username, const std::vector<std::string>& subscribedChannels) 
        : EventSystem::BaseEventAgent(username),
          m_username(username),
          m_subscribedChannels(subscribedChannels) {}
    
    // Send a message to a specific channel
    void sendMessage(const std::string& message, const std::string& channel) {
        auto event = std::make_shared<TextMessageEvent>(message, channel);
        publishEvent(event);
        std::cout << m_username << " sent message to channel " << channel << ": " << message << std::endl;
    }
    
    // Send a direct message to another user
    void sendDirectMessage(const std::string& message, const std::string& recipientId) {
        auto event = std::make_shared<TextMessageEvent>(message, "direct");
        event->targetId = recipientId;  // Set specific target
        publishEvent(event);
        std::cout << m_username << " sent direct message to " << recipientId << ": " << message << std::endl;
    }

protected:
    void registerEventInterests() override {
        // Register interest in TextMessageEvent
        registerHandler<TextMessageEvent>([this](std::shared_ptr<TextMessageEvent> event) {
            // Filter out messages from channels we haven't subscribed to
            if (event->targetId.empty()) {  // Broadcast message
                if (std::find(m_subscribedChannels.begin(), m_subscribedChannels.end(), 
                             event->channel) != m_subscribedChannels.end()) {
                    std::cout << m_username << " received channel message from " 
                             << event->sourceId << ": " << event->message << std::endl;
                }
            } else if (event->targetId == getId()) {  // Direct message
                std::cout << m_username << " received direct message from " 
                         << event->sourceId << ": " << event->message << std::endl;
            }
        });
    }

private:
    std::string m_username;
    std::vector<std::string> m_subscribedChannels;
};

// A terminal that can run commands and produce output
class Terminal : public EventSystem::BaseEventAgent {
public:
    Terminal(const std::string& id) 
        : EventSystem::BaseEventAgent(id) {}
    
    // Run a command directly
    void runCommand(const std::string& command, const std::vector<std::string>& args) {
        std::cout << getId() << " executing: " << command;
        for (const auto& arg : args) {
            std::cout << " " << arg;
        }
        std::cout << std::endl;
        
        // In a real implementation, you would actually run the command here
        // For this example, we'll just simulate output
        if (command == "echo") {
            std::string output;
            for (const auto& arg : args) {
                output += arg + " ";
            }
            
            auto outputEvent = std::make_shared<OutputEvent>(output);
            publishEvent(outputEvent);
        } else if (command == "error") {
            auto errorEvent = std::make_shared<OutputEvent>("Unknown command", true);
            publishEvent(errorEvent);
        }
    }

protected:
    void registerEventInterests() override {
        // Register interest in CommandEvent
        registerHandler<CommandEvent>([this](std::shared_ptr<CommandEvent> event) {
            std::cout << getId() << " received command from " 
                    << event->sourceId << ": " << event->command << std::endl;
            
            runCommand(event->command, event->arguments);
        });
    }
};

// An AI assistant that can monitor both chat and terminal interactions
class AIAssistant : public EventSystem::BaseEventAgent {
public:
    AIAssistant(const std::string& id, const std::vector<std::string>& monitoredChannels) 
        : EventSystem::BaseEventAgent(id),
          m_monitoredChannels(monitoredChannels) {}
    
    // Respond to a message
    void respond(const std::string& message, const std::string& targetId) {
        auto response = std::make_shared<TextMessageEvent>(message, "direct");
        response->targetId = targetId;
        publishEvent(response);
        std::cout << getId() << " responded to " << targetId << ": " << message << std::endl;
    }
    
    // Execute a command
    void executeCommand(const std::string& command, const std::vector<std::string>& args, 
                        const std::string& terminalId) {
        auto cmdEvent = std::make_shared<CommandEvent>(command, args);
        cmdEvent->targetId = terminalId;  // Target a specific terminal
        publishEvent(cmdEvent);
        std::cout << getId() << " sent command to terminal " << terminalId << ": " << command << std::endl;
    }

protected:
    void registerEventInterests() override {
        // Listen for text messages in monitored channels
        registerHandler<TextMessageEvent>([this](std::shared_ptr<TextMessageEvent> event) {
            // Only process messages in monitored channels or direct messages to us
            if ((event->targetId.empty() && 
                 std::find(m_monitoredChannels.begin(), m_monitoredChannels.end(), 
                          event->channel) != m_monitoredChannels.end()) ||
                event->targetId == getId()) {
                
                std::cout << getId() << " processing message from " 
                         << event->sourceId << ": " << event->message << std::endl;
                
                // Simple response logic - in a real implementation this would be more sophisticated
                if (event->message.find("help") != std::string::npos) {
                    respond("I'm here to help! What do you need?", event->sourceId);
                }
            }
        });
        
        // Listen for terminal output to learn and assist
        registerHandler<OutputEvent>([this](std::shared_ptr<OutputEvent> event) {
            std::cout << getId() << " observed output from " 
                     << event->sourceId << ": " << event->content << std::endl;
            
            if (event->isError) {
                // Offer help for errors
                respond("I noticed an error. Can I help?", event->sourceId);
            }
        });
    }

private:
    std::vector<std::string> m_monitoredChannels;
};

// Example of how to use the system
void RunExample() {
    // Create the central event bus (with async delivery for thread safety)
    auto eventBus = std::make_shared<EventSystem::EventBus>(true);
    
    // Create some users, terminals, and AI
    auto user1 = std::make_shared<ChatUser>("Alice", std::vector<std::string>{"general", "dev"});
    auto user2 = std::make_shared<ChatUser>("Bob", std::vector<std::string>{"general"});
    auto user3 = std::make_shared<ChatUser>("Charlie", std::vector<std::string>{"dev"});
    
    auto terminal1 = std::make_shared<Terminal>("Terminal1");
    auto terminal2 = std::make_shared<Terminal>("Terminal2");
    
    auto assistant = std::make_shared<AIAssistant>("Assistant", std::vector<std::string>{"help"});
    
    // Register all components with the event bus
    user1->registerWithEventBus(eventBus);
    user2->registerWithEventBus(eventBus);
    user3->registerWithEventBus(eventBus);
    terminal1->registerWithEventBus(eventBus);
    terminal2->registerWithEventBus(eventBus);
    assistant->registerWithEventBus(eventBus);
    
    // Simulate some interactions
    user1->sendMessage("Hello everyone!", "general");  // Both user2 and user1 will receive this
    user1->sendMessage("Any devs online?", "dev");     // Only user3 and user1 will receive this
    user1->sendDirectMessage("Hey Bob, got a minute?", "Bob");  // Only user2 will receive this
    
    user2->sendMessage("I need help with something", "help");  // The AI assistant monitors this channel
    
    // AI responds and interacts with a terminal
    assistant->executeCommand("echo", {"Hello", "from", "AI"}, "Terminal1");
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Run the example with the filtered event bus
void RunFilteredExample() {
    // Create a logger for the event system
    auto logger = std::make_shared<tools::Logger>("EventSystem", "event_system.log");
    logger->setMinLogLevel(tools::Logger::Level::DEBUG);
    
    // Create the filtered event bus (with async delivery for thread safety)
    auto eventBus = std::make_shared<EventSystem::FilteredEventBus>(true, logger);
    
    // Get the self-message filter and configure it
    auto selfFilter = eventBus->getSelfMessageFilter();
    
    // Create some users
    auto user1 = std::make_shared<ChatUser>("Alice", std::vector<std::string>{"general", "dev"});
    auto user2 = std::make_shared<ChatUser>("Bob", std::vector<std::string>{"general"});
    auto user3 = std::make_shared<ChatUser>("Charlie", std::vector<std::string>{"dev"});
    
    // Register all components with the event bus
    user1->registerWithEventBus(eventBus);
    user2->registerWithEventBus(eventBus);
    user3->registerWithEventBus(eventBus);
    
    std::cout << "\n=== With self-message filtering DISABLED ===\n" << std::endl;
    
    // Initially, self-message filtering is disabled (default)
    selfFilter->setFilterSelfMessages(false);
    
    // Simulate some interactions (Alice will see her own messages)
    user1->sendMessage("Hello everyone!", "general");
    user1->sendMessage("Any devs online?", "dev");
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "\n=== With self-message filtering ENABLED ===\n" << std::endl;
    
    // Enable self-message filtering
    selfFilter->setFilterSelfMessages(true);
    
    // Simulate the same interactions (Alice will NOT see her own messages)
    user1->sendMessage("Hello everyone again!", "general");
    user1->sendMessage("Any devs responding?", "dev");
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create a custom filter (for example purposes)
    class ChannelFilter : public EventSystem::EventFilter {
    public:
        ChannelFilter(const std::string& blockedChannel) : m_blockedChannel(blockedChannel) {}
        
        bool shouldDeliverEvent(const EventSystem::ComponentId& consumerId, 
                               std::shared_ptr<EventSystem::Event> event) override {
            // Try to cast to TextMessageEvent
            auto textEvent = std::dynamic_pointer_cast<TextMessageEvent>(event);
            if (textEvent) {
                // Filter out messages from the blocked channel
                return textEvent->channel != m_blockedChannel;
            }
            return true; // Allow other event types
        }
    private:
        std::string m_blockedChannel;
    };
    
    std::cout << "\n=== With custom channel filtering ===\n" << std::endl;
    
    // Add a custom filter to block the "general" channel
    auto channelFilter = std::make_shared<ChannelFilter>("general");
    eventBus->addEventFilter(channelFilter);
    
    // Simulate messages (general channel messages will be filtered)
    user1->sendMessage("This is from general channel", "general");
    user1->sendMessage("This is from dev channel", "dev");
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "\n=== After clearing all filters ===\n" << std::endl;
    
    // Clear all filters
    eventBus->clearFilters();
    selfFilter->setFilterSelfMessages(false);
    
    // Simulate messages (all messages should be delivered)
    user1->sendMessage("After clearing filters (general)", "general");
    user1->sendMessage("After clearing filters (dev)", "dev");
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

} // namespace Example


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