#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <string>

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
enum direction {up,down, leftDir, rightDir};

//global variable
pid_t firstPid;
const char *shareMemName = "shareMap";
int shm_fd; //share memory file descriptor
int ipc_key;
int shm_id;
mapFile map;
void *mapPtr;
unsigned *numProcess;
//function declaration
void init(char *args[]);
void loadMap(char *fileName);
coordinate findStartPoint(void); 
unsigned coordinate2offset(coordinate &pos);
void printMap();
void printMsg(pid_t pid, coordinate &pos, msgType type);
unsigned int findWays(const coordinate &location, bool isWayPassable[4]);
direction findOneWay(bool isWayPassable[4]);
void walk(coordinate &location, direction &d);
bool isValidPos(const coordinate &location);

int main(int argv, char *args[])
{
    //local process data
    pid_t childPid = 0;
    coordinate location;
    direction walkDirection = up;
    bool isWayPassable[4];
    vector<pid_t> childrenPid;
    //first process
    init(args);
    printMap();
    location = map.sPoint;
    printMsg(firstPid, map.sPoint, childStatus);
    
Loop:
    //other process
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
           unsigned numWay = findWays(location, isWayPassable);
           if(numWay == 0)
           {
               printMap();
               if( map.src[coordinate2offset(location)] == 'K'){
                    printMsg(getpid(), location, found);
                    return true;
               }
               else{
                    printMsg(getpid(), location, none);
                    return false;
               }
           }
           else if(numWay == 1)
           {
                walkDirection = findOneWay(isWayPassable);
                walk(location, walkDirection);
           }
           else
           {
               childrenPid.clear();
               if(isWayPassable[up]){
                    *numProcess += 1;
                    walkDirection = up;
                    childPid = 0;
                    childPid = fork();      
                    if(childPid == 0){
                        walk(location, walkDirection);
                        goto Loop;
                    }
                    else{
                        printMsg(childPid, location, childStatus);
                        childrenPid.push_back(childPid);
                    }
               }
               if(isWayPassable[down]){
                    *numProcess += 1;
                    walkDirection = down;
                    childPid = 0;
                    childPid = fork();      
                    if(childPid == 0){
                        walk(location, walkDirection);
                        goto Loop;
                    }
                    else{
                        printMsg(childPid, location, childStatus);
                        childrenPid.push_back(childPid);
                    } 
               }
               if(isWayPassable[leftDir]){
                    *numProcess += 1;
                    walkDirection = leftDir;
                    childPid = 0;
                    childPid = fork();      
                    if(childPid == 0){
                        walk(location, walkDirection);
                        goto Loop;
                    }
                    else{
                        printMsg(childPid, location, childStatus);
                        childrenPid.push_back(childPid);
                    }
               }
               if(isWayPassable[rightDir]){
                   *numProcess += 1;
                   walkDirection = rightDir;
                   childPid = 0;
                   childPid = fork();      
                   if(childPid == 0){
                       walk(location, walkDirection);
                       goto Loop;
                   }
                   else{
                       printMsg(childPid, location, childStatus);
                       childrenPid.push_back(childPid);
                   }
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
        bool isFound = false;
        int rValue = 0;
        for(int i = 0; i < childrenPid.size(); ++i) {
            waitpid(childrenPid[i], &rValue, 0);
            if(WEXITSTATUS(rValue) == true) {
                isFound = true;
                break;
            }
        }
        cout << endl;
        //print isfound message
        if(isFound){
            printMsg(getpid(), location, found);
            return true;
        }
        else{
            printMsg(getpid(), location, none);
            return false;
        }
    }

    if(getpid() == firstPid) {
        cout << "pid=" << getpid() << " "<< "Number of Processes: " << *numProcess << endl;
        printMap();
        cout << "Search mineral done!" << endl;
        close(shm_fd);
    }
    delete numProcess;
    return 0;
}

void init(char *args[])
{
    numProcess = new unsigned;
    *numProcess = 1;
    loadMap(args[1]);
    firstPid = getpid();
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
    const size_t dataSize = numLine * 20;
    shm_fd = shm_open(shareMemName, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, dataSize);
    ipc_key = ftok(shareMemName, 'R');
    mapPtr = mmap(0, dataSize, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    shm_id = shmget(ipc_key, dataSize, IPC_EXCL);
    shmctl(shm_id, IPC_RMID, 0);
    //write data to share memory
    for(int i = 0; i < mapData.size(); ++i)
        ((char *)mapPtr)[i] = mapData[i];
    
    map.src = ((char *)mapPtr);
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

void printMsg(pid_t pid, coordinate &pos, msgType type)
{
    switch(type)
    {
        case childStatus:
            cout << "[pid=" << pid << "]: (" << pos.row << "," << pos.col << ")" << endl;
            break;
        case found:
            cout << pid << " (" << pos.row << "," << pos.col << ")" << "Found!" << endl;
            break;
        case none:
            cout << pid << " (" << pos.row << "," << pos.col << ")" << "None!" << endl;
            break;
        default:
            cout << "Error invalid msgType" << endl;
            break;
    }
}

unsigned int findWays(const coordinate &location, bool isWayPassable[4])
{
    unsigned numWay = 0;
    isWayPassable[0] = isWayPassable[1] = isWayPassable[2] = isWayPassable[3] = false;
    coordinate w, s, a, d;
    w = s = a = d = location;
    w.row -= 1; //up
    s.row += 1; //down
    a.col -= 1; //left
    d.col += 1; //right

    char wChar, sChar, aChar,dChar;
    wChar = sChar = aChar = dChar = 'X';
    if(isValidPos(w))
        wChar = map.src[coordinate2offset(w)]; //up character
    if(isValidPos(s))
        sChar = map.src[coordinate2offset(s)]; //down character
    if(isValidPos(a))
        aChar = map.src[coordinate2offset(a)]; //left character
    if(isValidPos(d))
        dChar = map.src[coordinate2offset(d)]; //right character

    if(wChar == ' ' || wChar == 'K'){
        ++numWay;
        isWayPassable[0] = true;
    }
    if(sChar == ' ' || sChar == 'K'){
        ++numWay;
        isWayPassable[1] = true;
    }
    if(aChar == ' ' || aChar == 'K'){
        ++numWay;
        isWayPassable[2] = true;
    }
    if(dChar == ' ' || dChar == 'K'){
        ++numWay;
        isWayPassable[3] = true;
    }

    return numWay;
}

direction findOneWay(bool isWayPassable[4])
{
    if(isWayPassable[0])
        return up;
    if(isWayPassable[1])
        return down;
    if(isWayPassable[2])
        return leftDir;
    if(isWayPassable[3])
        return rightDir;
}

void walk(coordinate &location, direction &d)
{
    unsigned int offset = coordinate2offset(location);
    switch(d)
    {
        case up:
            location.row -= 1;
            if(map.src[offset] != 'K')
                map.src[offset] = '-';
            break;
        case down:
            location.row += 1;
            if(map.src[offset] != 'K')
                map.src[offset] = '-';
            break;
        case leftDir:
            location.col -=1;
            if(map.src[offset] != 'K')
                map.src[offset] = '-';
            break;
        case rightDir:
            location.col += 1;
            if(map.src[offset] != 'K')
                map.src[offset] = '-';
            break;
    }
}

bool isValidPos(const coordinate &location)
{
    if(location.col > 19 || location.col < 0)
        return false;
    if(location.row >= map.numLine || location.row < 0)
        return false;
    //valid direction
    return true;
}
