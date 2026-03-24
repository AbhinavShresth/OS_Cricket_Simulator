#include "player.hpp"
#include "match_context.hpp"
#include <iostream>
#include <random>

Player::Player(const std::string& name, PlayerRole role, double strike_rate, double avg,
               int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats)
    : name(name), role(role), strike_rate(strike_rate), avg(avg),
      expected_balls(expected_balls), thread_priority(thread_priority),
      is_death_specialist(is_death_specialist), stats(stats) {}

std::string Player::getName() const { return name; }
PlayerRole Player::getRole() const { return role; }
double Player::getStrikeRate() const { return strike_rate; }
double Player::getAvg() const { return avg; }
int Player::getExpectedBalls() const { return expected_balls; }
int Player::getThreadPriority() const { return thread_priority; }
bool Player::isDeathSpecialist() const { return is_death_specialist; }
const PlayerStats& Player::getStats() const { return stats; }

void Player::setName(const std::string& name) { this->name = name; }
void Player::setPriority(int priority) { this->thread_priority = priority; }
void Player::setContext(MatchContext* ctx) { context = ctx; }
void Player::setFieldingQuarter(int quarter) { fielding_quarter = quarter; }
void Player::setCurrentlyBowling(bool status) { is_currently_bowling = status; }
void Player::setStriker(bool status) { is_striker = status; }
void Player::setIsFieldingTeam(bool status) {is_fielding_team = status;}


void Player::startThread() {
    pthread_create(&thread_id, nullptr, Player::threadEntry, this);
}

void Player::joinThread() {
    pthread_join(thread_id, nullptr);
}

void* Player::threadEntry(void* arg) {
    Player* player = static_cast<Player*>(arg);
    player->threadLoop();
    return nullptr;
}

void Player::fielderThreadLoop() {
 
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    while (context->match_active) {
        pthread_mutex_lock(&context->field_mutex);
        std::cout << name << " fielder acquired field_mutex" << std::endl;
        
        while (!context->ball_in_air && context->match_active) {
            std::cout << name << " fielder waiting for field_cv" << std::endl;
            pthread_cond_wait(&context->field_cv, &context->field_mutex);
        }

        if (!context->match_active) {
            pthread_mutex_unlock(&context->field_mutex);
            break;
        }

        if (!context->ball_dead && !context->is_wicket_this_ball) {
            double catch_chance = (context->hit_quarter == fielding_quarter) ? stats.catching_efficiency * 0.5 : 0.0;
            
            if (catch_chance > 0.0 && dis(gen) < catch_chance) {
                context->is_wicket_this_ball = true;
                context->ball_dead = true; 
                context->runs_scored_this_ball = 0;
            }
        }

        context->fielders_pending--;

        if (context->fielders_pending == 0) {
            std::cout << "I was the last fielder " << name << std::endl;
            
            if (!context->is_wicket_this_ball) {
                context->runs_scored_this_ball = (dis(gen) > 0.5) ? 1 : 2;
                context->ball_dead = true;
            }
            pthread_cond_signal(&context->umpire_cv);
        }

        while (context->ball_in_air && context->match_active) {
            pthread_cond_wait(&context->umpire_cv, &context->field_mutex);
        }

        pthread_mutex_unlock(&context->field_mutex);
    }
}
void Player::batsmanThreadLoop(){
    while (context && context->match_active) {
        pthread_mutex_lock(&context->roster_mutex);
        while (!is_striker && context->match_active){
            pthread_cond_wait(&context->roster_cv, &context->roster_mutex);
        }
        pthread_mutex_unlock(&context->roster_mutex);
        if (!context->match_active) break;
        pthread_mutex_lock(&context->pitch_mutex);
        std::cout << name <<  "I have the pitch_mutex and i am batsman" << std::endl;
        
        while ((!context->ball_delivered || !is_striker) && context->match_active) {
            pthread_cond_wait(&context->pitch_cv, &context->pitch_mutex);
        }
        
        if (!context->match_active) {
            pthread_mutex_unlock(&context->pitch_mutex);
            break;
        }
        std::cout << name << "I am the striker currently" << std::endl;
        
        context->ball_delivered = false;
        pthread_mutex_unlock(&context->pitch_mutex);

        pthread_mutex_lock(&context->field_mutex);
        context->hit_quarter = (rand() % 4);
        context->is_wicket_this_ball = false;
        context->runs_scored_this_ball = 0;
        context->fielders_pending = 10;
        context->ball_dead = false;
        context->ball_in_air = true;
        std::cout << name << "I have hit the ball into the field" << std::endl;
        pthread_cond_broadcast(&context->field_cv);
        std::cout << name << "I have removed the pitch_mutex" << std::endl;
        
        pthread_mutex_unlock(&context->field_mutex);
    }
}

