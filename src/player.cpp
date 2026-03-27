#include "player.hpp"
#include "match_context.hpp"
#include <cmath>
#include <iostream>
#include <random>

namespace {

double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

/// Maps hit-ball speed (from bowler pace × power_hitting) and striker skills to 1/2/4/6.
int sampleRunsFromHit(const BallData& hit_ball, const StrikerBattingSnapshot& s, std::mt19937& gen) {
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    const double hit_strength = clamp01((hit_ball.speed - 35.0) / 170.0);
    const double movement = clamp01((std::abs(hit_ball.swing) + std::abs(hit_ball.spin)) / 10.0);
    const double bat = clamp01(s.batting);
    const double powh = clamp01(s.power_hitting);
    const double sel = clamp01(s.shot_selection);
    const double sr_norm = clamp01((s.strike_rate - 110.0) / 75.0);
    const double expected_norm = clamp01((static_cast<double>(s.expected_balls) - 8.0) / 30.0);

    // Realistic cricket modifiers
    const double clean_contact = 1.0 - 0.55 * movement;
    const double scramble_factor = 1.0 + 0.45 * movement;
    const double anchor_factor = 1.0 + 0.35 * expected_norm;
    const double attack_factor = 1.0 - 0.28 * expected_norm;

    // Boundary probabilities
    double w6 = (0.015 + 0.50 * hit_strength * hit_strength * powh *
                (0.4 + 0.6 * sr_norm) * clean_contact) * attack_factor;

    double w4 = (0.045 + 0.55 * hit_strength *
                (0.45 * powh + 0.55 * bat) *
                (0.65 + 0.35 * sr_norm) * clean_contact) * attack_factor;

    // 🔥 NEW: 3 runs (gap + running)
    double w3 = (0.05
                + 0.25 * sel * bat * (1.0 - hit_strength)
                + 0.15 * movement) * scramble_factor * anchor_factor;

    // Running-based outcomes
    double w2 = (0.10
                + 0.35 * sel * std::sqrt(std::max(0.0, 1.0 - 0.6 * hit_strength))
                + 0.10 * bat) * scramble_factor * anchor_factor;

    double w1 = (0.25
                + 0.20 * bat * sel
                + 0.15 * (1.0 - hit_strength)) * scramble_factor * anchor_factor;

    // Optional realism: strong hits rarely give 3
    if (hit_strength > 0.85) {
        w3 *= 0.2;
    }

    // Normalize sampling
    const double sum = w1 + w2 + w3 + w4 + w6;
    double r = uni(gen) * sum;
    if (r < w6) return 6;
    r -= w6;

    if (r < w4) return 4;
    r -= w4;

    if (r < w3) return 3;
    r -= w3;

    if (r < w2) return 2;

    return 1;
}

}  // namespace

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
BallData Player::calculateBowling(const PlayerStats& bowler_stats) {
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(-5.0, 5.0);
    
    BallData ball;
    ball.speed = bowler_stats.pace_skill + dis(gen);
    ball.swing = bowler_stats.swing_skill + dis(gen);
    ball.spin  = bowler_stats.spin_skill + dis(gen);
    
    return ball;
}

BattingResult Player::calculateBatting(const BallData& incoming_ball, const PlayerStats& batter_stats) {
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis_quarter(0, 3);
    std::uniform_real_distribution<> dis_prob(0.0, 1.0);

    BattingResult res;
    res.hit_quarter = dis_quarter(gen);
    
    // Base wicket probability (e.g., Bowled / LBW bypassing fielders)
    double miss_prob = 0.0005 / (batter_stats.batting > 0 ? batter_stats.batting : 1.0);

    // SJF scaling: wicket probability is proportional to 1/expected_balls.
    // A batsman with fewer expected balls is proportionally more likely to be out.
    // E.g. with reference_expected_balls=25: a batsman with expected_balls=5 gets
    // factor=5, so they are 5x as likely to be dismissed as one with expected_balls=25.
    if (context && context->sjf_mode && expected_balls > 0) {
        miss_prob *= static_cast<double>(context->reference_expected_balls) /
                     static_cast<double>(expected_balls);
    }

    res.is_wicket = (dis_prob(gen) < miss_prob); 

    // Calculate post-contact ball profile for the fielding phase.
    res.hit_ball.speed = incoming_ball.speed * (batter_stats.power_hitting > 0 ? batter_stats.power_hitting : 1.0);
    // Movement reduces after contact, but does not vanish.
    res.hit_ball.swing = incoming_ball.swing * 0.35;
    res.hit_ball.spin = incoming_ball.spin * 0.35;
    std::cout << "Is Wicket? " << res.is_wicket << "Quarter? " << res.hit_quarter << std::endl;
        
    return res;
}
bool Player::calculateFielding(const BallData& hit_ball, const PlayerStats& fielder_stats, int hit_quarter, int fielding_quarter) {
    if (hit_quarter != fielding_quarter) return false;
    
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Dynamic difficulty: faster balls are harder to catch
    double speed_modifier = (hit_ball.speed > 130.0) ? 0.7 : 1.0;
    double catch_chance = fielder_stats.catching_efficiency * 0.001 * speed_modifier;
    
    return (catch_chance > 0.0 && dis(gen) < catch_chance);
}

