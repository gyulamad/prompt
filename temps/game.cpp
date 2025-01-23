#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>


// Forward declarations
class Game;
class Player;
class Location;
class Item;
class NetworkManager;
class CommandHandler;


// NetworkManager interface (abstract base class)
class NetworkManager {
public:
    virtual ~NetworkManager() = default;
    virtual void sendMessage(const std::string& message, int playerID) = 0;
    virtual std::string receiveMessage(int playerID) = 0;
};


// Player class
class Player {
public:
    std::string name;
    int id;
    int score;
    int health;
    int currentLocation;
    std::vector<std::string> inventory;

    Player(std::string name, int id, int location) : name(std::move(name)), id(id), score(0), health(100), currentLocation(location) {}

    void addToInventory(const std::string& item) { inventory.push_back(item); }

    void removeFromInventory(const std::string& item) {
        inventory.erase(std::remove(inventory.begin(), inventory.end(), item), inventory.end());
    }

    bool hasItem(const std::string& item) const {
        return std::find(inventory.begin(), inventory.end(), item) != inventory.end();
    }
};


// Location class - simplified connection management
class Location {
public:
    std::string name;
    std::string description;
    std::map<std::string, int> connections; //Direct connection by name
    std::vector<std::string> items;

    Location(std::string name, std::string description) : name(std::move(name)), description(std::move(description)) {}

    void addItem(const std::string& item) { items.push_back(item); }

    void removeItem(const std::string& item) {
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
    }
    bool hasItem(const std::string& item) const {
        return std::find(items.begin(), items.end(), item) != items.end();
    }
    bool isConnectedTo(const std::string& locationName) const{
        return connections.count(locationName);
    }
};


// Item class
class Item {
public:
    std::string name;
    std::string description;
    int damage; // For combat

    Item(std::string name, std::string description, int damage = 0) : name(std::move(name)), description(std::move(description)), damage(damage) {}
};


// Result class for command handling
struct CommandResult {
    // bool success;
    std::string message; // Removed optional, always provide a message
};


// Abstract base class for command handlers
class CommandHandler {
public:
    virtual ~CommandHandler() = default;
    virtual CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) = 0;
};


// Concrete command handlers (one for each command type)
class GoCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class TakeCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class UseCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class TalkCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class LookCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class InventoryCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};

class AttackCommandHandler : public CommandHandler {
public:
    CommandResult handle(Game& game, Player& player, const std::vector<std::string>& args) override;
};


// Game class
class Game {
public:
    std::vector<Player> players;
    std::vector<Location> locations;
    std::map<std::string, Item> items;
    std::unique_ptr<NetworkManager> networkManager;
    std::map<std::string, std::unique_ptr<CommandHandler>> commandHandlers;

    Game(std::unique_ptr<NetworkManager> networkManager) : networkManager(std::move(networkManager)) {
        registerCommandHandler("go", std::make_unique<GoCommandHandler>());
        registerCommandHandler("take", std::make_unique<TakeCommandHandler>());
        registerCommandHandler("use", std::make_unique<UseCommandHandler>());
        registerCommandHandler("talk", std::make_unique<TalkCommandHandler>());
        registerCommandHandler("look", std::make_unique<LookCommandHandler>());
        registerCommandHandler("inventory", std::make_unique<InventoryCommandHandler>());
        registerCommandHandler("attack", std::make_unique<AttackCommandHandler>());
    }

    void addPlayer(const std::string& name) {
        players.emplace_back(name, players.size(), 0);
    }

    void handleCommand(Player& player, const std::string& line) {
        std::stringstream ss(line);
        std::string command;
        ss >> command;
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command.empty()) return; // Ignore empty lines

        auto it = commandHandlers.find(command);
        if (it == commandHandlers.end()) {
            sendMessage("Invalid command: " + command, player.id);
            return;
        }

        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) args.push_back(arg);

        CommandResult result = it->second->handle(*this, player, args);
        sendMessage(result.message, player.id);
    }

    void sendMessage(const std::string& message, int playerID, const std::string& sender = "") const {
        if (networkManager) networkManager->sendMessage((sender.empty() ? "" : sender + ": ") + message, playerID);
        else std::cerr << "NetworkManager not initialized!\n";
    }

    Player* getPlayer(int id) const {
        for (const auto& p : players) {
            if (p.id == id) return const_cast<Player*>(&p);
        }
        return nullptr;
    }

    Player* getPlayerByName(const std::string& name) const {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        for (const auto& p : players) {
            std::string lowerPlayerName = p.name;
            std::transform(lowerPlayerName.begin(), lowerPlayerName.end(), lowerPlayerName.begin(), ::tolower);
            if (lowerPlayerName == lowerName) return const_cast<Player*>(&p);
        }
        return nullptr;
    }

    Location* getLocation(int id) const {
        if (id < 0 || id >= locations.size()) return nullptr;
        return const_cast<Location*>(&locations[id]);
    }

    Location* getLocationByName(const std::string& name) const {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        for (const auto& loc : locations) {
            std::string lowerLocName = loc.name;
            std::transform(lowerLocName.begin(), lowerLocName.end(), lowerLocName.begin(), ::tolower);
            if (lowerLocName == lowerName) return const_cast<Location*>(&loc);
        }
        return nullptr;
    }

    Item* getItemByName(const std::string& name) const {
        auto it = items.find(name);
        return (it == items.end()) ? nullptr : const_cast<Item*>(&(it->second));
    }

    void registerCommandHandler(const std::string& command, std::unique_ptr<CommandHandler> handler) {
        commandHandlers[command] = std::move(handler);
    }
};