Batsman::Batsman(const std::string& name, double strike_rate, double avg,
                 int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats)
    : Player(name, PlayerRole::BATSMAN, strike_rate, avg, expected_balls, thread_priority, is_death_specialist, stats) {}

int Batsman::getRunsScored() const { return runs_scored; }
int Batsman::getBallsFaced() const { return balls_faced; }
bool Batsman::isOut() const { return is_out; }
void Batsman::addRuns(int runs) { if (!is_out) runs_scored += runs; }
void Batsman::addBall() { if (!is_out) balls_faced++; }
void Batsman::setOut(bool out) { is_out = out; }
void Batsman::play() {}

// // [Mod Start] Batsman specific thread loop evaluating pitch buffer and signaling field
void Batsman::threadLoop() {
    if (is_fielding_team){
        fielderThreadLoop();
    } else {
        batsmanThreadLoop();
    }
}

Bowler::Bowler(const std::string& name, double strike_rate, double avg,
               int expected_balls, double economy, int max_overs,
               int thread_priority, bool is_death_specialist, const PlayerStats& stats)
    : Player(name, PlayerRole::BOWLER, strike_rate, avg, expected_balls, thread_priority, is_death_specialist, stats),
      economy(economy), max_overs(max_overs) {}

double Bowler::getEconomy() const { return economy; }
int Bowler::getMaxOvers() const { return max_overs; }
int Bowler::getOversBowled() const { return overs_bowled; }
int Bowler::getRunsConceded() const { return runs_conceded; }
int Bowler::getWicketsTaken() const { return wickets_taken; }

void Bowler::addOverBowled(int runs_in_over) {
    if (overs_bowled < max_overs) {
        overs_bowled++;
        runs_conceded += runs_in_over;
    }
}
void Bowler::addWicket() { wickets_taken++; }
void Bowler::play() {}

// // [Mod Start] Bowler specific thread loop integrating role evaluation
void Bowler::threadLoop() {
    if (!is_fielding_team){
        batsmanThreadLoop();
        return;
    }
    if (!is_currently_bowling){
        fielderThreadLoop();
        return;
    }

    while (context && context->match_active) {
        pthread_mutex_lock(&context->pitch_mutex);
        std::cout << name << "I have the pitch mutex and i am bowler" << std::endl;
        
        
        while (!context->bowler_turn && context->match_active) {
            std::cout << name <<  " I am waiting for bowling turn" << std::endl;
            
            pthread_cond_wait(&context->pitch_cv, &context->pitch_mutex);
        }
        
        if (!context->match_active) {
            pthread_mutex_unlock(&context->pitch_mutex);
            break;
        }
        std::cout << name << " I am the current bowler and i have thrown it" << std::endl;
        
        context->bowler_turn = false;
        context->current_ball.speed = stats.pace_skill; 
        context->ball_delivered = true;
        
        pthread_cond_broadcast(&context->pitch_cv);
        pthread_mutex_unlock(&context->pitch_mutex);
    }
}
// // [Mod End]