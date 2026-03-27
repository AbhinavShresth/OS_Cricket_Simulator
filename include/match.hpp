#ifndef MATCH_HPP
#define MATCH_HPP

#include "player.hpp"
#include "match_context.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>

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
    int target_overs = 20;  

    Player* striker = nullptr;
    Player* non_striker = nullptr;
    Bowler* current_bowler = nullptr;

    MatchContext context;

public:
    Match();
    ~Match();

    bool start(const std::string& india_path = "stats/india.txt",
               const std::string& england_path = "stats/england.txt");

    bool toss();


    bool run(int num_overs = 20);
    void setBattingOrder(const std::vector<int>& new_indices);
    void setBowlingOrder(const std::vector<int>& new_indices);

    const std::vector<Player*>& getBattingTeam() const;
    const std::vector<Player*>& getBowlingTeam() const;
    std::string getBattingTeamName() const;
    std::string getBowlingTeamName() const;
    int getCurrentOver() const;
    int getCurrentBall() const;
    int getTotalRuns() const;
    int getWickets() const;

private:
    void simulateBall();
    std::pair<int, int> runInnings(int num_overs, int chase_target = -1);
    void cleanup();
};

#endif // MATCH_HPP
