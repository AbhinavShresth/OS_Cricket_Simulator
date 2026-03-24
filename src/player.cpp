#include "player.hpp"
#include <iostream>


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

Batsman::Batsman(const std::string& name, double strike_rate, double avg,
                 int expected_balls, int thread_priority, bool is_death_specialist, const PlayerStats& stats)
    : Player(name, PlayerRole::BATSMAN, strike_rate, avg, expected_balls, thread_priority, is_death_specialist, stats) {}

int Batsman::getRunsScored() const { return runs_scored; }
int Batsman::getBallsFaced() const { return balls_faced; }
bool Batsman::isOut() const { return is_out; }
void Batsman::addRuns(int runs) { if (!is_out) runs_scored += runs; }
void Batsman::addBall() { if (!is_out) balls_faced++; }
void Batsman::setOut(bool out) { is_out = out; }
void Batsman::play() { std::cout << name << " (Batsman) is batting...\n"; }

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
void Bowler::play() { std::cout << name << " (Bowler) is bowling...\n"; }