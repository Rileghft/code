#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <ctime>
#include <exception>
#include <fstream>
#include <unistd.h>
#include <sys/syscall.h>

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
enum msgType {childStatus, found, none};
enum direction {upDir,downDir, leftDir, rightDir};
struct report{
    pthread_t tid[4];
    clock_t cputime[4];
    coordinate location[4];
    unsigned numFound[4];
    msgType msg[4];
};
//share variable
vector<Map *> map;
//function
void init(char *args[]);
void loadMap(char *fileName);
void findNodes(Map *map); 
//unsigned coordinate2offset(coordinate &pos);
void printMap(int &level);
//void printMsg(pid_t pid, coordinate &location, msgType type);
//unsigned int findWays(); 
//bool isValidPos(const coordinate &location);
//direction findOneWay();
//void walk();
//bool search(const direction &dir);
//int *shmFactory(const char *name, size_t spaceSize);
//const char *randName();
//void printCPUtime(clock_t t);

int main(int argv, char *args[])
{
    init(args);
    for(Map *p: map)
        delete p;
}

void init(char *args[])
{
    loadMap(args[1]);
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
    Map *mapPtr;
    for(Map *p: map)
        if(p->level == level) {
            mapPtr = p;
            break;
        }
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
