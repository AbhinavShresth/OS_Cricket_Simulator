#include "match.hpp"
#include "loader.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

Match::Match() {
    // Constructor
}

Match::~Match() {
    cleanup();
}

bool Match::start(const std::string& india_path, const std::string& england_path) {
    std::cout << "\n=== Starting Match Setup ===" << std::endl;
    
    // Load both teams using TeamLoader
    bool india_loaded = TeamLoader::load(india_path, india_team);
    bool england_loaded = TeamLoader::load(england_path, england_team);

    if (!india_loaded || !england_loaded) {
        std::cerr << "Error: Failed to load one or both teams" << std::endl;
        return false;
    }

    std::cout << "India team: " << india_team.size() << " players" << std::endl;
    std::cout << "England team: " << england_team.size() << " players" << std::endl;

    // Print loaded players
    std::cout << "\n--- India Team ---" << std::endl;
    for (size_t i = 0; i < india_team.size(); ++i) {
        std::cout << (i + 1) << ". " << india_team[i]->getName() 
                  << " (" << static_cast<int>(india_team[i]->getRole()) << ")" << std::endl;
    }

    std::cout << "\n--- England Team ---" << std::endl;
    for (size_t i = 0; i < england_team.size(); ++i) {
        std::cout << (i + 1) << ". " << england_team[i]->getName() 
                  << " (" << static_cast<int>(england_team[i]->getRole()) << ")" << std::endl;
    }

    return true;
}

bool Match::toss() {
    std::cout << "\n=== Conducting Toss ===" << std::endl;

    // Simulate a random coin toss
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    int toss_result = dis(gen);

    if (toss_result == 0) {
        // India wins toss, chooses to bat
        batting_team = india_team;
        bowling_team = england_team;
        batting_team_name = "India";
        bowling_team_name = "England";
        std::cout << "India wins the toss and chooses to bat!" << std::endl;
    } else {
        // England wins toss, chooses to bat
        batting_team = england_team;
        bowling_team = india_team;
        batting_team_name = "England";
        bowling_team_name = "India";
        std::cout << "England wins the toss and chooses to bat!" << std::endl;
    }

    std::cout << batting_team_name << " will bat first" << std::endl;
    std::cout << bowling_team_name << " will bowl first" << std::endl;

    return true;
}

bool Match::run(int num_overs) {
    std::cout << "\n=== Starting Match Simulation ===" << std::endl;
    std::cout << "Target: " << num_overs << " overs" << std::endl;
    std::cout << batting_team_name << " vs " << bowling_team_name << std::endl;

    target_overs = num_overs;
    total_runs = 0;
    wickets = 0;
    current_over = 0;
    current_ball = 0;

    // Initialize striker and non-striker (first two batsmen)
    if (batting_team.size() >= 2) {
        striker = dynamic_cast<Batsman*>(batting_team[0]);
        non_striker = dynamic_cast<Batsman*>(batting_team[1]);
    } else {
        std::cerr << "Error: Not enough batsmen in batting team" << std::endl;
        return false;
    }

    // Initialize bowler (first bowler)
    current_bowler = dynamic_cast<Bowler*>(bowling_team[0]);
    if (current_bowler == nullptr) {
        // If first player is not a bowler, find the first bowler
        for (auto player : bowling_team) {
            if (player->getRole() == PlayerRole::BOWLER) {
                current_bowler = dynamic_cast<Bowler*>(player);
                break;
            }
        }
    }

    if (current_bowler == nullptr) {
        std::cerr << "Error: No bowlers in bowling team" << std::endl;
        return false;
    }

    std::cout << "\nStrike: " << striker->getName() << std::endl;
    std::cout << "Non-Strike: " << non_striker->getName() << std::endl;
    std::cout << "Bowling: " << current_bowler->getName() << std::endl;

    // Main match loop
    while (current_over < target_overs && wickets < 10) {
        std::cout << "\n--- Over " << (current_over + 1) << " ---" << std::endl;

        // Simulate 6 balls in an over
        for (int ball = 0; ball < 6 && current_over < target_overs && wickets < 10; ++ball) {
            simulateBall();
            current_ball++;
        }

        current_over++;
        current_ball = 0;
        
        if (wickets < 10) {
            std::cout << "End of Over " << current_over << " | Score: " << total_runs << "/" 
                      << wickets << " | Bowler: " << current_bowler->getName() << std::endl;
        }
    }

    // Print match summary
    std::cout << "\n=== Match Summary ===" << std::endl;
    std::cout << batting_team_name << " scored: " << total_runs << "/" << wickets 
              << " in " << current_over << " overs" << std::endl;
    std::cout << "Strike Rate: " << (total_runs * 100.0 / (current_over * 6)) << " runs per 100 balls" << std::endl;

    return true;
}

void Match::simulateBall() {
    // Placeholder for actual ball simulation logic
    // In a real implementation, this would:
    // 1. Calculate outcome based on batsman stats vs bowler stats
    // 2. Update runs, wickets
    // 3. Handle deliveries (dot, single, double, boundary, etc.)
    // 4. Check for wickets
    // 5. Rotate strike if needed

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> outcome_dis(0, 10);

    int outcome = outcome_dis(gen);
    std::string ball_result = "DOT";

    if (outcome == 0) {
        ball_result = "WICKET";
        wickets++;
        if (wickets < 10) {
            // Move to next batsman
            std::cout << "Over " << (current_over + 1) << ", Ball " << (current_ball + 1) 
                      << " | " << current_bowler->getName() << " -> " << striker->getName() 
                      << " | " << ball_result << " (Wicket!)" << std::endl;
        }
    } else if (outcome <= 3) {
        ball_result = "SINGLE";
        total_runs += 1;
        striker->addRuns(1);
        // Swap striker and non-striker
        std::swap(striker, non_striker);
    } else if (outcome <= 6) {
        ball_result = "DOUBLE";
        total_runs += 2;
        striker->addRuns(2);
    } else if (outcome == 7) {
        ball_result = "FOUR";
        total_runs += 4;
        striker->addRuns(4);
    } else if (outcome == 8) {
        ball_result = "SIX";
        total_runs += 6;
        striker->addRuns(6);
    } else {
        ball_result = "DOT";
    }

    if (outcome != 0) {
        std::cout << "Over " << (current_over + 1) << ", Ball " << (current_ball + 1) 
                  << " | " << current_bowler->getName() << " -> " << striker->getName() 
                  << " | " << ball_result << " | Score: " << total_runs << "/" << wickets << std::endl;
    }
}

const std::vector<Player*>& Match::getBattingTeam() const {
    return batting_team;
}

const std::vector<Player*>& Match::getBowlingTeam() const {
    return bowling_team;
}

std::string Match::getBattingTeamName() const {
    return batting_team_name;
}

std::string Match::getBowlingTeamName() const {
    return bowling_team_name;
}

int Match::getCurrentOver() const {
    return current_over;
}

int Match::getCurrentBall() const {
    return current_ball;
}

int Match::getTotalRuns() const {
    return total_runs;
}

int Match::getWickets() const {
    return wickets;
}

void Match::cleanup() {
    for (auto player : india_team) {
        delete player;
    }
    for (auto player : england_team) {
        delete player;
    }
    india_team.clear();
    england_team.clear();
    batting_team.clear();
    bowling_team.clear();
}
