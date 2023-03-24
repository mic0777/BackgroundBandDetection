//
//  worker.cpp
//  BackgroundBand Detection
//
//  Created by Michael Philatov
//  Copyright © 2023. All rights reserved.
//

#include <stdexcept>
#include <fstream>
#include "worker.h"
#include "utility.h"

using namespace cv;

Worker::Worker(string path) : path(path)
{
    cap = VideoCapture(path.c_str());
    if (!cap.isOpened()) {
        throw invalid_argument("unable to open video file: " + path);
    }
    width = cap.get(CAP_PROP_FRAME_WIDTH);
    height = cap.get(CAP_PROP_FRAME_HEIGHT);
    fps = cap.get(CAP_PROP_FPS);
    {
        lock_guard<mutex> lck(statusMutex);
        totalFrames =  cap.get(CAP_PROP_FRAME_COUNT);
    }
    if (totalFrames <= 0) {
        throw invalid_argument("no frames in video file: " + path);
    }

    threadObject = thread(&Worker::run, this);
}

Worker::~Worker()
{
    threadObject.join();
}

void Worker::run()
{
    setStatus(0);
    Mat m, prevM;
    int frameN{0};

    for (int frameN=0; frameN < totalFrames; ++frameN) {
        cap >> m;
        if (m.empty()) {
            if (frameN < totalFrames-1) {
                throw invalid_argument("incomplete video file: " + path);
            }
            break;
        }
        upperLines.push_back(detectStaticBackgroundLines(m, upper, lower));

        pair<int, int> p = detectStaticBackgroundByFrameChange(prevM, m);
        if (p.first > 0) {
            frameAppear[frameN] = p;
        }
        prevM = m;

        imshow("Background Bar", m);
        waitKey(1);

        if (frameN % 40 == 0) {
            setStatus((100 * frameN) / totalFrames);
        }
    }
    analyzeResults();
    setStatus(100);
}

void Worker::analyzeResults()
{
    map<int, int> upperStat, lowerStat;
    for (auto u : upper) {
        upperStat[u]++;
    }
    for (auto l : lower) {
        lowerStat[l]++;
    }
    int upperMode{-1}, lowerMode{-1};
    for (auto v : upperStat) {
        if (upperMode < 0 || upperStat[upperMode] < v.second) {
            upperMode = v.first;
        }
    }
    for (auto v : lowerStat) {
        if (lowerMode < 0 || lowerStat[lowerMode] < v.second) {
            lowerMode = v.first;
        }
    }
    if (upperMode > 0 && upperStat[upperMode] > totalFrames*0.3) {
        for (auto it=frameAppear.begin(); it!=frameAppear.end(); ++it) {
            int &upper = it->second.first;
            int &lower = it->second.second;
            if (fabs(upper - upperMode) < 40) {
                upper = upperMode;
                if (lowerMode > 0 && lowerStat[lowerMode] > totalFrames*0.2 && fabs(lower - lowerMode) < 40) {
                    lower = lowerMode;
                }else lower = -1;
            }else {
                upper = -1;
            }
        }
        // remove frameAppear elements with upper == -1
        auto it = frameAppear.begin();
        while (it != frameAppear.end()) {
            if (it->second.first < 0) {
                frameAppear.erase(it);
                it = frameAppear.begin();
            }else ++it;
        }
        if (frameAppear.size() == 0) {
            frameAppear[0] = make_pair(upperMode, lowerMode);
        }
    }else upperMode = -1;

    if (upperMode < 0) { // bad file, no background            
        throw invalid_argument("background bands not found in video file: " + path);
    }
    for (auto &u : upper) {
        if (abs(u - upperMode) < 5) {
            u = upperMode;
        }
    }
    if (lowerMode > 0) {
        for (auto &l : lower) {
            if (abs(l - lowerMode) < 5) {
                l = lowerMode;
            }
        }
    }
    for (auto &m : upperLines)
        for (auto &l : m) {
        if (abs(l.first - upperMode) < 5) {
            m[upperMode] = l.second;
        }
    }
    // detect missing frameAppear/disappear by upperLines
    bool frameIsON = false;
    int cntLinesPresent{0}, cntLinesMissing{0};
    for (int n=0; n<totalFrames; ++n) {
        if (frameAppear.count(n) > 0) {
            frameIsON = true;
            cntLinesPresent++;
            cntLinesMissing = 0;
            continue;
        }
        if (upperLines[n].count(upperMode) && upperLines[n][upperMode]) {
            cntLinesPresent++;
            cntLinesMissing = 0;
        }else if (upperLines[n].count(upperMode) == 0) {
            cntLinesPresent = 0;
            cntLinesMissing++;
        }
        if (!frameIsON && cntLinesPresent > 30) {
            frameIsON = true;
            frameAppear[n-cntLinesPresent].first = upperMode;
            frameAppear[n-cntLinesPresent].second = lowerMode;
            continue;
        }
        if (cntLinesMissing > 30) {
            frameIsON = false;
            frameDisappear.push_back(n-cntLinesPresent);
            cntLinesMissing = 0;
        }
    }
    // merge results:
    map<int, bool> events;
    for (auto it=frameAppear.begin(); it!=frameAppear.end(); ++it) {
        events[it->first] = true;
    }
    for (int t : frameDisappear) {
        events[t] = false;
    }
    // write results to csv file:
    ofstream f(path + ".csv");
    f << "FrameN,upperY,lowerY" << endl;
    for (auto e : events) {
        if (e.second) {
            f << e.first << "," << upperMode << "," << lowerMode << endl;
        }else f << e.first << "," << -1 << "," << -1 << endl;
    }


}
int Worker::getStatus()
{
    lock_guard<mutex> lck(statusMutex);
    return status;
}

