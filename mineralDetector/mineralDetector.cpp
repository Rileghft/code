#include <sys/types.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace::std;

//Data Structure
struct coordinate{
    unsigned row;
    unsigned col;
};

struct mapFile{
    char *src;
    coordinate sPoint;
    unsigned numLine;
};

enum msgType {childStatus, found, none};
enum direction {upDir,downDir, leftDir, rightDir};

//global variable
const char *shareMapName = "shareMap";
mapFile map;
unsigned *numProcess;
pid_t firstPid;
pid_t childPid;
vector<pid_t> childrenPid;
coordinate location;
direction walkDirection;
bool isWayPassable[4];
bool isChild;
//function declaration
void init(char *args[]);
void loadMap(char *fileName);
coordinate findStartPoint(void); 
unsigned coordinate2offset(coordinate &pos);
void printMap();
void printMsg(pid_t pid, msgType type);
unsigned int findWays(); 
bool isValidPos(const coordinate &location);
direction findOneWay();
void walk();
bool search(const direction &dir);
int *shmFactory(const char *name, size_t spaceSize);

int main(int argv, char *args[])
{
    //first process
    init(args);
    printMap();
    printMsg(firstPid, childStatus);
    
Loop:
    //error occur
    if(childPid < 0)
    {
        cout << "error pid is less than 0" << endl;
        return -1;
    }
    //child task
    else if(childPid == 0)
    {
       if(*numProcess < 400)
       {
           unsigned numWay = findWays();
           if(numWay == 0)
           {
               if( map.src[coordinate2offset(location)] == 'K'){
                    printMsg(getpid(), found);
                    return true;
               }
               else{
                    printMsg(getpid(), none);
                    return false;
               }
           }
           else if(numWay == 1)
           {
                walkDirection = findOneWay();
                walk();
           }
           else
           {
               isChild = false;
               childrenPid.clear();
               for(unsigned dir = upDir; dir <= rightDir; ++dir)
                   if(isWayPassable[dir]){
                        isChild = search((direction)dir);
                        if(isChild)
                            goto Loop;
                   }
           }
           //parent create child finish
           goto Loop;
       }
       else{
            cout << "error create too many processes, it may be a process bomb." << endl;
            cout << "numProcess: " <<  *numProcess << endl;
            return -1;
       }
    }
    //parent task
    else
    {
        delete numProcess;
        bool isFound = false;
        int rValue = 0;
        for(int i = 0; i < childrenPid.size(); ++i) {
            waitpid(childrenPid[i], &rValue, 0);
            if(WEXITSTATUS(rValue) == true) {
                isFound = true;
                break;
            }
        }
        if(getpid() == firstPid)
            shm_unlink(shareMapName);
        //print isfound message
        if(isFound)
            printMsg(getpid(), found);
        else
            printMsg(getpid(), none);
        if(getpid() == firstPid){
            printMap();
        }
        return isFound;
    }

    return 0;
}

void init(char *args[])
{
    numProcess = new unsigned;
    *numProcess = 1;
    loadMap(args[1]);
    firstPid = getpid();
    location = map.sPoint;
}

void loadMap(char *fileName)
{
    //read data from a file
    string row;
    string mapData;
    int numLine = 0;
    ifstream fin(fileName, ios::in);
    while(getline(fin, row, '\n')){
        mapData += row;
        ++numLine;
    }
    cout << "Map Size: " << numLine << " X 20" << endl;
    fin.close();

    //create a share memory object
    const size_t mapSize = numLine * 20;
    
    map.src = (char *)shmFactory(shareMapName, mapSize);
    //initialize map
    for(int i = 0; i < mapData.size(); ++i)
        map.src[i] = mapData[i];
    map.numLine = numLine;
    map.sPoint = findStartPoint();
    
}

unsigned coordinate2offset(coordinate &pos)
{
    return pos.row * 20 + pos.col;
}

coordinate findStartPoint()
{
    for(int row = 0; row < map.numLine; ++row)
        for(int col = 0; col < 20; ++col)
            if(map.src[row * 20 + col] == 'S') {
                coordinate startPoint = {row ,col};
                return startPoint;
            }
    //error
    cout << "Cannot find start point!" << endl;
    exit(-1);
}

void printMap()
{
    for(int row = 0; row < map.numLine; ++row) {
        for(int col = 0; col < 20; ++col)
            cout << map.src[row * 20 + col];
        cout << endl;
    }
    cout << endl;
}

void printMsg(pid_t pid, msgType type)
{
    switch(type)
    {
        case childStatus:
            cout << "[pid=" << pid << "]: (" << location.row << "," << location.col << ")" << endl;
            break;
        case found:
            cout << pid << " (" << location.row << "," << location.col << ")" << "Found!" << endl;
            break;
        case none:
            cout << pid << " (" << location.row << "," << location.col << ")" << "None!" << endl;
            break;
        default:
            cout << "Error invalid msgType" << endl;
            break;
    }
}

unsigned int findWays()
{
    unsigned numWay = 0;
    coordinate fourDirection[4];
    char fourDirChar[4];
    //initialization
    for(int i = 0; i < 4; ++i){
        isWayPassable[i] = false;
        fourDirection[i] = location;
        fourDirChar[i] = 'X';
    }
    //modify four direction locations
    fourDirection[upDir].row -= 1;      //up
    fourDirection[downDir].row += 1;    //down
    fourDirection[leftDir].col -= 1;    //left
    fourDirection[rightDir].col += 1;   //right
    //get underfoot character
    for(int i = 0; i < 4; ++i)
        if(isValidPos(fourDirection[i]))
            fourDirChar[i] = map.src[coordinate2offset(fourDirection[i])];
    //calculate how many ways is passable
    for(int i = 0; i < 4; ++i)
        if(fourDirChar[i] == ' ' || fourDirChar[i] == 'K') {
            ++numWay;
            isWayPassable[i] = true;
        }

    return numWay;
}

direction findOneWay()
{
    for (unsigned dir = 0; dir < 4; ++dir)
        if(isWayPassable[dir])
            return (direction)dir;
}

void walk()
{
    unsigned offset = coordinate2offset(location);
    switch(walkDirection)
    {
        case upDir:
            location.row -= 1;
            break;
        case downDir:
            location.row += 1;
            break;
        case leftDir:
            location.col -=1;
            break;
        case rightDir:
            location.col += 1;
            break;
    }
    if(map.src[offset] != 'K')
        map.src[offset] = '-';
}

bool isValidPos(const coordinate &location)
{
    if(location.col > 19 || location.col < 0)
        return false;
    if(location.row >= map.numLine || location.row < 0)
        return false;
    //valid location
    return true;
}

bool search(const direction &dir)
{
    *numProcess += 1;
    walkDirection = dir;
    childPid = 0;
    childPid = fork();      
    if(childPid == 0){
        walk();
        return true;
    }
    else{
        printMsg(childPid, childStatus);
        childrenPid.push_back(childPid);
        return false;
    }
}

int *shmFactory(const char *name, size_t spaceSize)
{
    int shm_fd;
    void *shmPtr;

    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, spaceSize);
    shmPtr = mmap(0, spaceSize, PROT_WRITE, MAP_SHARED, shm_fd, 0);

    return (int *)shmPtr;
}