void Player::fielderThreadLoop() {
    std::random_device rd;
    std::mt19937 gen(rd());

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
            bool caught = calculateFielding(context->current_ball, stats, context->hit_quarter, fielding_quarter);

            if (caught) {
                context->is_wicket_this_ball = true;
                context->ball_dead = true; 
                context->runs_scored_this_ball = 0;
            }
        }

        context->fielders_pending--;
        std::cout << "[FIELD - " << name << "] Processed ball. fielders_pending = " << context->fielders_pending << std::endl;

        if (context->fielders_pending == 0) {
            if (!context->is_wicket_this_ball) {
                context->runs_scored_this_ball =
                    sampleRunsFromHit(context->current_ball, context->striker_this_ball, gen);
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
        BattingResult b_result = calculateBatting(context->current_ball, stats);
        context->current_ball = b_result.hit_ball;
        context->hit_quarter = b_result.hit_quarter;
        context->is_wicket_this_ball = b_result.is_wicket;
        context->runs_scored_this_ball = 0;
        context->striker_this_ball.batting = stats.batting;
        context->striker_this_ball.shot_selection = stats.shot_selection;
        context->striker_this_ball.power_hitting = stats.power_hitting;
        context->striker_this_ball.strike_rate = getStrikeRate();
        context->striker_this_ball.expected_balls = getExpectedBalls();

        context->fielders_pending = 10;
        context->fielders_ready = 0;
        context->ball_dead = b_result.is_wicket;
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

    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

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
        // Decide if this delivery is a WIDE. If it is, batsman/fielders should not act
        // and the umpire should add +1 run while not counting this delivery.
        context->bowler_turn = false;
        context->current_ball = calculateBowling(stats);

        const BallData delivery = context->current_ball;
        const double line_term = clamp01(1.0 - stats.line_accuracy);
        const double length_term = clamp01(1.0 - stats.length_control);
        const double movement_term =
            clamp01((std::abs(delivery.swing) + std::abs(delivery.spin)) / 10.0);
        const double speed_term = clamp01(delivery.speed / 200.0);

        double wide_prob =
            0.01 + 0.18 * line_term + 0.14 * length_term + 0.06 * movement_term + 0.03 * speed_term;
        wide_prob = clamp01(wide_prob);

        bool is_wide = (dis(gen) < wide_prob);
        if (is_wide) {
            // Complete umpire resolution immediately.
            context->ball_delivered = false;
            pthread_mutex_unlock(&context->pitch_mutex);

            pthread_mutex_lock(&context->field_mutex);
            context->is_wide_this_ball = true;
            context->is_wicket_this_ball = false;
            context->runs_scored_this_ball = 1; // WIDE adds 1 run
            context->ball_dead = true;
            context->ball_in_air = false;

            std::cout << "[BOWL - " << name << "] WIDE! Signaling umpire." << std::endl;
            pthread_cond_broadcast(&context->umpire_cv);
            pthread_mutex_unlock(&context->field_mutex);
            continue;
        }

        std::cout << "[BOWL - " << name << "] I am the current bowler and I have thrown it. Broadcasting pitch_cv." << std::endl;
        context->ball_delivered = true;
        pthread_cond_broadcast(&context->pitch_cv);
        pthread_mutex_unlock(&context->pitch_mutex);
    }
}