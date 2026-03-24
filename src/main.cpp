#include "loader.hpp"
#include "match.hpp"
#include <iostream>
#include <vector>

int main() {
    try {
        // Create a match instance
        Match match;

        // Start the match by loading both teams
        if (!match.start("stats/india.txt", "stats/england.txt")) {
            std::cerr << "Failed to start match" << std::endl;
            return 1;
        }

        // Conduct the toss
        if (!match.toss()) {
            std::cerr << "Failed to conduct toss" << std::endl;
            return 1;
        }

        // Run the match simulation for 20 overs
        if (!match.run(20)) {
            std::cerr << "Failed to run match" << std::endl;
            return 1;
        }

        std::cout << "\nMatch completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}