#ifndef MATCH_HPP
#define MATCH_HPP

#include "player.hpp"
#include "match_context.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>

/// Scheduling policy applied to the batting order within a match.
enum class SchedulerType {
    FCFS, ///< First-Come First-Served: bats in descending thread_priority order (as per stats file)
    SJF   ///< Shortest-Job-First: bats in ascending expected_balls order
};

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

    // Scheduling
    SchedulerType scheduler_type = SchedulerType::FCFS;

    // Waiting-time tracking (indexed by batting position after scheduling sort)
    // crease_entry_ball[k] = legal balls bowled when batting_team[k] entered the crease
    std::vector<int> crease_entry_ball;
    // first_faced_ball[k] = legal balls bowled when batting_team[k] first faced a delivery as striker
    std::vector<int> first_faced_ball;
    // flag: has batting_team[k] faced at least one delivery?
    std::vector<bool> has_faced_ball;

public:
    Match();
    ~Match();

    bool start(const std::string& india_path = "stats/india.txt",
               const std::string& england_path = "stats/england.txt");

    bool toss();

    /// Run a complete match (2 innings) with the given scheduler policy.
    bool run(int num_overs = 20, SchedulerType sched = SchedulerType::FCFS);
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

    /// Sort batting_team according to the active scheduler_type and log the order.
    void applyBattingScheduler(const std::string& innings_label);

    /// Print waiting-time analysis for the middle order (positions 4-7, 1-indexed).
    void printWaitingTimeAnalysis(const std::string& innings_label) const;
};

#endif // MATCH_HPP
