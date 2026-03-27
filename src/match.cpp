#include "match.hpp"
#include "loader.hpp"
#include <iostream>
#include <cstdlib>
#include <random>
#include <utility>

Match::Match() {}

Match::~Match() {
    cleanup();
}

// --- Modification start: Resource cleanup logic ---
void Match::cleanup() {
    pthread_mutex_destroy(&context.pitch_mutex);
    pthread_cond_destroy(&context.pitch_cv);
    pthread_mutex_destroy(&context.field_mutex);
    pthread_cond_destroy(&context.field_cv);
    pthread_cond_destroy(&context.umpire_cv);
    pthread_mutex_destroy(&context.roster_mutex);
    pthread_cond_destroy(&context.roster_cv);

    for (auto player : india_team) delete player;
    for (auto player : england_team) delete player;
    
    india_team.clear();
    england_team.clear();
    batting_team.clear();
    bowling_team.clear();
}

bool Match::start(const std::string& india_path, const std::string& england_path) {
    if (!TeamLoader::load(india_path, india_team) || !TeamLoader::load(england_path, england_team)) {
        return false;
    }
    return true;
}

bool Match::toss() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    if (dis(gen) == 0) {
        batting_team = india_team;
        bowling_team = england_team;
        batting_team_name = "India";
        bowling_team_name = "England";
    } else {
        batting_team = england_team;
        bowling_team = india_team;
        batting_team_name = "England";
        bowling_team_name = "India";
    }
    std::cout << "Bowling team: " << bowling_team_name << ", Batting Team: " << batting_team_name << std::endl;
    return true;
}

void Match::setBattingOrder(const std::vector<int>& new_indices) {
    if (new_indices.size() != batting_team.size()) return;
    std::vector<Player*> temp_team;
    temp_team.reserve(batting_team.size());
    for (int index : new_indices) temp_team.push_back(batting_team[index]);
    batting_team = temp_team;
}
void Match::setBowlingOrder(const std::vector<int>& new_indices) {
    if (new_indices.size() != bowling_team.size()) return;
    std::vector<Player*> temp_team;
    temp_team.reserve(bowling_team.size());
    for (int index : new_indices) temp_team.push_back(bowling_team[index]);
    bowling_team = temp_team;
}

