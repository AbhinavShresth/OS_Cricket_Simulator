#ifndef LOADER_HPP
#define LOADER_HPP

#include "player.hpp"
#include <vector>
#include <string>

class TeamLoader {
public:
    /**
     * Load a team from an INI-format text file
     * @param filepath Path to the team data file (e.g., "stats/india.txt")
     * @param players Vector to populate with Player objects
     * @return true if successfully loaded, false otherwise
     */
    static bool load(const std::string& filepath, std::vector<Player*>& players);

private:
    /**
     * Parse a single player block from INI format
     * @param lines Vector of lines from the [player] block
     * @return Pointer to created Player object (Batsman or Bowler)
     */
    static Player* parsePlayer(const std::vector<std::string>& lines);

    /**
     * Trim whitespace from string
     */
    static std::string trim(const std::string& str);

    /**
     * Get value for a key from INI format
     */
    static std::string getValue(const std::string& line);
};

#endif // LOADER_HPP
