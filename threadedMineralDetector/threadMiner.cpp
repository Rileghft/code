#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <ctime>
#include <exception>
#include <fstream>
#include <unistd.h>
#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

using namespace std;

//Data Structure
struct coordinate{
    unsigned row;
    unsigned col;
};
struct Map{
    int level = 0;
    coordinate sPoint = {0, 0};
    coordinate upPoint = {0, 0};
    coordinate downPoint = {0, 0};
    unsigned numLine = 0;
    string data;
};
enum msgType {newThread, found, none};
enum direction {noDir = -1, upDir,downDir, leftDir, rightDir};
struct paramType{
    int level = 0;
    Map *mapPtr;
    coordinate predessesor;
    direction walkDir;
};
struct reportType{
    pthread_t tid;
    coordinate pos;
    unsigned numStep = 0;
    unsigned numFound = 0;
    msgType msg;
    clock_t cpuTime;
};
//share variable
vector<Map *> map;
unsigned numThread;
pthread_t firstTid;
//function
void init(char *args[]);
void *explore(void *newParam);
void loadMap(char *fileName);
void findNodes(Map *map); 
void printMap(int &level);
unsigned get_offset(coordinate &pos);
void printMsg(pthread_t pid, coordinate &location, msgType type);
unsigned int findWays(bool *isWayPassable[], coordinate &curPos, Map *mapPtr); 
bool isValidPos(const coordinate &location, Map *mapPtr);
direction findOneWay(bool isWayPassable[]);
unsigned walk(const direction &walkDir, bool isWayPassable[], coordinate &curPos, Map *mapPtr, unsigned &numSteps);
Map *getMapPtr(int level);
pthread_t search(const int &level, Map *mapPtr, const coordinate &curPos, const direction &dir, vector<paramType *> &paramPtr);
void printCPUtime(clock_t &t);

int main(int argv, char *args[])
{
    clock_t cpuTime_start = clock();
    init(args);
    //first thread
    Map *mapPtr = getMapPtr(0);
    printMsg(gettid(),mapPtr->sPoint, newThread);
    
    paramType *param = new paramType;
    param->level = 0;
    param->mapPtr = mapPtr;
    param->predessesor = mapPtr->sPoint;
    param->walkDir = noDir;
    reportType *totalReport =  (reportType *)explore((void *)param);

    printMsg(gettid(), totalReport->pos, totalReport->msg);
    if(totalReport->msg == found) {
        cout << totalReport->numFound << '!' << endl;
    }
    for(Map *p: map) {
        printMap(p->level);
        delete p;
    }
    clock_t cpuTime = totalReport->cpuTime + clock() - cpuTime_start;
    printCPUtime(cpuTime);
    return 0;
}

void init(char *args[])
{
    loadMap(args[1]);
    numThread = 1;
    firstTid = gettid();
}

