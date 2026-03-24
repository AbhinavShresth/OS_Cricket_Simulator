#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>

enum class PlayerRole {
    BATSMAN,
    BOWLER,
    ALL_ROUNDER,
    WICKETKEEPER
};

class Player {
protected:
    std::string name;
    PlayerRole role;
    double strike_rate;  // For batsmen: runs per 100 balls; For bowlers: can be inversely related
    double avg;  // Average runs or average economy
    int expected_balls;
    int thread_priority;
    bool is_death_specialist;

public:
    Player(const std::string& name, PlayerRole role, double strike_rate, double avg,
           int expected_balls, int thread_priority, bool is_death_specialist);
    virtual ~Player() = default;

    // Getters
    std::string getName() const;
    PlayerRole getRole() const;
    double getStrikeRate() const;
    double getAvg() const;
    int getExpectedBalls() const;
    int getThreadPriority() const;
    bool isDeathSpecialist() const;

    // Setters
    void setName(const std::string& name);
    void setPriority(int priority);

    virtual void play() = 0;
};

class Batsman : public Player {
private:
    int runs_scored = 0;
    int balls_faced = 0;
    bool is_out = false;

public:
    Batsman(const std::string& name, double strike_rate, double avg,
            int expected_balls, int thread_priority, bool is_death_specialist);

    int getRunsScored() const;
    int getBallsFaced() const;
    bool isOut() const;

    void addRuns(int runs);
    void addBall();
    void setOut(bool out);

    void play() override;
};

class Bowler : public Player {
private:
    double economy;
    int max_overs;
    int overs_bowled = 0;
    int runs_conceded = 0;
    int wickets_taken = 0;

public:
    Bowler(const std::string& name, double strike_rate, double avg,
           int expected_balls, double economy, int max_overs,
           int thread_priority, bool is_death_specialist);

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
