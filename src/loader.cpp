#include "loader.hpp"
#include <fstream>
#include <iostream>

std::string TeamLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string TeamLoader::getValue(const std::string& line) {
    size_t pos = line.find('=');
    return (pos != std::string::npos) ? trim(line.substr(pos + 1)) : "";
}

Player* TeamLoader::parsePlayer(const std::vector<std::string>& lines) {
    std::string name, role_str;
    double strike_rate = 0.0, avg = 0.0, economy = 0.0;
    int expected_balls = 0, thread_priority = 5, max_overs = 0;
    bool is_death_specialist = false;
    PlayerStats stats;

    for (const auto& line : lines) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#' || t[0] == '[') continue;

        if (t.find("name=") == 0) name = getValue(t);
        else if (t.find("role=") == 0) role_str = getValue(t);
        else if (t.find("strike_rate=") == 0) strike_rate = std::stod(getValue(t));
        else if (t.find("avg=") == 0) avg = std::stod(getValue(t));
        else if (t.find("expected_balls=") == 0) expected_balls = std::stoi(getValue(t));
        else if (t.find("thread_priority=") == 0) thread_priority = std::stoi(getValue(t));
        else if (t.find("is_death_specialist=") == 0) {
            std::string val = getValue(t);
            is_death_specialist = (val == "true" || val == "1");
        }
        else if (t.find("economy=") == 0) economy = std::stod(getValue(t));
        else if (t.find("max_overs=") == 0) max_overs = std::stoi(getValue(t));
        
        else if (t.find("fitness=") == 0) stats.fitness = std::stod(getValue(t));
        else if (t.find("catching_efficiency=") == 0) stats.catching_efficiency = std::stod(getValue(t));
        else if (t.find("clutch_factor=") == 0) stats.clutch_factor = std::stod(getValue(t));
        else if (t.find("pressure_performance=") == 0) stats.pressure_performance = std::stod(getValue(t));
        else if (t.find("batting=") == 0) stats.batting = std::stod(getValue(t));
        else if (t.find("shot_selection=") == 0) stats.shot_selection = std::stod(getValue(t));
        else if (t.find("power_hitting=") == 0) stats.power_hitting = std::stod(getValue(t));
        else if (t.find("pace_skill=") == 0) stats.pace_skill = std::stod(getValue(t));
        else if (t.find("swing_skill=") == 0) stats.swing_skill = std::stod(getValue(t));
        else if (t.find("spin_skill=") == 0) stats.spin_skill = std::stod(getValue(t));
        else if (t.find("spin_quantity=") == 0) stats.spin_quantity = std::stod(getValue(t));
        else if (t.find("line_accuracy=") == 0) stats.line_accuracy = std::stod(getValue(t));
        else if (t.find("length_control=") == 0) stats.length_control = std::stod(getValue(t));
    }

    if (role_str == "BATSMAN" || role_str == "ALL_ROUNDER" || role_str == "WICKETKEEPER") {
        return new Batsman(name, strike_rate, avg, expected_balls, thread_priority, is_death_specialist, stats);
    } else if (role_str == "BOWLER") {
        return new Bowler(name, strike_rate, avg, expected_balls, economy, max_overs, thread_priority, is_death_specialist, stats);
    }
    
    std::cerr << "Unknown player role: " << role_str << "\n";
    return nullptr;
}
bool TeamLoader::load(const std::string& filepath, std::vector<Player*>& players) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    std::vector<std::string> block;

    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#' || t.find("TEAM ") == 0) continue;

        if (t == "[player]") {
            if (!block.empty()) {
                if (Player* p = parsePlayer(block)) players.push_back(p);
                block.clear();
            }
        }
        block.push_back(t);
    }

    if (!block.empty()) {
        if (Player* p = parsePlayer(block)) players.push_back(p);
    }

    return !players.empty();
}