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

// ---------------------------------------------------------------------------
// Data structures for logging and visualisation
// ---------------------------------------------------------------------------

/// One delivery event recorded during an innings.
struct BallEvent {
    int over;           ///< 0-indexed over number
    int ball_in_over;   ///< 1-indexed ball within the over (legal balls only)
    int abs_ball;       ///< 1-indexed absolute legal-ball count in innings
    std::string striker;
    std::string non_striker;
    std::string bowler;
    int runs;           ///< runs off this ball (0 for wicket / wide)
    bool is_wicket;
    bool is_wide;
    std::string dismissed_name;  ///< striker dismissed (if is_wicket)
    std::string catcher;         ///< fielder who caught (if caught dismissal)
    int score_runs;     ///< cumulative runs after this ball
    int score_wkts;     ///< cumulative wickets after this ball
};

/// Per-batsman record for the innings scorecard.
struct BatsmanRecord {
    std::string name;
    int runs   = 0;
    int balls  = 0;
    int fours  = 0;
    int sixes  = 0;
    bool is_out = false;
    std::string dismissal_info;   ///< e.g. "caught by Jos Buttler" or "bowled"
    std::string bowler_name;      ///< bowler who got the wicket
};

/// Per-bowler record for the innings scorecard.
struct BowlerRecord {
    std::string name;
    int balls_bowled = 0;
    int runs_conceded = 0;
    int wickets = 0;
    int wides   = 0;
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
    std::vector<int>  crease_entry_ball;
    std::vector<int>  first_faced_ball;
    std::vector<bool> has_faced_ball;

    // Logging / visualisation
    std::vector<BallEvent>     ball_log;
    std::vector<BatsmanRecord> bat_records;
    std::vector<BowlerRecord>  bowl_records;

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

    /// Print a formatted ball-by-ball commentary log.
    void printBallLog(const std::string& innings_label) const;

    /// Print the innings scorecard (batting + bowling tables).
    void printInningsScorecard(const std::string& innings_label,
                                int total_runs, int wickets,
                                int extras_wides) const;

    /// Print a text Gantt chart showing each batsman's activity across all balls.
    void printGanttChart(const std::string& innings_label) const;
};

#endif // MATCH_HPP
