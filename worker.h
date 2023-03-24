//
//  worker.h
//  BackgroundBand Detection
//
//  Created by Michael Philatov
//  Copyright © 2023. All rights reserved.
//

#include <map>
#include <thread>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc.hpp"

using namespace std;

class Worker
{
 public:
    Worker(string path);
    ~Worker();
    int getStatus();  // returns percents done
    bool finished();

 protected:
    string path;
    int width{0};
    int height{0};
    int fps{0};
    int totalFrames{0};
    int status{0};
    mutex statusMutex;
    thread threadObject;
    cv::VideoCapture cap;

    // detected background band coordinates:
    vector<int> upper, lower;
    vector<map<int, bool>> upperLines;

    // detected background band appear and disappear positions:
    map<int, pair<int, int>> frameAppear;
    vector<int> frameDisappear;

    void run();
    void analyzeResults();
    map<int, bool> detectStaticBackgroundLines(const cv::Mat &m, vector<int> &upper, vector<int> &lower);
    pair<int, int> detectStaticBackgroundByFrameChange(const cv::Mat &prev, const cv::Mat&m);
    void setStatus(int percents);
};