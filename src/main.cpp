#include "loader.hpp"
#include "match.hpp"
#include <iostream>
#include <vector>
using namespace std;
int main() {
    try {
        Match match;
        if (!match.start("stats/india.txt", "stats/england.txt")) {
            cerr << "Failed to start match" << endl;
            
            return 1;
        }
        if (!match.toss()) {
            cerr << "Failed to conduct toss" << endl;
            cout.flush();
            return 1;
        }
        cout << "starting match now" << endl;
         cout.flush();
        if (!match.run(20000)) {
            cerr << "Failed to run match" << endl;
            return 1;
        }

        cout << "\nMatch completed successfully!" << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}