void Worker::setStatus(int percents)
{
    lock_guard<mutex> lck(statusMutex);
    status = percents;
}

bool Worker::finished()
{
    lock_guard<mutex> lck(statusMutex);
    return status == 100;
}

map<int, bool> Worker::detectStaticBackgroundLines(const Mat &m, vector<int> &upper, vector<int> &lower)
{
    map<int, bool> result;
    for (int y=m.rows/2; y<m.rows; ++y) {
        int nDarkened{0}, nLightened{0}, nDarkenedCur{0}, nLightenedCur{0}, zerosD{0}, zerosL{0}, longestDark{0}, longestLight{0};
        if (y == 284) {
            int jhg=0;
        }
        vector<double> darkenCoeffs, lightenCoeffs;
        for (int x=0; x<m.cols; ++x) {
            int H1, S1, V1;
            int H2, S2, V2;
            RGB2HSV(m.data+m.channels()*((y-1)*m.cols+x), H1, S1, V1);
            RGB2HSV(m.data+m.channels()*(y*m.cols+x), H2, S2, V2);
            if ((V1 < 20 && V2 < 20)) {
                if (V2 < 15) ++zerosD;
                if (V1 < 15) ++zerosL;
                if (longestDark < nDarkenedCur) longestDark = nDarkenedCur;
                nDarkenedCur = 0;
                if (longestLight < nLightenedCur) longestLight = nLightenedCur;
                nLightenedCur = 0;
                continue;
            }
            double k = (V2-V1)/double(V2+V1);
            if (k < -0.1) {
                // upper
                nDarkened++;
                nDarkenedCur++;
                if (V1 > 50 && V2 > 50) {
                    darkenCoeffs.push_back(fabs(k));
                }
            }else {
                if (longestDark < nDarkenedCur) longestDark = nDarkenedCur;
                nDarkenedCur = 0;
            }
            if (k > 0.1) {
                //lower
                nLightened++;
                nLightenedCur++;
                if (V1 > 50 && V2 > 50) {
                    lightenCoeffs.push_back(fabs(k));
                }
            }else {
                if (longestLight < nLightenedCur) longestLight = nLightenedCur;
                nLightenedCur = 0;
            }
        }
        if (longestDark < nDarkenedCur) longestDark = nDarkenedCur;
        if (longestLight < nLightenedCur) longestLight = nLightenedCur;

        if (nDarkened > 0.9*m.cols) longestDark = nDarkened;
        if ((y < m.rows*0.8) && nDarkened > 0.2*m.cols && longestDark > 0.1*m.cols && (nDarkened+zerosD) > 0.4*m.cols) {
            if (calcVariance(darkenCoeffs) < 0.1) {
                result[y] = ((zerosD < 0.2*m.cols || nDarkened > 0.5*m.cols) ? true : false);
                upper.push_back(y);
            } else result[y] = false;
        }
        if (nLightened > 0.9*m.cols) longestLight = nLightened;
        if ((y > m.rows*0.9) && nLightened > 0.2*m.cols && longestLight > 0.1*m.cols && (nLightened+zerosL) > 0.5*m.cols) {
            if (calcVariance(lightenCoeffs) < 0.1) lower.push_back(y);
        }
    }
    return result;
}