std::pair<int, int> Match::runInnings(int num_overs, int chase_target) {
    target_overs = num_overs;
    total_runs = 0;
    wickets = 0;
    current_over = 0;
    current_ball = 0;

    context.match_active = true;
    context.ball_delivered = false;
    context.ball_in_air = false;
    // call bowling and batting scheduler
    // bowler is the bowling_team[0]
    // Setup bowler and assign fielding quarters
    striker = batting_team[0];
    non_striker = batting_team[1];
    pthread_mutex_lock(&context.roster_mutex);
    striker->setStriker(true);
    non_striker->setStriker(false);
    std::cout << "The current striker is " << striker->getName() << std::endl;
    std::cout << "The current non striker is " << non_striker->getName() << std::endl;
    
    pthread_cond_broadcast(&context.roster_cv);
    pthread_mutex_unlock(&context.roster_mutex);
    current_bowler = nullptr;
    
    int quarter_assign = 0;
    for (size_t i = 0; i < bowling_team.size(); ++i) {
        if (bowling_team[i]->getRole() == PlayerRole::BOWLER && current_bowler == nullptr) {
            current_bowler = dynamic_cast<Bowler*>(bowling_team[i]);
            bowling_team[i]->setCurrentlyBowling(true);
        } else {
            bowling_team[i]->setCurrentlyBowling(false);
            bowling_team[i]->setFieldingQuarter(quarter_assign % 4);
            quarter_assign++;
        }
    }
    std::cout << "The current bowler is "<< current_bowler->getName() << std::endl;
    
    std::cout << "quarters assigned" << std::endl;
    for (auto player : batting_team) {
        player->setContext(&context);
        player->setIsFieldingTeam(false);
        player->startThread();
    }
    for (auto player : bowling_team) {
        player->setContext(&context);
        player->setIsFieldingTeam(true);
        player->startThread();
    }
    std::cout << "threads assigned" << std::endl;
    while (current_over < target_overs && wickets < 10) {
        if (chase_target > 0 && total_runs >= chase_target) break;
        while (current_ball < 6 && wickets < 10) {
            std::cout << "\n[UMPIRE] --- START OF OVER " << current_over << " BALL " << current_ball + 1 << " ---" << std::endl;
            
            pthread_mutex_lock(&context.field_mutex);
            std::cout << "[UMPIRE] Acquired field_mutex. Checking trapdoor (fielders_ready: " << context.fielders_ready << ")." << std::endl;
            
            while (context.fielders_ready < 10 && context.match_active){
                std::cout << "[UMPIRE] Waiting for fielders to reset. Waiting on umpire_cv." << std::endl;
                pthread_cond_wait(&context.umpire_cv, &context.field_mutex);
            }
            
            std::cout << "[UMPIRE] Trapdoor clear. Resetting field state." << std::endl;
            context.ball_dead = false;
            context.runs_scored_this_ball = 0;
            context.is_wicket_this_ball = false;
            context.is_wide_this_ball = false;
            context.ball_in_air = false;
            pthread_mutex_unlock(&context.field_mutex);
            
            pthread_mutex_lock(&context.pitch_mutex);
            std::cout << "[UMPIRE] Acquired pitch_mutex. Signaling bowler." << std::endl;
            context.bowler_turn = true;
            pthread_cond_broadcast(&context.pitch_cv);
            pthread_mutex_unlock(&context.pitch_mutex);
            
            pthread_mutex_lock(&context.field_mutex);
            std::cout << "[UMPIRE] Acquired field_mutex. Waiting for play resolution." << std::endl;
            while (!context.ball_dead && context.match_active) {
                std::cout << "[UMPIRE] Ball still alive. Waiting on umpire_cv." << std::endl;
                pthread_cond_wait(&context.umpire_cv, &context.field_mutex);
            }
            std::cout << "[UMPIRE] Play resolved. Calculating scores." << std::endl;
            
            total_runs += context.runs_scored_this_ball;
            if (!context.is_wide_this_ball) {
                pthread_mutex_lock(&context.roster_mutex);
                if (context.is_wicket_this_ball) {
                    wickets++;
                    if (striker) {
                        striker->setOut(true);
                        striker->setStriker(false);
                    }
                    if (static_cast<size_t>(wickets + 1) < batting_team.size()) {
                        Player* next_p = batting_team[wickets + 1];
                        striker = next_p;
                        if (striker) striker->setStriker(true);
                    } else {
                        striker = nullptr;
                    }
                } else if (context.runs_scored_this_ball % 2 != 0) {
                    if (striker && non_striker){
                        striker->setStriker(false);
                        non_striker->setStriker(true);
                        std::swap(striker, non_striker);
                    }
                }
                pthread_cond_broadcast(&context.roster_cv);
                pthread_mutex_unlock(&context.roster_mutex);
            }

            std::cout << "[UMPIRE] Releasing fielders to reset." << std::endl;
            context.ball_in_air = false;
            pthread_cond_broadcast(&context.umpire_cv);
            pthread_mutex_unlock(&context.field_mutex);
            
            if (!context.is_wide_this_ball) current_ball++;
            std::cout << "[SCORE] Over " << current_over << "." << current_ball << " | Score: " << total_runs << "/" << wickets << std::endl;

            if (chase_target > 0 && total_runs >= chase_target) break;
        }
        
        current_over++;
        current_ball = 0;
        
        // --- Modification start: End of Over Bowler Rotation ---
        std::cout << "[UMPIRE] Over complete. Rotating strike and bowler." << std::endl;
        
        // 1. Strike rotation
        if (striker && non_striker){
            pthread_mutex_lock(&context.roster_mutex);
            striker->setStriker(false);
            non_striker->setStriker(true);
            std::swap(striker, non_striker); 
            pthread_cond_broadcast(&context.roster_cv);
            pthread_mutex_unlock(&context.roster_mutex);
        }

        // 2. Bowler rotation
        // To prevent threads from deadlocking in the rotation, the actual logic for 
        // a thread switching from fielderThreadLoop to Bowler::threadLoop requires 
        // the while-loops to be merged. For now, simply finding the next bowler avoids 
        // the same bowler throwing 120 balls.
        int next_bowler_idx = -1;
        for (size_t i = 0; i < bowling_team.size(); ++i) {
            if (bowling_team[i] == current_bowler) {
                next_bowler_idx = (i + 1) % bowling_team.size();
                break;
            }
        }
        
        if (next_bowler_idx != -1) {
            // Find next valid bowler
            for (size_t i = 0; i < bowling_team.size(); ++i) {
                int check_idx = (next_bowler_idx + i) % bowling_team.size();
                if (bowling_team[check_idx]->getRole() == PlayerRole::BOWLER) {
                    if (current_bowler) current_bowler->setCurrentlyBowling(false);
                    current_bowler = dynamic_cast<Bowler*>(bowling_team[check_idx]);
                    if (current_bowler) current_bowler->setCurrentlyBowling(true);
                    break;
                }
            }
        }
        // --- Modification end ---
    }
        // Termination sequence
    context.match_active = false;
    pthread_mutex_lock(&context.pitch_mutex);
    pthread_cond_broadcast(&context.pitch_cv);
    pthread_mutex_unlock(&context.pitch_mutex);
    
    pthread_mutex_lock(&context.field_mutex);
    pthread_cond_broadcast(&context.field_cv);
    pthread_cond_broadcast(&context.umpire_cv);
    pthread_mutex_unlock(&context.field_mutex);
    
    pthread_mutex_lock(&context.roster_mutex);
    pthread_cond_broadcast(&context.roster_cv);
    pthread_mutex_unlock(&context.roster_mutex);
    
    for (auto player : batting_team) player->joinThread();
    for (auto player : bowling_team) player->joinThread();

    return {total_runs, wickets};
}