void *explore(void *newParam)
{
    clock_t cpuTime_start = clock();
    paramType *param = (paramType *)newParam;
    int level = param->level;
    Map *mapPtr = param->mapPtr;
    coordinate curPos = param->predessesor;
    direction walkDir = param->walkDir;
    unsigned numSteps = 0;
    unsigned numWay = 0;
    bool isWayPassable[4];

nextLevel:
    walk(walkDir, isWayPassable, curPos, mapPtr, numSteps);
    walkDir = noDir;
    do {
        numWay = walk(walkDir, isWayPassable, curPos, mapPtr, numSteps);
    }while(numWay == 1) ;
    char *underFootChar = &mapPtr->data[get_offset(curPos)];
    if(numWay == 0)
    {
        reportType *report = new reportType;
        report->tid = gettid();
        report->pos = curPos;
        report->numStep = numSteps;
        switch(*underFootChar)
        {
            case 'K':
                report->msg = found;
                report->numFound = 1;
                break;
            case 'U':
                delete report;
                mapPtr = getMapPtr(level + 1);
                curPos = mapPtr->downPoint;
                cout << "go up!" << endl;
                goto nextLevel;
                break;
            case 'D':
                delete report;
                mapPtr = getMapPtr(level - 1);
                curPos = mapPtr->upPoint;
                cout << "go down!" << endl;
                goto nextLevel;
                break;
            default:
                report->msg = none;
                report->numFound = 0;
                *underFootChar = 'X';
                break;
        };
        report->cpuTime = clock() - cpuTime_start;

        if(gettid() == firstTid)
            return report;
        pthread_exit(report);
    }
    //there are >1 ways to walk
    if(*underFootChar != 'K' && *underFootChar != 'S' && *underFootChar != 'U' && *underFootChar != 'D')
        *underFootChar = '#';
    //create threads to search different ways
    vector<pthread_t> tids;
    vector<paramType *>paramPtr;
    for(unsigned dir = upDir; dir <= rightDir; ++dir)
        if(isWayPassable[dir])
            tids.push_back(search(level, mapPtr, curPos, (direction)dir, paramPtr));
    //check children report
    unsigned numFound = 0;
    msgType isFound = none;
    reportType *rValue;
    clock_t child_cpuTime = 0; 
    unsigned numChildSteps = 0;
    for(pthread_t tid: tids) {
        pthread_join(tid, (void **)&rValue);
        if(rValue->msg == found){
            numFound += rValue->numFound;
            isFound = found;
            printMsg(rValue->tid, rValue->pos, rValue->msg);
            cout << rValue->numFound << '!' << endl;
        }
        else
            printMsg(rValue->tid, rValue->pos, rValue->msg);
        numChildSteps += rValue->numStep;
        child_cpuTime += rValue->cpuTime;
        delete rValue;
    }
    for(paramType *param: paramPtr) {
        delete param;
    }
    //report
    reportType *report = new reportType;
    report->tid = gettid();
    report->pos = curPos;
    report->numStep = numSteps;
    report->numFound = numFound;
    if(isFound == found)
        report->msg = found;
    else
        report->msg = none;
    report->cpuTime = child_cpuTime + clock() - cpuTime_start;
    if(gettid() == firstTid)
        return report;
    pthread_exit(report);
}

pthread_t search(const int &level, Map *mapPtr, const coordinate &curPos, const direction &dir, vector<paramType *> &paramPtr)
{
    numThread += 1;
    paramType *newParam = new paramType;
    paramPtr.push_back(newParam);
    newParam->level = level;
    newParam->mapPtr = mapPtr;
    newParam->predessesor = curPos;
    newParam->walkDir = dir;
    //create pthread
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, explore, (void *)newParam);
    coordinate newPos = curPos;
    printMsg(tid, newPos, newThread);
    return tid;
}

void loadMap(char *fileName)
{
    ifstream fin(fileName, ios::in);

    Map *newMap = new Map;
    string row;
    unsigned level = 0;
    bool notFirst = false;

    while(getline(fin, row, '\n')) {
        if(newMap->numLine % 10 == 0) {
            try {
                level = stoi(row, 0, 10);
                if(notFirst) {
                    findNodes(newMap);
                    map.push_back(newMap);
                    newMap = new Map;
                    newMap->level = level;
                }
                else {
                    newMap->level = level;
                    notFirst = true;
                }
                continue;
            }
            catch(exception& e) {
            }
        }
        newMap->data += row;
        newMap->numLine += 1;
    }
    findNodes(newMap);
    newMap->level = level;
    map.push_back(newMap);
}

void printMap(int &level)
{
    Map *mapPtr = getMapPtr(level);
    cout << "Map Info: Level=" << mapPtr->level << " Size=" << mapPtr->numLine << "X20" << endl;
    for(unsigned row= 0; row < mapPtr->numLine; ++row) {
        for(int col = 0; col < 20; ++col)
            cout << mapPtr->data[row * 20 + col];
        cout << endl;
    }
}

