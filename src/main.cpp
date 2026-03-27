#include "loader.hpp"
#include "match.hpp"
#include <iostream>
using namespace std;

static bool runMatch(SchedulerType sched, const char* label) {
    cout << "\n\n"
         << "############################################################\n"
         << "  MATCH: " << label << "\n"
         << "############################################################\n";

    Match match;
    if (!match.start("stats/india.txt", "stats/england.txt")) {
        cerr << "Failed to load teams for " << label << "\n";
        return false;
    }
    if (!match.toss()) return false;

    // Real T20: 20 overs per innings, innings ends at 10 wickets or 20 overs
    return match.run(20, sched);
}

int main() {
    try {
        if (!runMatch(SchedulerType::FCFS, "Match 1 - FCFS Scheduler")) return 1;
        if (!runMatch(SchedulerType::SJF,  "Match 2 - SJF Scheduler"))  return 1;
        cout << "\nBoth matches completed successfully!\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
