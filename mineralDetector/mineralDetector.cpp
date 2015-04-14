#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
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
struct report{
    pid_t pid[4];
    clock_t cputime[4];
    coordinate location[4];
    unsigned numFound[4];
    msgType msg[4];
};

//global variable
const char *MapName = "/shareMap";
const char *NumProcessName = "/numProcess";
char *reportName;
mapFile map;
unsigned *numProcess;
pid_t firstPid;
pid_t childPid;
clock_t CPUtime;
int shm_fd;
report *upReport = NULL, *downReport = NULL;
coordinate location;
direction sourceDirection;
direction walkDirection;
bool isWayPassable[4];
//function declaration
void init(char *args[]);
void loadMap(char *fileName);
coordinate findStartPoint(void); 
unsigned coordinate2offset(coordinate &pos);
void printMap();
void printMsg(pid_t pid, coordinate &location, msgType type);
unsigned int findWays(); 
bool isValidPos(const coordinate &location);
direction findOneWay();
void walk();
bool search(const direction &dir);
int *shmFactory(const char *name, size_t spaceSize);
const char *randName();
void printCPUtime(clock_t t);

int main(int argv, char *args[])
{
    //local variable
    bool isChild;
    char *underFootChar;
    //first process
    clock_t cputime_start = clock();
    init(args);
    printMap();
    printMsg(firstPid, location, childStatus);
    
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
           underFootChar = &map.src[coordinate2offset(location)];
           if(numWay == 0 || *underFootChar == 'K')
           {
               if(*numProcess == 1){
                   if(*underFootChar == 'K'){
                       printMsg(firstPid, location, found);
                       cout << 1 << '!' << endl;
                   }
                   else
                       printMsg(firstPid, location, none);
                   printMap();
                   printCPUtime(clock() - cputime_start);
                   return 0;
               }
               downReport->location[sourceDirection] = location;
               if( *underFootChar == 'K'){
                    downReport->msg[sourceDirection] = found;
                    downReport->numFound[sourceDirection] = 1;
               }
               else{
                    *underFootChar = 'X';
                    downReport->msg[sourceDirection] = none;
                    downReport->numFound[sourceDirection] = 0;
               }
               downReport->cputime[sourceDirection] = clock() - CPUtime;
               return 0;
           }
           else if(numWay == 1)
           {
                walkDirection = findOneWay();
                walk();
           }
           else
           {
               upReport = downReport;
               downReport = (report *)shmFactory(randName(), 88);
               isChild = false;
               if(*numProcess != 1)
                   cputime_start = CPUtime;
               if(*underFootChar != 'K' && *underFootChar != 'S')
                   *underFootChar = '#';
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
        clock_t child_cputime = 0;
        unsigned numFound = 0;
        msgType isFound = none;
        //check children report
        for(int i = 0; i < 4; ++i) {
            if(downReport->pid[i] != 0){
                waitpid(downReport->pid[i], NULL, 0);
                if(downReport->msg[i] == found){
                    numFound += downReport->numFound[i];
                    isFound = found;
                    printMsg(downReport->pid[i], downReport->location[i], downReport->msg[i]);
                    cout << downReport->numFound[i] << '!' << endl;
                }
                else
                    printMsg(downReport->pid[i], downReport->location[i], downReport->msg[i]);
                child_cputime += downReport->cputime[i];
            }
        }
        cout << "child cpu time: " << child_cputime << endl;
        //report
        if(getpid() == firstPid){
            printMsg(firstPid, location, isFound);
            if(isFound == found)
                cout << numFound << '!' << endl;
            printMap();
            cout << "numProcess: " <<  *numProcess << endl;
            close(shm_fd);
            shm_unlink(MapName);
            shm_unlink(reportName);
            shm_unlink(NumProcessName);
            printCPUtime(child_cputime + clock() - CPUtime);
            return 0;
        }
        upReport->location[sourceDirection] = location;
        upReport->numFound[sourceDirection] = numFound;
        if(isFound == found)
            upReport->msg[sourceDirection] = found;
        else
            upReport->msg[sourceDirection] = none;
        shm_unlink(reportName);
        upReport->cputime[sourceDirection] = child_cputime + clock() - CPUtime;
        return 0;
    }
}

void init(char *args[])
{
    numProcess = (unsigned *)shmFactory(NumProcessName, 4);
    *numProcess = 1;
    srand(time(0));
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

    //initialize map
    const size_t mapSize = numLine * 20;
    map.src = (char *)shmFactory(MapName, mapSize);
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

void printMsg(pid_t pid, coordinate &location, msgType type)
{
    switch(type)
    {
        case childStatus:
            cout << "[pid=" << pid << "]: (" << location.row << "," << location.col << ")" << endl;
            break;
        case found:
            cout << pid << " (" << location.row << "," << location.col << ")" << "Found ";
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
    char *underFootChar = &map.src[coordinate2offset(location)];
    if(*underFootChar != 'K')
        *underFootChar = '-';
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
    CPUtime = clock();
    if(childPid == 0){
        sourceDirection = dir;
        walk();
        return true;
    }
    else{
        printMsg(childPid, location, childStatus);
        downReport->pid[walkDirection] = childPid;
        return false;
    }
}

int *shmFactory(const char *name, size_t spaceSize)
{
    void *shmPtr;
    unsigned numFailed = 100;
    shm_fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666);
    while(shm_fd == -1){
        --numFailed;
        shm_fd = shm_open(randName(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if(!numFailed){
            cout << "Error, 100 new name cannot handle shm_open() failure!" << endl;
            exit(-1);
        }
    }
    if(ftruncate(shm_fd, spaceSize) == -1){
        cout << "Error, ftruncate failed" << endl;
        exit(-1);
    }
    shmPtr = mmap(0, spaceSize, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shmPtr == MAP_FAILED){
        cout << "Error, mmap failed" << endl;
        exit(-1);
    }

    return (int *)shmPtr;
}

const char *randName()
{
    char *name = new char[10];
    for(int i = 1; i < 9; ++i)
        name[i] = rand() % 208 + 48;
    name[0] = '/';
    name[9] = '\0';
    reportName = name;
    
    return (const char *)name;
}

void printCPUtime(clock_t t)
{
    printf("Total: %2f ms\n", (double)t);
}
