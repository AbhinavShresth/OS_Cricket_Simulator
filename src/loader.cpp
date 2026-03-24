#include "loader.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

std::string TeamLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string TeamLoader::getValue(const std::string& line) {
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
        return trim(line.substr(pos + 1));
    }
    return "";
}

Player* TeamLoader::parsePlayer(const std::vector<std::string>& lines) {
    std::string name;
    std::string role_str;
    double strike_rate = 0.0;
    double avg = 0.0;
    int expected_balls = 0;
    int thread_priority = 5;
    bool is_death_specialist = false;
    double economy = 0.0;
    int max_overs = 0;

    // Parse each line in the player block
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        if (trimmed.find("name=") != std::string::npos) {
            name = getValue(trimmed);
        } else if (trimmed.find("role=") != std::string::npos) {
            role_str = getValue(trimmed);
        } else if (trimmed.find("strike_rate=") != std::string::npos) {
            strike_rate = std::stod(getValue(trimmed));
        } else if (trimmed.find("avg=") != std::string::npos) {
            avg = std::stod(getValue(trimmed));
        } else if (trimmed.find("expected_balls=") != std::string::npos) {
            expected_balls = std::stoi(getValue(trimmed));
        } else if (trimmed.find("thread_priority=") != std::string::npos) {
            thread_priority = std::stoi(getValue(trimmed));
        } else if (trimmed.find("is_death_specialist=") != std::string::npos) {
            std::string val = getValue(trimmed);
            is_death_specialist = (val == "true" || val == "True" || val == "TRUE" || val == "1");
        } else if (trimmed.find("economy=") != std::string::npos) {
            economy = std::stod(getValue(trimmed));
        } else if (trimmed.find("max_overs=") != std::string::npos) {
            max_overs = std::stoi(getValue(trimmed));
        }
    }

    // Create appropriate player object based on role
    if (role_str == "BATSMAN") {
        return new Batsman(name, strike_rate, avg, expected_balls, thread_priority, is_death_specialist);
    } else if (role_str == "BOWLER") {
        return new Bowler(name, strike_rate, avg, expected_balls, economy, max_overs, thread_priority, is_death_specialist);
    } else if (role_str == "ALL_ROUNDER") {
        // For all-rounders, create as Batsman for now (can be extended later)
        return new Batsman(name, strike_rate, avg, expected_balls, thread_priority, is_death_specialist);
    } else if (role_str == "WICKETKEEPER") {
        // Wicketkeepers are treated as batsmen
        return new Batsman(name, strike_rate, avg, expected_balls, thread_priority, is_death_specialist);
    }

    std::cerr << "Warning: Unknown player role: " << role_str << std::endl;
    return nullptr;
}

bool TeamLoader::load(const std::string& filepath, std::vector<Player*>& players) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    std::vector<std::string> current_player_block;
    bool in_player_block = false;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);

        // Skip empty lines and comments
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // Check for TEAM line
        if (trimmed.find("TEAM ") == 0) {
            // Reset for new team (though we only load one team per file)
            continue;
        }

        // Check for start of player block
        if (trimmed == "[player]") {
            // If we were in a previous block, parse it
            if (in_player_block && !current_player_block.empty()) {
                Player* player = parsePlayer(current_player_block);
                if (player != nullptr) {
                    players.push_back(player);
                }
            }
            current_player_block.clear();
            in_player_block = true;
            continue;
        }

        // If we're in a player block, collect lines
        if (in_player_block) {
            current_player_block.push_back(trimmed);
        }
    }

    // Parse the last player block if it exists
    if (in_player_block && !current_player_block.empty()) {
        Player* player = parsePlayer(current_player_block);
        if (player != nullptr) {
            players.push_back(player);
        }
    }

    file.close();

    if (players.empty()) {
        std::cerr << "Warning: No players loaded from " << filepath << std::endl;
        return false;
    }

    std::cout << "Loaded " << players.size() << " players from " << filepath << std::endl;
    return true;
}
