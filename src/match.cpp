#include "match.hpp"
#include "loader.hpp"
#include <iostream>
#include <cstdlib>
#include <random>
#include <utility>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static std::string runsDescription(int r) {
    switch (r) {
        case 0: return "dot ball";
        case 1: return "1 run";
        case 2: return "2 runs";
        case 3: return "3 runs";
        case 4: return "FOUR!";
        case 6: return "SIX!";
        default: return std::to_string(r) + " runs";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────
Match::Match() {}

Match::~Match() {
    cleanup();
}

void Match::cleanup() {
    pthread_mutex_destroy(&context.pitch_mutex);
    pthread_cond_destroy(&context.pitch_cv);
    pthread_mutex_destroy(&context.field_mutex);
    pthread_cond_destroy(&context.field_cv);
    pthread_cond_destroy(&context.umpire_cv);
    pthread_mutex_destroy(&context.roster_mutex);
    pthread_cond_destroy(&context.roster_cv);

    for (auto player : india_team)   delete player;
    for (auto player : england_team) delete player;

    india_team.clear();
    england_team.clear();
    batting_team.clear();
    bowling_team.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Load / Toss / Order
// ─────────────────────────────────────────────────────────────────────────────
bool Match::start(const std::string& india_path, const std::string& england_path) {
    if (!TeamLoader::load(india_path, india_team) || !TeamLoader::load(england_path, england_team))
        return false;
    return true;
}

bool Match::toss() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    if (dis(gen) == 0) {
        batting_team = india_team;  bowling_team = england_team;
        batting_team_name = "India"; bowling_team_name = "England";
    } else {
        batting_team = england_team; bowling_team = india_team;
        batting_team_name = "England"; bowling_team_name = "India";
    }
    std::cout << "Bowling team: " << bowling_team_name
              << ", Batting Team: " << batting_team_name << "\n";
    return true;
}

void Match::setBattingOrder(const std::vector<int>& new_indices) {
    if (new_indices.size() != batting_team.size()) return;
    std::vector<Player*> tmp;
    tmp.reserve(batting_team.size());
    for (int i : new_indices) tmp.push_back(batting_team[i]);
    batting_team = tmp;
}
void Match::setBowlingOrder(const std::vector<int>& new_indices) {
    if (new_indices.size() != bowling_team.size()) return;
    std::vector<Player*> tmp;
    tmp.reserve(bowling_team.size());
    for (int i : new_indices) tmp.push_back(bowling_team[i]);
    bowling_team = tmp;
}

// ─────────────────────────────────────────────────────────────────────────────
// Batting scheduler
// ─────────────────────────────────────────────────────────────────────────────
void Match::applyBattingScheduler(const std::string& innings_label) {
    const char* tag = (scheduler_type == SchedulerType::FCFS) ? "FCFS" : "SJF";

    if (scheduler_type == SchedulerType::FCFS) {
        std::stable_sort(batting_team.begin(), batting_team.end(),
            [](const Player* a, const Player* b) {
                return a->getThreadPriority() > b->getThreadPriority();
            });
    } else {
        std::stable_sort(batting_team.begin(), batting_team.end(),
            [](const Player* a, const Player* b) {
                return a->getExpectedBalls() < b->getExpectedBalls();
            });
    }

    std::cout << "\n[" << tag << "] " << innings_label << " batting order:\n";
    for (size_t i = 0; i < batting_team.size(); ++i) {
        const Player* p = batting_team[i];
        if (scheduler_type == SchedulerType::FCFS)
            std::cout << "[" << tag << "]  #" << (i+1) << " " << p->getName()
                      << "  (priority=" << p->getThreadPriority()
                      << ", exp_balls=" << p->getExpectedBalls() << ")\n";
        else
            std::cout << "[" << tag << "]  #" << (i+1) << " " << p->getName()
                      << "  (exp_balls=" << p->getExpectedBalls()
                      << ", priority=" << p->getThreadPriority() << ")\n";
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Waiting-time analysis
// ─────────────────────────────────────────────────────────────────────────────
void Match::printWaitingTimeAnalysis(const std::string& innings_label) const {
    const char* tag = (scheduler_type == SchedulerType::FCFS) ? "FCFS" : "SJF";
    const size_t n = batting_team.size();

    std::cout << "\n[" << tag << "-WAIT] " << innings_label << " waiting times:\n";
    for (size_t i = 0; i < n && i < crease_entry_ball.size(); ++i) {
        int entry = crease_entry_ball[i];
        if (entry < 0) {
            std::cout << "[" << tag << "-WAIT]  #" << (i+1) << " "
                      << batting_team[i]->getName() << "  DNB\n";
        } else {
            int faced = (i < first_faced_ball.size() && has_faced_ball[i]) ? first_faced_ball[i] : -1;
            std::cout << "[" << tag << "-WAIT]  #" << (i+1) << " "
                      << batting_team[i]->getName()
                      << "  crease_entry=" << entry << " balls"
                      << "  first_faced=" << (faced >= 0 ? std::to_string(faced) : "DNB")
                      << "  wait=" << entry << " balls\n";
        }
    }

    // Average wait for middle order (positions 4-7, 0-indexed 3-6)
    std::vector<int> mo;
    for (int i = 3; i <= 6 && i < (int)crease_entry_ball.size(); ++i)
        if (crease_entry_ball[i] >= 0) mo.push_back(crease_entry_ball[i]);

    if (!mo.empty()) {
        double avg = std::accumulate(mo.begin(), mo.end(), 0.0) / mo.size();
        std::cout << "[" << tag << "-WAIT]  >> Middle-order (" << mo.size()
                  << " batted) avg wait: "
                  << std::fixed << std::setprecision(1) << avg << " balls\n";
    } else {
        std::cout << "[" << tag << "-WAIT]  >> Middle-order: none batted.\n";
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Ball-by-ball commentary
// ─────────────────────────────────────────────────────────────────────────────
void Match::printBallLog(const std::string& innings_label) const {
    std::cout << "\n══════════════════════════════════════════════════════════\n";
    std::cout << "  BALL-BY-BALL LOG: " << innings_label << "\n";
    std::cout << "══════════════════════════════════════════════════════════\n";

    int prev_over = -1;
    for (const auto& ev : ball_log) {
        if (ev.over != prev_over) {
            if (prev_over >= 0) std::cout << "\n";
            std::cout << "─── Over " << (ev.over + 1) << " ───\n";
            prev_over = ev.over;
        }

        // Format: "Over X.Y  │ Bowler → Striker │ result │ Score W/L"
        std::ostringstream line;
        line << "  " << std::setw(2) << (ev.over + 1) << "." << ev.ball_in_over
             << "  " << std::left << std::setw(18) << ev.bowler.substr(0, 17)
             << " → " << std::setw(20) << ev.striker.substr(0, 19) << "  ";

        if (ev.is_wide) {
            line << "WIDE (+1)";
        } else if (ev.is_wicket) {
            if (!ev.catcher.empty())
                line << "WICKET  caught by " << ev.catcher;
            else
                line << "WICKET  bowled/LBW";
        } else {
            line << std::setw(10) << runsDescription(ev.runs);
        }

        line << "  │  " << ev.score_runs << "/" << ev.score_wkts;
        std::cout << line.str() << "\n";
    }
    std::cout << "══════════════════════════════════════════════════════════\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Innings scorecard
// ─────────────────────────────────────────────────────────────────────────────
void Match::printInningsScorecard(const std::string& innings_label,
                                   int inns_runs, int inns_wkts,
                                   int extras_wides) const {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "  SCORECARD: " << innings_label << "\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    // Batting table
    std::cout << "\n  BATTING\n";
    std::cout << "  " << std::left << std::setw(24) << "Batsman"
              << std::right << std::setw(5)  << "R"
              << std::setw(5)  << "B"
              << std::setw(5)  << "4s"
              << std::setw(5)  << "6s"
              << std::setw(8)  << "SR" << "\n";
    std::cout << "  " << std::string(52, '-') << "\n";
    for (const auto& rec : bat_records) {
        if (rec.balls == 0 && !rec.is_out) continue; // DNB - skip if never got to bat
        std::string status = rec.is_out
            ? (rec.dismissal_info.empty() ? "out" : rec.dismissal_info)
            : "not out";
        double sr = (rec.balls > 0) ? (100.0 * rec.runs / rec.balls) : 0.0;
        std::cout << "  " << std::left << std::setw(24) << rec.name.substr(0, 23)
                  << std::right << std::setw(5)  << rec.runs
                  << std::setw(5)  << rec.balls
                  << std::setw(5)  << rec.fours
                  << std::setw(5)  << rec.sixes
                  << std::setw(7)  << std::fixed << std::setprecision(1) << sr << "  "
                  << status << "\n";
    }
    std::cout << "  " << std::string(52, '-') << "\n";
    std::cout << "  Extras (wides): " << extras_wides << "\n";
    std::cout << "  Total: " << inns_runs << "/" << inns_wkts << "\n";

    // Bowling table
    std::cout << "\n  BOWLING\n";
    std::cout << "  " << std::left << std::setw(24) << "Bowler"
              << std::right << std::setw(5) << "O"
              << std::setw(5) << "R"
              << std::setw(5) << "W"
              << std::setw(5) << "Wd"
              << std::setw(8) << "Econ" << "\n";
    std::cout << "  " << std::string(52, '-') << "\n";
    for (const auto& br : bowl_records) {
        if (br.balls_bowled == 0) continue;
        int full_overs = br.balls_bowled / 6;
        int rem_balls  = br.balls_bowled % 6;
        double econ = (full_overs > 0 || rem_balls > 0)
            ? (6.0 * br.runs_conceded / br.balls_bowled)
            : 0.0;
        std::string over_str = std::to_string(full_overs)
            + (rem_balls ? ("." + std::to_string(rem_balls)) : "");
        std::cout << "  " << std::left << std::setw(24) << br.name.substr(0, 23)
                  << std::right << std::setw(5) << over_str
                  << std::setw(5) << br.runs_conceded
                  << std::setw(5) << br.wickets
                  << std::setw(5) << br.wides
                  << std::setw(7) << std::fixed << std::setprecision(2) << econ << "\n";
    }
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Gantt chart
// ─────────────────────────────────────────────────────────────────────────────
void Match::printGanttChart(const std::string& innings_label) const {
    if (ball_log.empty()) return;

    const int total_balls = static_cast<int>(ball_log.size());

    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "  GANTT CHART: " << innings_label << "\n";
    std::cout << "  B=batting  n=non-striker  -=pavilion  W=dismissed  .=wide\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    // Print over header (every 6 balls)
    std::cout << "  " << std::left << std::setw(22) << "Player";
    for (int b = 0; b < total_balls; ++b) {
        if (b % 6 == 0) std::cout << "|";
        std::cout << " ";
    }
    std::cout << "|\n";

    // Print ball number header
    std::cout << "  " << std::setw(22) << "";
    for (int b = 0; b < total_balls; ++b) {
        const auto& ev = ball_log[b];
        if (b % 6 == 0) std::cout << "|";
        std::cout << ev.ball_in_over;
    }
    std::cout << "|\n";

    // Over label row
    std::cout << "  " << std::setw(22) << "Over";
    int prev_ov = -1;
    for (int b = 0; b < total_balls; ++b) {
        const auto& ev = ball_log[b];
        if (b % 6 == 0) std::cout << "|";
        if (ev.over != prev_ov && ev.ball_in_over == 1) {
            std::cout << (ev.over + 1) % 10; // single digit over
            prev_ov = ev.over;
        } else {
            std::cout << " ";
        }
    }
    std::cout << "|\n";

    std::cout << "  " << std::string(22, '-');
    for (int b = 0; b <= total_balls; ++b) {
        if (b % 6 == 0) std::cout << "+";
        else             std::cout << "-";
    }
    std::cout << "\n";

    // One row per batsman
    for (const auto& rec : bat_records) {
        const std::string& bname = rec.name;
        if (rec.balls == 0 && !rec.is_out) {
            // Truly DNB – print all dashes
            bool ever_appeared = false;
            for (const auto& ev : ball_log)
                if (ev.striker == bname || ev.non_striker == bname) { ever_appeared = true; break; }
            if (!ever_appeared) continue;
        }

        std::cout << "  " << std::left << std::setw(22) << bname.substr(0, 21);
        for (int b = 0; b < total_balls; ++b) {
            const auto& ev = ball_log[b];
            if (b % 6 == 0) std::cout << "|";
            if (ev.is_wide) {
                std::cout << ".";
            } else if (ev.striker == bname) {
                if (ev.is_wicket) std::cout << "W";
                else              std::cout << "B";
            } else if (ev.non_striker == bname) {
                std::cout << "n";
            } else {
                std::cout << "-";
            }
        }
        std::cout << "|\n";
    }

    // Bowler row (just "X" for every ball they bowl)
    for (const auto& br : bowl_records) {
        if (br.balls_bowled == 0) continue;
        std::cout << "  " << std::left << std::setw(22) << br.name.substr(0, 21);
        for (int b = 0; b < total_balls; ++b) {
            const auto& ev = ball_log[b];
            if (b % 6 == 0) std::cout << "|";
            std::cout << (ev.bowler == br.name ? "X" : " ");
        }
        std::cout << "|\n";
    }

    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// runInnings
// ─────────────────────────────────────────────────────────────────────────────
std::pair<int, int> Match::runInnings(int num_overs, int chase_target) {
    target_overs = num_overs;
    total_runs   = 0;
    wickets      = 0;
    current_over = 0;
    current_ball = 0;

    // Apply batting scheduler
    const std::string innings_label = batting_team_name + " innings";
    applyBattingScheduler(innings_label);

    // SJF context flags
    int max_exp = 1;
    for (const auto* p : batting_team) max_exp = std::max(max_exp, p->getExpectedBalls());
    context.sjf_mode = (scheduler_type == SchedulerType::SJF);
    context.reference_expected_balls = max_exp;

    // ── Waiting-time tracking ──
    const size_t team_size = batting_team.size();
    crease_entry_ball.assign(team_size, -1);
    crease_entry_ball[0] = 0;
    if (team_size > 1) crease_entry_ball[1] = 0;
    first_faced_ball.assign(team_size, -1);
    has_faced_ball.assign(team_size, false);
    int balls_bowled = 0; // legal deliveries only

    // ── Logging / scorecard initialisation ──
    ball_log.clear();
    bat_records.clear();
    bowl_records.clear();
    int extras_wides = 0;

    for (size_t i = 0; i < batting_team.size(); ++i) {
        BatsmanRecord rec;
        rec.name = batting_team[i]->getName();
        bat_records.push_back(rec);
    }
    {
        // Identify unique bowlers
        std::vector<std::string> seen;
        for (auto* p : bowling_team) {
            if (std::find(seen.begin(), seen.end(), p->getName()) == seen.end()) {
                BowlerRecord br;
                br.name = p->getName();
                bowl_records.push_back(br);
                seen.push_back(p->getName());
            }
        }
    }

    // ── KEY DEADLOCK FIX: reset all context state before starting threads ──
    context.match_active        = true;
    context.ball_delivered      = false;
    context.ball_in_air         = false;
    context.ball_dead           = false;
    context.bowler_turn         = false;
    context.fielders_ready      = 10;   // ← critical: umpire must see this as 10 at start
    context.fielders_pending    = 0;
    context.is_wicket_this_ball = false;
    context.is_wide_this_ball   = false;
    context.is_aerial_this_ball = false;
    context.runs_scored_this_ball = 0;
    context.catcher_name.clear();

    // Setup opener pair
    striker     = batting_team[0];
    non_striker = batting_team[1];
    pthread_mutex_lock(&context.roster_mutex);
    striker->setStriker(true);
    non_striker->setStriker(false);
    const char* sched_tag = (scheduler_type == SchedulerType::FCFS) ? "FCFS" : "SJF";
    std::cout << "[" << sched_tag << "] Opener: " << striker->getName()
              << " (striker), " << non_striker->getName() << " (non-striker)\n";
    pthread_cond_broadcast(&context.roster_cv);
    pthread_mutex_unlock(&context.roster_mutex);
    current_bowler = nullptr;

    // Assign fielding quarters + pick first bowler
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
    std::cout << "Opening bowler: " << current_bowler->getName() << "\n";

    // Start all threads
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

    // ── Umpire loop ──
    while (current_over < target_overs && wickets < 10) {
        if (chase_target > 0 && total_runs >= chase_target) break;
        while (current_ball < 6 && wickets < 10) {
            // Wait for fielders to be ready (trapdoor barrier)
            pthread_mutex_lock(&context.field_mutex);
            while (context.fielders_ready < 10 && context.match_active)
                pthread_cond_wait(&context.umpire_cv, &context.field_mutex);

            // Reset per-ball state
            context.ball_dead             = false;
            context.runs_scored_this_ball = 0;
            context.is_wicket_this_ball   = false;
            context.is_wide_this_ball     = false;
            context.is_aerial_this_ball   = false;
            context.ball_in_air           = false;
            context.catcher_name.clear();
            pthread_mutex_unlock(&context.field_mutex);

            // Signal bowler
            pthread_mutex_lock(&context.pitch_mutex);
            context.bowler_turn = true;
            pthread_cond_broadcast(&context.pitch_cv);
            pthread_mutex_unlock(&context.pitch_mutex);

            // Wait for ball to be resolved
            pthread_mutex_lock(&context.field_mutex);
            while (!context.ball_dead && context.match_active)
                pthread_cond_wait(&context.umpire_cv, &context.field_mutex);

            // ── Tally the result ──
            const int runs_this_ball    = context.runs_scored_this_ball;
            const bool is_wicket        = context.is_wicket_this_ball;
            const bool is_wide          = context.is_wide_this_ball;
            const std::string catcher   = context.catcher_name;
            total_runs += runs_this_ball;

            if (is_wide) {
                extras_wides++;
                // Update bowling record
                for (auto& br : bowl_records)
                    if (br.name == current_bowler->getName()) { br.runs_conceded++; br.wides++; break; }
            }

            if (!is_wide) {
                // Track first-ball-faced
                if (striker) {
                    for (size_t k = 0; k < team_size; ++k) {
                        if (batting_team[k] == striker && !has_faced_ball[k]) {
                            first_faced_ball[k] = balls_bowled;
                            has_faced_ball[k]   = true;
                            break;
                        }
                    }
                }

                // Update bowler record
                for (auto& br : bowl_records)
                    if (br.name == current_bowler->getName()) {
                        br.balls_bowled++;
                        br.runs_conceded += runs_this_ball;
                        if (is_wicket) br.wickets++;
                        break;
                    }

                // Update batsman record
                if (striker) {
                    for (auto& rec : bat_records) {
                        if (rec.name == striker->getName()) {
                            rec.balls++;
                            rec.runs  += runs_this_ball;
                            if (runs_this_ball == 4) rec.fours++;
                            if (runs_this_ball == 6) rec.sixes++;
                            break;
                        }
                    }
                }

                balls_bowled++;

                pthread_mutex_lock(&context.roster_mutex);
                std::string dismissed_name;
                if (is_wicket) {
                    dismissed_name = striker ? striker->getName() : "unknown";
                    wickets++;
                    if (striker) { striker->setOut(true); striker->setStriker(false); }

                    // Mark batsman record as out
                    for (auto& rec : bat_records) {
                        if (rec.name == dismissed_name) {
                            rec.is_out = true;
                            if (!catcher.empty()) {
                                rec.dismissal_info = "c " + catcher + " b " + current_bowler->getName();
                            } else {
                                rec.dismissal_info = "b " + current_bowler->getName();
                            }
                            break;
                        }
                    }

                    if (static_cast<size_t>(wickets + 1) < batting_team.size()) {
                        Player* next_p = batting_team[wickets + 1];
                        crease_entry_ball[wickets + 1] = balls_bowled;
                        striker = next_p;
                        if (striker) striker->setStriker(true);
                    } else {
                        striker = nullptr;
                    }
                } else if (runs_this_ball % 2 != 0) {
                    if (striker && non_striker) {
                        striker->setStriker(false);
                        non_striker->setStriker(true);
                        std::swap(striker, non_striker);
                    }
                }
                pthread_cond_broadcast(&context.roster_cv);
                pthread_mutex_unlock(&context.roster_mutex);
            }

            // Release fielders
            context.ball_in_air = false;
            pthread_cond_broadcast(&context.umpire_cv);
            pthread_mutex_unlock(&context.field_mutex);

            // ── Record ball event for log & Gantt ──
            if (!is_wide) {
                BallEvent ev;
                ev.over          = current_over;
                ev.ball_in_over  = current_ball + 1;
                ev.abs_ball      = balls_bowled;
                ev.striker       = striker ? striker->getName() :
                                   (batting_team.size() > (size_t)wickets
                                    ? batting_team[wickets]->getName() : "");
                ev.non_striker   = non_striker ? non_striker->getName() : "";
                // Correct: if wicket just fell, record the dismissed batter
                if (is_wicket && !ball_log.empty()) {
                    // Find who was striker before wicket
                    for (const auto& rec : bat_records)
                        if (rec.is_out && rec.dismissal_info.find(current_bowler->getName()) != std::string::npos
                            && ev.striker != rec.name) {
                            ev.striker = rec.name;
                            break;
                        }
                }
                ev.bowler        = current_bowler ? current_bowler->getName() : "";
                ev.runs          = runs_this_ball;
                ev.is_wicket     = is_wicket;
                ev.is_wide       = false;
                ev.catcher       = catcher;
                ev.score_runs    = total_runs;
                ev.score_wkts    = wickets;
                ball_log.push_back(ev);

                current_ball++;
            } else {
                // Wide – record in log but not in legal-ball Gantt
                BallEvent ev;
                ev.over          = current_over;
                ev.ball_in_over  = current_ball + 1;
                ev.abs_ball      = balls_bowled;
                ev.striker       = striker ? striker->getName() : "";
                ev.non_striker   = non_striker ? non_striker->getName() : "";
                ev.bowler        = current_bowler ? current_bowler->getName() : "";
                ev.runs          = 1;
                ev.is_wicket     = false;
                ev.is_wide       = true;
                ev.score_runs    = total_runs;
                ev.score_wkts    = wickets;
                ball_log.push_back(ev);
            }

            if (chase_target > 0 && total_runs >= chase_target) break;
        } // end ball loop

        current_over++;
        current_ball = 0;

        // Strike rotation at end of over
        if (striker && non_striker) {
            pthread_mutex_lock(&context.roster_mutex);
            striker->setStriker(false);
            non_striker->setStriker(true);
            std::swap(striker, non_striker);
            pthread_cond_broadcast(&context.roster_cv);
            pthread_mutex_unlock(&context.roster_mutex);
        }

        // Bowler rotation (find next bowler-role player)
        int next_idx = -1;
        for (size_t i = 0; i < bowling_team.size(); ++i) {
            if (bowling_team[i] == current_bowler) { next_idx = (i+1)%bowling_team.size(); break; }
        }
        if (next_idx != -1) {
            for (size_t i = 0; i < bowling_team.size(); ++i) {
                int ci = (next_idx + i) % bowling_team.size();
                if (bowling_team[ci]->getRole() == PlayerRole::BOWLER) {
                    if (current_bowler) current_bowler->setCurrentlyBowling(false);
                    current_bowler = dynamic_cast<Bowler*>(bowling_team[ci]);
                    if (current_bowler) current_bowler->setCurrentlyBowling(true);
                    break;
                }
            }
        }
    } // end over loop

    // ── Termination sequence ──
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

    // ── Post-innings output ──
    printBallLog(innings_label);
    printInningsScorecard(innings_label, total_runs, wickets, extras_wides);
    printWaitingTimeAnalysis(innings_label);
    printGanttChart(innings_label);

    return {total_runs, wickets};
}

// ─────────────────────────────────────────────────────────────────────────────
// Main match driver
// ─────────────────────────────────────────────────────────────────────────────
bool Match::run(int num_overs, SchedulerType sched) {
    scheduler_type = sched;
    const char* tag = (sched == SchedulerType::FCFS) ? "FCFS" : "SJF";

    const std::string first_bat  = batting_team_name;
    const std::string first_bowl = bowling_team_name;

    std::cout << "\n══════════════════════════════════════════════════════════\n";
    std::cout << "  FIRST INNINGS [" << tag << "]\n";
    std::cout << "══════════════════════════════════════════════════════════\n";
    auto [r1, w1] = runInnings(num_overs);
    const int target = r1 + 1;

    std::cout << "► " << first_bat << " scored " << r1 << "/" << w1 << "\n";
    std::cout << "► " << first_bowl << " need " << target << " to win.\n";

    // Swap innings
    std::swap(batting_team, bowling_team);
    std::swap(batting_team_name, bowling_team_name);

    // Reset transient per-player state
    for (auto p : india_team)   { p->setOut(false); p->setStriker(false); p->setCurrentlyBowling(false); }
    for (auto p : england_team) { p->setOut(false); p->setStriker(false); p->setCurrentlyBowling(false); }

    std::cout << "\n══════════════════════════════════════════════════════════\n";
    std::cout << "  SECOND INNINGS [" << tag << "]\n";
    std::cout << "══════════════════════════════════════════════════════════\n";
    auto [r2, w2] = runInnings(num_overs, target);

    std::cout << "► " << batting_team_name << " scored " << r2 << "/" << w2 << "\n";

    if (r2 >= target)
        std::cout << "\n[RESULT] " << batting_team_name << " won by " << (10-w2) << " wickets.\n";
    else if (r2 < r1)
        std::cout << "\n[RESULT] " << first_bat << " won by " << (r1-r2) << " runs.\n";
    else
        std::cout << "\n[RESULT] Match tied.\n";

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────
const std::vector<Player*>& Match::getBattingTeam() const { return batting_team; }
const std::vector<Player*>& Match::getBowlingTeam() const { return bowling_team; }
std::string Match::getBattingTeamName() const { return batting_team_name; }
std::string Match::getBowlingTeamName() const { return bowling_team_name; }
int Match::getCurrentOver()  const { return current_over; }
int Match::getCurrentBall()  const { return current_ball; }
int Match::getTotalRuns()    const { return total_runs; }
int Match::getWickets()      const { return wickets; }
