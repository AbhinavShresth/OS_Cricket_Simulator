#ifndef MATCH_HPP
#define MATCH_HPP

#include "player.hpp"
#include <vector>
#include <string>
#include <memory>

class Match {
private:
    std::vector<Player*> india_team;
    std::vector<Player*> england_team;
    std::vector<Player*> batting_team;
    std::vector<Player*> bowling_team;
    std::string batting_team_name;
    std::string bowling_team_name;

    // Match statistics
    int current_over = 0;
    int current_ball = 0;
    int total_runs = 0;
    int wickets = 0;
    int target_overs = 20;  // Default T20

    // Current players
    Batsman* current_batsman = nullptr;
    Batsman* striker = nullptr;
    Batsman* non_striker = nullptr;
    Bowler* current_bowler = nullptr;

public:
    Match();
    ~Match();

    /**
     * Start the match by loading teams from file paths
     * @param india_path Path to India team file (default: "stats/india.txt")
     * @param australia_path Path to Australia team file (default: "stats/australia.txt")
     * @return true if successfully loaded, false otherwise
     */
    bool start(const std::string& india_path = "stats/india.txt",
               const std::string& australia_path = "stats/australia.txt");

    /**
     * Conduct the toss and decide which team bats and which bowls
     * @return true if one team wins toss, false on error
     */
    bool toss();

    /**
     * Run the match simulation
     * @param num_overs Number of overs to simulate (default: 20 for T20)
     * @return true if match completed successfully, false otherwise
     */
    bool run(int num_overs = 20);

    // Getters
    const std::vector<Player*>& getBattingTeam() const;
    const std::vector<Player*>& getBowlingTeam() const;
    std::string getBattingTeamName() const;
    std::string getBowlingTeamName() const;
    int getCurrentOver() const;
    int getCurrentBall() const;
    int getTotalRuns() const;
    int getWickets() const;

private:
    /**
     * Simulate a single ball
     */
    void simulateBall();

    /**
     * Cleanup dynamically allocated players
     */
    void cleanup();
};

#endif // MATCH_HPP