// Implementations of command handlers
CommandResult GoCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (args.empty()) return {"Go where?"};
    Location* targetLocation = game.getLocationByName(args[0]);
    if (!targetLocation) return {"Location '" + args[0] + "' not found."};
    if (!game.locations[player.currentLocation].isConnectedTo(targetLocation->name)) return {"You cannot go there."};
    player.currentLocation = std::distance(game.locations.begin(), std::find(game.locations.begin(), game.locations.end(), *targetLocation));
    return {"You went to " + targetLocation->name + "."};
}


CommandResult TakeCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (args.empty()) return {"Take what?"};
    std::string itemName = args[0];
    if (!game.locations[player.currentLocation].hasItem(itemName)) return {"That item isn't here."};
    player.addToInventory(itemName);
    game.locations[player.currentLocation].removeItem(itemName);
    return {"You took the " + itemName + "."};
}


CommandResult UseCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (args.empty()) return {"Use what?"};
    // Add your logic here.
    return {"Not yet implemented."};
}

CommandResult TalkCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (args.empty()) return {"Talk to whom?"};
    Player* targetPlayer = game.getPlayerByName(args[0]);
    if (!targetPlayer || targetPlayer->currentLocation != player.currentLocation) return {"That player is not here."};
    //Implementation of talking logic
    return {"Not yet implemented."};
}


CommandResult LookCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    std::string message = game.locations[player.currentLocation].description + "\n";
    if (!game.locations[player.currentLocation].items.empty()) {
        message += "Items here:\n";
        for (const auto& item : game.locations[player.currentLocation].items) {
            message += "- " + item + "\n";
        }
    }
    return {message};
}


CommandResult InventoryCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (player.inventory.empty()) return {"Your inventory is empty."};
    std::string message = "Your Inventory:\n";
    for (const auto& item : player.inventory) {
        message += "- " + item + "\n";
    }
    return {message};
}

CommandResult AttackCommandHandler::handle(Game& game, Player& player, const std::vector<std::string>& args) {
    if (args.empty()) return {"Attack who?"};
    Player* target = game.getPlayerByName(args[0]);
    if (!target || target->id == player.id || target->currentLocation != player.currentLocation) return {"Cannot attack that target."};
    Item* weapon = game.getItemByName(player.inventory.empty() ? "" : player.inventory[0]);
    if (!weapon) return {"You have no weapon to attack with."};
    int damageDealt = weapon->damage;
    target->health -= damageDealt;
    std::string message = "You attacked " + target->name + " with " + weapon->name + " dealing " + std::to_string(damageDealt) + " damage!";
    game.sendMessage(message, player.id);
    game.sendMessage(player.name + " attacked you dealing " + std::to_string(damageDealt) + " damage!", target->id);
    if (target->health <= 0) {
        game.sendMessage(target->name + " has been defeated!", player.id);
        game.sendMessage("You have been defeated!", target->id);
        //Handle player death (remove from game, award points etc.)
    }
    return {""};
}



// Example SimpleNetworkManager (replace with a real implementation using Boost.Asio or RakNet)
class SimpleNetworkManager : public NetworkManager {
public:
    void sendMessage(const std::string& message, int playerID) override {
        std::cout << "Player " << playerID << ": " << message << std::endl;
    }
    std::string receiveMessage(int playerID) override { return ""; }
};


int main() {
    auto networkManager = std::make_unique<SimpleNetworkManager>();
    Game game(std::move(networkManager));

    // Initialize locations, items, players
    game.locations.emplace_back("Forest", "A dark and mysterious forest.");
    game.locations.emplace_back("Cave", "A dark and spooky cave.");
    game.locations[0].connections["cave"] = 1;
    game.locations[1].connections["forest"] = 0;
    game.items["Sword"] = Item("Sword", "A sharp sword.", 20);
    game.locations[0].addItem("Sword");
    game.addPlayer("Alice");
    game.addPlayer("Bob");


    // Game loop - improved responsiveness
    while (true) {
        for (auto& player : game.players) {
            std::string line;
            std::cout << player.name << "> ";
            std::getline(std::cin, line);
            game.handleCommand(player, line);
        }
    }
    return 0;
}