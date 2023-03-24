//
//  main.cpp
//  BackgroundBand Detection
//
//  Created by Michael Philatov
//  Copyright © 2023. All rights reserved.
//
#include <iostream>
#include <filesystem>
#include <chrono>
#include "worker.h"

using namespace std;

int main(int argc, char**argv) {
    if (argc != 2) {
        cout << "Wrong number of params. Use: BackgroundBand <folder with mp4 files>" << endl;
        return 0;
    }
    string path(argv[1]);
    try{
        for (const auto & entry : filesystem::directory_iterator(path)) {
            cout << entry.path() << endl;
            Worker w(entry.path().u8string());
            while(!w.finished()) {
                cout << w.getStatus() << "%\r" << flush;
                this_thread::sleep_for(1s);
            }
            cout << w.getStatus() << "%\n";
        }
    }
    catch(exception& ex) {
        cout << ex.what() << endl;
    }

    return 0;
}
