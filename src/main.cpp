#include "loader.hpp"
#include "match.hpp"
#include <iostream>
#include <vector>
using namespace std;

static bool runMatch(SchedulerType sched, const char* label, int num_overs) {
    cout << "\n\n"
         << "============================================================\n"
         << "  MATCH: " << label << "\n"
         << "============================================================\n";

    Match match;
    if (!match.start("stats/india.txt", "stats/england.txt")) {
        cerr << "Failed to load teams for " << label << endl;
        return false;
    }
    if (!match.toss()) {
        cerr << "Failed to conduct toss for " << label << endl;
        return false;
    }
    cout << "Starting match now (" << label << ")" << endl;
    cout.flush();

    if (!match.run(num_overs, sched)) {
        cerr << "Failed to run " << label << endl;
        return false;
    }
    cout << "\n" << label << " completed successfully!\n";
    return true;
}

int main() {
    try {
        const int NUM_OVERS = 20000; // No over limit: innings ends when all 10 wickets fall.

        // --- Match 1: FCFS ---
        if (!runMatch(SchedulerType::FCFS, "Match 1 - FCFS Scheduler", NUM_OVERS)) return 1;

        // --- Match 2: SJF ---
        if (!runMatch(SchedulerType::SJF, "Match 2 - SJF Scheduler", NUM_OVERS)) return 1;

        cout << "\nBoth matches completed successfully!\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