void findNodes(Map *map)
{
    for(unsigned row = 0; row < map->numLine; ++row) 
        for(unsigned col = 0; col < 20; ++col)
            switch(map->data[row * 20 + col])
            {
                case 'S':
                    map->sPoint = {row, col};
                    break;
                case 'U':
                    map->upPoint = {row, col};
                    break;
                case 'D':
                    map->downPoint = {row, col};
                    break;
            }
}

unsigned get_offset(coordinate &pos)
{
    return pos.row * 20 + pos.col;
}

Map *getMapPtr(int level)
{
    Map *mapPtr;
    for(Map *p: map)
        if(p->level == level) {
            mapPtr = p;
            break;
        }
    return mapPtr;
}

void printMsg(pthread_t tid, coordinate &location, msgType type)
{
    switch(type)
    {
        case newThread:
            printf("[tid=%llu]: (%u,%u)\n", tid, location.row, location.col);
            break;
        case found:
            printf("[tid=%llu]: (%u,%u) Found ", tid, location.row, location.col);
            break;
        case none:
            printf("[tid=%llu]: (%u,%u) None!\n", tid, location.row, location.col);
            break;
        default:
            cout << "Error invalid msgType" << endl;
            break;
    }
}

unsigned int findWays(bool isWayPassable[], coordinate &curPos, Map *mapPtr)
{
    unsigned numWay = 0;
    coordinate fourDir[4];
    char fourDirChar[4];
    //initialization
    for(int i = 0; i < 4; ++i){
        isWayPassable[i] = false;
        fourDir[i] = curPos;
        fourDirChar[i] = 'X';
    }
    //modify four direction locations
    fourDir[upDir].row -= 1;      //up
    fourDir[downDir].row += 1;    //down
    fourDir[leftDir].col -= 1;    //left
    fourDir[rightDir].col += 1;   //right
    //get underfoot character
    for(int i = 0; i < 4; ++i)
        if(isValidPos(fourDir[i], mapPtr))
            fourDirChar[i] = mapPtr->data[get_offset(fourDir[i])];
    //calculate how many ways is passable
    for(int i = 0; i < 4; ++i)
        if(fourDirChar[i] == ' ' || fourDirChar[i] == 'K' || fourDirChar[i] == 'U' || fourDirChar[i] == 'D') {
            ++numWay;
            isWayPassable[i] = true;
        }

    return numWay;
}

direction findOneWay(bool isWayPassable[])
{
    for (unsigned dir = 0; dir < 4; ++dir)
        if(isWayPassable[dir])
            return (direction)dir;
}

unsigned walk(const direction &walkDir, bool isWayPassable[], coordinate &curPos, Map *mapPtr, unsigned &numSteps)
{
    unsigned numWay = 0;
    direction dir = walkDir;
    if(walkDir == noDir) {
        numWay = findWays(isWayPassable, curPos, mapPtr);
        if(numWay > 1 || numWay == 0)
            return numWay;
        dir = findOneWay(isWayPassable);
    }
    switch(dir)
    {
        case upDir:
            curPos.row -= 1;
            break;
        case downDir:
            curPos.row += 1;
            break;
        case leftDir:
            curPos.col -=1;
            break;
        case rightDir:
            curPos.col += 1;
            break;
    }
    ++numSteps;
    char *underFootChar = &mapPtr->data[get_offset(curPos)];
    if(*underFootChar != 'K' && *underFootChar != 'U' && *underFootChar != 'D' && *underFootChar != 'S')
        *underFootChar = '-';
    return numWay;
}

bool isValidPos(const coordinate &location, Map *mapPtr)
{
    if(location.col > 19 || location.col < 0)
        return false;
    if(location.row >= mapPtr->numLine || location.row < 0)
        return false;
    //valid location
    return true;
}

void printCPUtime(clock_t &t)
{
    printf("Total: %2f ms\n", (double)t / 1000);
}
