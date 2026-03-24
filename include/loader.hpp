#ifndef LOADER_HPP
#define LOADER_HPP

#include "player.hpp"
#include <string>
#include <vector>

class TeamLoader {
private:
    static std::string trim(const std::string& str);
    static std::string getValue(const std::string& line);
    static Player* parsePlayer(const std::vector<std::string>& lines);

public:
    static bool load(const std::string& filepath, std::vector<Player*>& players);
};

#endif 