pair<int, int> Worker::detectStaticBackgroundByFrameChange(const Mat &prev, const Mat&m)
{
    if (prev.rows == 0) {
        return {-1, -1};
    }

    int firstY{-1}, lastY{-1}, errorLines{0}, darkenedLines{0};
    vector<double> darkenCoeffs;
    for (int y=0; y<m.rows; ++y) {
        int nDarkened{0}, nDarkened2{0}, nSame{0}, zeros{0};
        if (y > 300) {
            int jhg=0;
        }
        for (int x=0; x<m.cols; ++x) {
            int H1, S1, V1;
            int H2, S2, V2;
            RGB2HSV(m.data+m.channels()*(y*m.cols+x), H1, S1, V1);
            RGB2HSV(prev.data+m.channels()*(y*m.cols+x), H2, S2, V2);
            double colorDist = calcColorDistance(m.data+m.channels()*(y*m.cols+x), prev.data+m.channels()*(y*m.cols+x));
            int dH = abs(H1 - H2);
            if (dH > 360/2) dH = 360 - dH;
            double k = (V2-V1)/double(V2+V1);
            if (dH < 50  && k > 0.1) nDarkened2++;
            bool zero = false;
            if ((V1 < 5 && V2 < 5) || (V1 < 15 && V2 < 25)) {++zeros; zero = true;}
            if (V1 < 20 && V2 < 25) {
                if (fabs(k) < 0.1 && !zero) ++nSame;
                continue;
            }  // both black
            if (V1>150 && V2>150) {if (fabs(k) < 0.1 && dH < 50) ++nSame; continue;} // both white
            //if (S1 < 20 && S2 < 20) continue; // both gray or white - no color

            if (dH < 50 && ((S1 > 100 && S2 > 100) || (abs(S1-S2) < 50) || dH <5)) { // same color
                if (V2-V1 > 100 || k > 0.20) {
                //if (V2-V1 > 100 || ((V2-V1)/double(V2+V1) > 0.18)) {
                    nDarkened++; if (firstY > 0) darkenCoeffs.push_back(k);
                    if (k < 0.5 || k > 0.7) {
                        int jhg=0;
                    }

                }else {
                    if (fabs(k) < 0.1) {
                        nSame++;
                    }
                }
            }else {
                if (dH < 10 && colorDist < 50 && fabs(k) < 0.5) {
                    nSame++;
                }
            }
        }
        if (nDarkened > m.cols*0.55 || (y > m.rows/2 && nDarkened > m.cols*0.2 && (nDarkened+zeros) > m.cols*0.55) ||
                (y > m.rows/2 && firstY > 0 && (lastY - firstY) > m.rows*0.2 && zeros > m.cols*0.3))  {
            ++darkenedLines;
            if (firstY < 0) firstY = y;
            lastY = y;
        }else if (firstY > 0 && nDarkened+zeros > m.cols*0.55) {
            lastY = y;
        }
        if (firstY < 0) {
            if ((nSame+zeros) < m.cols*0.7) {
                if (++errorLines > m.rows*0.3) {
                    return {-1, -1};
                }
            }else if (y < m.rows/2 &&
                (nDarkened2 > m.cols*0.3 && (nDarkened+zeros) > m.cols*0.55)) {
                if (++errorLines > m.rows*0.3) {
                    return {-1, -1};
                }
            }
        }
    }
    if ((lastY - firstY) > m.rows*0.2 && firstY > m.rows/2 && lastY > m.rows*0.85 && (darkenedLines > 0.6*(lastY-firstY))) {
        if (calcVariance(darkenCoeffs) > 0.1) return {-1, -1};

        return {firstY, lastY};
    }
    return {-1, -1};
}

