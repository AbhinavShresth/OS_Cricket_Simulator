#ifndef MATCH_CONTEXT_HPP
#define MATCH_CONTEXT_HPP

#include <pthread.h>

struct BallData {
    double speed = 0.0;
    double swing = 0.0;
    double spin = 0.0;
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
    
    // Global simulation state
    bool match_active = true;
};

#endif // MATCH_CONTEXT_HPP