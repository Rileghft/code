#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <iomanip>
#include <string>
#include <vector>
#include <exception>

using namespace std;

//data structure
struct processInfo {
    unsigned id;
    unsigned arrivalTime;
    unsigned burstTime;
};
struct procExecResult {
    unsigned id;
    unsigned sTime;
    unsigned eTime;
};
//global variable
unsigned timeQuantum = 0;
vector<processInfo> process;
vector<pair<unsigned, unsigned> > dependency;
vector<procExecResult> execResult;
//function template
void readFile(char *argv[]);
double fcfs();
double sjf();
double rr();
bool compare(const processInfo &first, const processInfo &second);
unsigned getIndex(const unsigned &id);
void deleteProc(const unsigned &id, vector<processInfo> &vec);
void printMsg(double avgTime, string algorithmName);

int main(int argc, char *argv[])
{
    double avgTime = 0;

    readFile(argv);
    avgTime = fcfs();
    printMsg(avgTime, "FCFS");
    avgTime = sjf();
    printMsg(avgTime, "SJF");
    avgTime = rr();
    printMsg(avgTime, "RR");
    
}

double fcfs()
{
    vector<processInfo> waitingProc;
    vector<processInfo> inputProc = process;
    procExecResult execInfo = {0, 0, 0};
    unsigned passedTime = 0;
    unsigned numWaitProc = 0;
    unsigned totalWaitingTime = 0;
    unsigned execTime = 0;
    bool isFirstProc = true;
    bool isProcExec = false;
    bool procFinish[process.size() + 1];

    do {
        for(vector<processInfo>::iterator it = inputProc.begin(); it != inputProc.end(); ++it)
            if(it->arrivalTime == passedTime) {
                waitingProc.push_back(*it);
                ++numWaitProc;
                inputProc.erase(it);
                --it;
            }
        if(execTime == 0) {
            execInfo.eTime = passedTime;
            if(isFirstProc)
                isFirstProc = false;
            else if(!procFinish[getIndex(execInfo.id)] && isProcExec){
                isFirstProc = false;
                procFinish[getIndex(execInfo.id)] = true;
                execResult.push_back(execInfo);
            }
            if(waitingProc.empty() && inputProc.empty())
                break;
            vector<processInfo>::iterator procInfoIt;
            bool isFoundNext = true;
            for(procInfoIt = waitingProc.begin(); procInfoIt != waitingProc.end(); ++ procInfoIt) {
                if(dependency.empty())
                    break;
                isFoundNext = true;
                for(const auto &dependProc: dependency)
                    if(dependProc.second == procInfoIt->id && !procFinish[getIndex(dependProc.first)]) {
                        isFoundNext = false;
                        break;
                    }
                if(isFoundNext)
                    break;
            }
            if(procInfoIt != waitingProc.end() && isFoundNext) {
                execTime = procInfoIt->burstTime;
                execInfo.id =  procInfoIt->id;
                execInfo.sTime = passedTime;
                --numWaitProc;
                isProcExec = true;
                waitingProc.erase(procInfoIt);
            }
        }
        ++passedTime;
        if(execTime != 0)
            --execTime;
        totalWaitingTime += numWaitProc;
    } while(true);
    return ((double)totalWaitingTime) / process.size();
}

double sjf()
{
    vector<processInfo> waitingProc;
    vector<processInfo> inputProc = process;
    procExecResult execInfo = {0, 0, 0};
    unsigned passedTime = 0;
    unsigned numWaitProc = 0;
    unsigned totalWaitingTime = 0;
    unsigned execTime = 0;
    bool isFirstProc = true;
    bool isProcExec = false;
    bool procFinish[process.size() + 1];

    do {
        for(vector<processInfo>::iterator it = inputProc.begin(); it != inputProc.end(); ++it)
            if(it->arrivalTime == passedTime) {
                waitingProc.push_back(*it);
                ++numWaitProc;
                inputProc.erase(it);
                --it;
            }
        if(execTime == 0) {
            execInfo.eTime = passedTime;
            if(isFirstProc)
                isFirstProc = false;
            else if(!procFinish[getIndex(execInfo.id)] && isProcExec){
                isProcExec = false;
                procFinish[getIndex(execInfo.id)] = true;
                execResult.push_back(execInfo);
            }
            if(waitingProc.empty() && inputProc.empty())
                break;
            sort(waitingProc.begin(), waitingProc.end(), compare);
            bool isFoundNext = true;
            vector<processInfo>::iterator procInfoIt;
            for(procInfoIt = waitingProc.begin(); procInfoIt != waitingProc.end(); ++ procInfoIt) {
                if(dependency.empty())
                    break;
                isFoundNext = true;
                for(const auto &dependProc: dependency)
                    if(dependProc.second == procInfoIt->id && !procFinish[getIndex(dependProc.first)]) {
                        isFoundNext = false;
                        break;
                    }
                if(isFoundNext)
                    break;
            }
            if(procInfoIt != waitingProc.end() && isFoundNext) {
                execTime = procInfoIt->burstTime;
                execInfo.id =  procInfoIt->id;
                execInfo.sTime = passedTime;
                --numWaitProc;
                isProcExec = true;
                waitingProc.erase(procInfoIt);
            }
        }
        ++passedTime;
        if(execTime != 0)
            --execTime;
        totalWaitingTime += numWaitProc;
    } while(true);
    return ((double)totalWaitingTime) / process.size();
}

