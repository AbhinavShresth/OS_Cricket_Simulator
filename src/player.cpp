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
bool Player::isOut() const { return is_out; }
const PlayerStats& Player::getStats() const { return stats; }

void Player::setName(const std::string& name) { this->name = name; }
void Player::setPriority(int priority) { this->thread_priority = priority; }
void Player::setContext(MatchContext* ctx) { context = ctx; }
void Player::setFieldingQuarter(int quarter) { fielding_quarter = quarter; }
void Player::setCurrentlyBowling(bool status) { is_currently_bowling = status; }
void Player::setStriker(bool status) { is_striker = status; }
void Player::setOut(bool out) { is_out = out; }
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
        std::cout << "[FIELD - " << name << "] Acquired field_mutex (Phase 1)." << std::endl;

        while (!context->ball_in_air && context->match_active) {
            std::cout << "[FIELD - " << name << "] Conditions not met (ball_in_air: " << context->ball_in_air << "). Waiting on field_cv." << std::endl;
            pthread_cond_wait(&context->field_cv, &context->field_mutex);
            std::cout << "[FIELD - " << name << "] Woke up from field_cv." << std::endl;
        }

        if (!context->match_active) {
            pthread_mutex_unlock(&context->field_mutex);
            break;
        }

        if (!context->ball_dead && !context->is_wicket_this_ball) {
            double catch_chance = (context->hit_quarter == fielding_quarter) ? stats.catching_efficiency * 0.0001 : 0.0;
            if (catch_chance > 0.0 && dis(gen) < catch_chance) {
                context->is_wicket_this_ball = true;
                context->ball_dead = true; 
                context->runs_scored_this_ball = 0;
            }
        }

        context->fielders_pending--;
        std::cout << "[FIELD - " << name << "] Processed ball. fielders_pending = " << context->fielders_pending << std::endl;

        if (context->fielders_pending == 0) {
            if (!context->is_wicket_this_ball) {
                context->runs_scored_this_ball = (dis(gen) > 0.5) ? 1 : 2;
                context->ball_dead = true;
            }
            std::cout << "[FIELD - " << name << "] I am the LAST fielder. Signaling umpire_cv." << std::endl;
            pthread_cond_broadcast(&context->umpire_cv);
        }

        while (context->ball_in_air && context->match_active) {
            std::cout << "[FIELD - " << name << "] Conditions not met (ball_in_air: " << context->ball_in_air << "). Waiting on umpire_cv (Phase 2)." << std::endl;
            pthread_cond_wait(&context->umpire_cv, &context->field_mutex);
            std::cout << "[FIELD - " << name << "] Woke up from umpire_cv." << std::endl;
        }
        
        if(!context->match_active){
            pthread_mutex_unlock(&context->field_mutex);
            break;
        }
        
        context->fielders_ready++;
        std::cout << "[FIELD - " << name << "] Escaped trapdoor. fielders_ready = " << context->fielders_ready << std::endl;
        
        if (context->fielders_ready == 10){
            std::cout << "[FIELD - " << name << "] Trapdoor clear. Signaling umpire_cv." << std::endl;
            pthread_cond_broadcast(&context->umpire_cv);
        }
        
        std::cout << "[FIELD - " << name << "] Releasing field_mutex." << std::endl;
        pthread_mutex_unlock(&context->field_mutex);
    }
}
void Player::batsmanThreadLoop(){
    while (context && context->match_active) {
        pthread_mutex_lock(&context->roster_mutex);
        std::cout << "[BAT - " << name << "] Acquired roster_mutex." << std::endl;
        
        while (!is_striker && context->match_active){
            std::cout << "[BAT - " << name << "] Not striker. Waiting on roster_cv." << std::endl;
            pthread_cond_wait(&context->roster_cv, &context->roster_mutex);
            std::cout << "[BAT - " << name << "] Woke up from roster_cv. is_striker = " << is_striker << std::endl;
        }
        pthread_mutex_unlock(&context->roster_mutex);
        
        if (!context->match_active) break;
        
        pthread_mutex_lock(&context->pitch_mutex);
        std::cout << "[BAT - " << name << "] Acquired pitch_mutex." << std::endl;
        
        while ((!context->ball_delivered || !is_striker) && context->match_active) {
            std::cout << "[BAT - " << name << "] Conditions not met (ball_delivered: " << context->ball_delivered << ", is_striker: " << is_striker << "). Waiting on pitch_cv." << std::endl;
            pthread_cond_wait(&context->pitch_cv, &context->pitch_mutex);
            std::cout << "[BAT - " << name << "] Woke up from pitch_cv." << std::endl;
        }
        
        if (!context->match_active) {
            pthread_mutex_unlock(&context->pitch_mutex);
            break;
        }
        std::cout << "[BAT - " << name << "] I am the striker currently and consuming ball_delivered." << std::endl;
        
        context->ball_delivered = false;
        pthread_mutex_unlock(&context->pitch_mutex);

        pthread_mutex_lock(&context->field_mutex);
        std::cout << "[BAT - " << name << "] Acquired field_mutex." << std::endl;
        
        context->hit_quarter = (rand() % 4);
        context->is_wicket_this_ball = false;
        context->runs_scored_this_ball = 0;
        context->fielders_pending = 10;
        context->fielders_ready = 0;
        context->ball_dead = false;
        context->ball_in_air = true;
        
        std::cout << "[BAT - " << name << "] Hit the ball into the field. Broadcasting field_cv." << std::endl;
        pthread_cond_broadcast(&context->field_cv);
        
        std::cout << "[BAT - " << name << "] Releasing field_mutex." << std::endl;
        pthread_mutex_unlock(&context->field_mutex);
    }
}
Batsman::Batsman(const std::string& name, double strike_rate, double avg,
                 int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats)
    : Player(name, PlayerRole::BATSMAN, strike_rate, avg, expected_balls, thread_priority, is_death_specialist, stats) {}

int Batsman::getRunsScored() const { return runs_scored; }
int Batsman::getBallsFaced() const { return balls_faced; }

void Batsman::addRuns(int runs) { if (!is_out) runs_scored += runs; }
void Batsman::addBall() { if (!is_out) balls_faced++; }

void Batsman::play() {}
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
        std::cout << "[BOWL - " << name << "] Acquired pitch_mutex." << std::endl;
        
        while (!context->bowler_turn && context->match_active) {
            std::cout << "[BOWL - " << name << "] Conditions not met (bowler_turn: " << context->bowler_turn << "). Waiting on pitch_cv." << std::endl;
            pthread_cond_wait(&context->pitch_cv, &context->pitch_mutex);
            std::cout << "[BOWL - " << name << "] Woke up from pitch_cv." << std::endl;
        }
        
        if (!context->match_active) {
            pthread_mutex_unlock(&context->pitch_mutex);
            break;
        }
        std::cout << "[BOWL - " << name << "] I am the current bowler and I have thrown it. Broadcasting pitch_cv." << std::endl;
        
        context->bowler_turn = false;
        context->current_ball.speed = stats.pace_skill; 
        context->ball_delivered = true;
        
        pthread_cond_broadcast(&context->pitch_cv);
        pthread_mutex_unlock(&context->pitch_mutex);
    }
}