// --- Modification start: Main Umpire execution loop and thread management ---
bool Match::run(int num_overs) {
    const std::string first_innings_batting = batting_team_name;
    const std::string first_innings_bowling = bowling_team_name;

    std::cout << "\n========== FIRST INNINGS ==========\n";
    auto first_innings = runInnings(num_overs);
    const int first_runs = first_innings.first;
    const int first_wickets = first_innings.second;
    const int target = first_runs + 1;

    std::cout << "[INNINGS END] " << first_innings_batting << " scored "
              << first_runs << "/" << first_wickets << std::endl;
    std::cout << "[TARGET] " << first_innings_bowling << " need " << target
              << " to win.\n";

    // Swap innings.
    std::swap(batting_team, bowling_team);
    std::swap(batting_team_name, bowling_team_name);

    // Reset per-player transient state before spawning second-innings threads.
    for (auto player : india_team) {
        player->setOut(false);
        player->setStriker(false);
        player->setCurrentlyBowling(false);
    }
    for (auto player : england_team) {
        player->setOut(false);
        player->setStriker(false);
        player->setCurrentlyBowling(false);
    }

    std::cout << "\n========== SECOND INNINGS ==========\n";
    auto second_innings = runInnings(num_overs, target);
    const int second_runs = second_innings.first;
    const int second_wickets = second_innings.second;

    std::cout << "[INNINGS END] " << batting_team_name << " scored "
              << second_runs << "/" << second_wickets << std::endl;

    if (second_runs >= target) {
        const int wickets_remaining = 10 - second_wickets;
        std::cout << "\n[RESULT] " << batting_team_name << " won by "
                  << wickets_remaining << " wickets.\n";
    } else if (second_runs < first_runs) {
        const int margin = first_runs - second_runs;
        std::cout << "\n[RESULT] " << first_innings_batting << " won by "
                  << margin << " runs.\n";
    } else {
        std::cout << "\n[RESULT] Match tied.\n";
    }

    return true;
}
// --- Modification end ---

const std::vector<Player*>& Match::getBattingTeam() const { return batting_team; }
const std::vector<Player*>& Match::getBowlingTeam() const { return bowling_team; }
std::string Match::getBattingTeamName() const { return batting_team_name; }
std::string Match::getBowlingTeamName() const { return bowling_team_name; }
int Match::getCurrentOver() const { return current_over; }
int Match::getCurrentBall() const { return current_ball; }
int Match::getTotalRuns() const { return total_runs; }
int Match::getWickets() const { return wickets; }