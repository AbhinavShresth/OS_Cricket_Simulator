#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include <pthread.h>
#include "match_context.hpp"

struct MatchContext;

enum class PlayerRole {
    BATSMAN,
    BOWLER,
    ALL_ROUNDER,
    WICKETKEEPER
};

struct BattingResult {
    BallData hit_ball;
    bool is_wicket;
    int hit_quarter;
};
struct PlayerStats {
    double fitness = 0.0;
    double catching_efficiency = 0.0;
    double clutch_factor = 0.0;
    double pressure_performance = 0.0;
    double batting = 0.0;
    double shot_selection = 0.0;
    double power_hitting = 0.0;
    double pace_skill = 0.0;
    double swing_skill = 0.0;
    double spin_skill = 0.0;
    double spin_quantity = 0.0;
    double line_accuracy = 0.0;
    double length_control = 0.0;
};

class Player {
protected:
    std::string name;
    PlayerRole role;
    double strike_rate;
    double avg;
    int expected_balls;
    int thread_priority;
    bool is_death_specialist;
    PlayerStats stats;

    // // [Mod Start] Concurrency and state tracking variables
    pthread_t thread_id;
    MatchContext* context = nullptr;
    int fielding_quarter = -1;
    bool is_currently_bowling = false;
    bool is_striker = false;
    bool is_fielding_team = false;
    bool is_out =false;

    static void* threadEntry(void* arg);
    virtual void threadLoop() = 0;
    void fielderThreadLoop();
    void batsmanThreadLoop();

    BallData calculateBowling(const PlayerStats& bowler_stats);
    BattingResult calculateBatting(const BallData& incoming_ball, const PlayerStats& batter_stats);
    bool calculateFielding(const BallData& hit_ball, const PlayerStats& fielder_stats, int hit_quarter, int fielding_quarter);

public:
    Player(const std::string& name, PlayerRole role, double strike_rate, double avg,
           int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats);
    virtual ~Player() = default;

    std::string getName() const;
    PlayerRole getRole() const;
    double getStrikeRate() const;
    double getAvg() const;
    int getExpectedBalls() const;
    int getThreadPriority() const;
    bool isDeathSpecialist() const;
    bool isOut() const;
    const PlayerStats& getStats() const;

    void setName(const std::string& name);
    void setPriority(int priority);

    // // [Mod Start] Thread management and state setters
    void setContext(MatchContext* ctx);
    void setFieldingQuarter(int quarter);
    void setCurrentlyBowling(bool status);
    void setStriker(bool status);
    void setIsFieldingTeam(bool status);
    void setOut(bool out);
    void startThread();
    void joinThread();
    // // [Mod End]

    virtual void play() = 0;
};

class Batsman : public Player {
private:
    int runs_scored = 0;
    int balls_faced = 0;
    

protected:
    void threadLoop() override;

public:
    Batsman(const std::string& name, double strike_rate, double avg,
            int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats);

    int getRunsScored() const;
    int getBallsFaced() const;
    

    void addRuns(int runs);
    void addBall();
    

    void play() override;
};

class Bowler : public Player {
private:
    double economy;
    int max_overs;
    int overs_bowled = 0;
    int runs_conceded = 0;
    int wickets_taken = 0;

protected:
    void threadLoop() override;

public:
    Bowler(const std::string& name, double strike_rate, double avg,
           int expected_balls, double economy, int max_overs,
           int thread_priority, bool is_death_specialist, const PlayerStats& stats);

    double getEconomy() const;
    int getMaxOvers() const;
    int getOversBowled() const;
    int getRunsConceded() const;
    int getWicketsTaken() const;

    void addOverBowled(int runs_in_over);
    void addWicket();

    void play() override;
};

#endif // PLAYER_HPP