double rr()
{
    vector<processInfo> waitingProc;
    vector<processInfo> inputProc = process;
    vector<processInfo> newProc;
    procExecResult execInfo = {0, 0, 0};
    unsigned passedTime = 0;
    unsigned timeFrameCounter = timeQuantum;
    unsigned numWaitProc = 0;
    unsigned totalWaitingTime = 0;
    unsigned execTime = 0;
    bool isFirstProc = true;
    bool procFinish[process.size() + 1];

    while(true) {
        for(vector<processInfo>::iterator it = inputProc.begin(); it != inputProc.end(); ++it)
            if(it->arrivalTime == passedTime) {
                waitingProc.push_back(*it);
                ++numWaitProc;
                inputProc.erase(it);
                --it;
            }
        if(waitingProc.empty())
            ++passedTime;
        else
            break;
    }
    do {
        for(vector<processInfo>::iterator it = inputProc.begin(); it != inputProc.end(); ++it)
            if(it->arrivalTime == passedTime) {
                if(timeFrameCounter == 0)
                    newProc.push_back(*it);
                else
                    waitingProc.push_back(*it);
                ++numWaitProc;
                inputProc.erase(it);
                --it;
            }
        if(execTime == 0 || timeFrameCounter == 0) {
            execInfo.eTime = passedTime;
            if(execTime == 0) {
                if(isFirstProc)
                    isFirstProc = false;
                else if(!procFinish[getIndex(execInfo.id)] && timeFrameCounter != timeQuantum){
                    procFinish[getIndex(execInfo.id)] = true;
                    execResult.push_back(execInfo);
                    deleteProc(execInfo.id, waitingProc);
                    for(const auto &procInfo: newProc)
                        waitingProc.push_back(procInfo);
                    newProc.clear();
                }
            }
            else {
                waitingProc.front().burstTime = execTime;
                waitingProc.push_back(waitingProc.front());
                waitingProc.erase(waitingProc.begin());
                execResult.push_back(execInfo);
                for(const auto &procInfo: newProc)
                    waitingProc.push_back(procInfo);
                newProc.clear();
                ++numWaitProc;
            }
            timeFrameCounter = timeQuantum;
            if(waitingProc.empty() && inputProc.empty())
                break;
            vector<processInfo>::iterator procInfoIt;
            bool isFoundNext = false;
            if(!waitingProc.empty())
            for(unsigned i = 0; i < waitingProc.size(); ++i) {
                procInfoIt = waitingProc.begin();
                if(dependency.empty())
                    break;
                isFoundNext = true;
                for(const auto &dependProc: dependency)
                    if(dependProc.second == procInfoIt->id && !procFinish[getIndex(dependProc.first)]) {
                        isFoundNext = false;
                        break;
                    }
                if(isFoundNext)
                    break;
                waitingProc.push_back(waitingProc.front());
                waitingProc.erase(waitingProc.begin());
            }
            if(!waitingProc.empty() && (isFoundNext || dependency.empty())) {
                execTime = procInfoIt->burstTime;
                execInfo.id =  procInfoIt->id;
                execInfo.sTime = passedTime;
                --numWaitProc;
            }
        }
        ++passedTime;
        if(execTime != 0) {
            --execTime;
            --timeFrameCounter;
        }
        totalWaitingTime += numWaitProc;
    } while(true);
    return ((double)totalWaitingTime) / process.size();
}

void readFile(char *argv[])
{
    ifstream fin(argv[1], ios::in);

    string in;
    getline(fin, in, '\n');
    string TQstr = in.substr(3);
    try {
        timeQuantum = stoi(TQstr);
    }
    catch(exception &e) {
        cout << "TQstr: " << TQstr << endl;
        cout << e.what() << endl;
        exit(1);
    }
    unsigned firstProc = 0, secondProc = 0;
    unsigned numberPos = 0;
    while(getline(fin, in, '\n')) {
        try {
            firstProc = stoi(in);
        } catch(exception &e) {break;}
        numberPos = in.find('>');
        secondProc = stoi(in.substr(numberPos + 1));
        dependency.push_back(make_pair(firstProc, secondProc));
    }
    processInfo info;
    while(getline(fin, in, '\n')) {
        try {
           info.id = stoi(in); 
        } catch(exception &e) {break;}
        numberPos = in.find(',');
        info.arrivalTime = stoi(in.substr(numberPos + 1));
        numberPos = in.find(',', numberPos + 1);
        info.burstTime = stoi(in.substr(numberPos + 1));
        process.push_back(info);
    }
    fin.close();
}

bool compare(const processInfo &first, const processInfo &second)
{
    return (first.burstTime < second.burstTime);
}

unsigned getIndex(const unsigned &id)
{
    unsigned index = 0;
    for(const auto &p: process)
        if(p.id == id)
            break;
        else
            ++index;
    return index;
}

void deleteProc(const unsigned &id, vector<processInfo> &vec)
{
    for(vector<processInfo>::iterator it = vec.begin(); it != vec.end(); ++it)
        if(it->id == id) {
            vec.erase(it);
            break;
        }
}

void printMsg(double avgTime, string algorithmName)
{
    cout << "# ID,Starting time, Ending Time" << endl;
    cout << algorithmName << endl;
    for(const auto &result: execResult)
        cout << result.id << ',' << result.sTime << ',' << result.eTime << endl;
    execResult.clear();
    cout << "Average waiting time: " << setprecision(2) << fixed << round(avgTime * 100) / 100 << "ms" << endl;
}
