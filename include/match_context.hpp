#ifndef MATCH_CONTEXT_HPP
#define MATCH_CONTEXT_HPP

#include <pthread.h>
#include <string>

struct BallData {
    double speed = 0.0;
    double swing = 0.0;
    double spin = 0.0;
};

/// Snapshot of striker attributes for this delivery (set when the ball is hit).
struct StrikerBattingSnapshot {
    double batting = 0.5;
    double shot_selection = 0.5;
    double power_hitting = 0.5;
    double strike_rate = 125.0;
    int expected_balls = 18;
};

/// Snapshot of current bowler's control attributes (set when the bowler bowls).
struct BowlerBowlingSnapshot {
    double line_accuracy  = 0.5;
    double length_control = 0.5;
};

struct MatchContext {

    // Bowler to Umpire/Batsman synchronization
    pthread_mutex_t pitch_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t pitch_cv = PTHREAD_COND_INITIALIZER;
    
    BallData current_ball;
    bool bowler_turn = false;
    bool ball_delivered = false;

    // Fielder to Batsman/Umpire synchronization
    pthread_mutex_t field_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t field_cv = PTHREAD_COND_INITIALIZER;
    pthread_cond_t umpire_cv = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t roster_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t roster_cv = PTHREAD_COND_INITIALIZER;
    
    bool ball_in_air = false;
    bool ball_dead = false;
    int hit_quarter = -1; 
    int fielders_pending = 0; 
    int fielders_ready = 10;
    
    // Play outcome data
    int runs_scored_this_ball = 0;
    bool is_wicket_this_ball = false;
    bool is_wide_this_ball = false;
    /// True when the batter hit the ball in the air (catchable).
    bool is_aerial_this_ball = false;
    /// Name of the fielder who took the catch (empty if not a catch dismissal).
    std::string catcher_name;
    StrikerBattingSnapshot striker_this_ball;
    BowlerBowlingSnapshot  bowler_snap;

    // Global simulation state
    bool match_active = true;

    // Scheduler mode: when true wicket probability scales with 1/expected_balls (SJF)
    bool sjf_mode = false;
    int reference_expected_balls = 25; // baseline for SJF wicket-prob scaling
};

#endif // MATCH_CONTEXT_HPP