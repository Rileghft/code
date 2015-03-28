#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <string>

using namespace::std;

//Data Structure
struct mapFile{
    const char *src;
    const int numLine;
};

//function declaration
//void init(void);
mapFile loadMap(char *fileName);
void printMap(const char *mapSrc, int numLine);

int main(int argv, char *args[])
{
    //cross processes data
    int numProcess = 0;
    mapFile map = loadMap(args[1]);
    //local process data
    pid_t childPid = 0;
    printMap(map.src , map.numLine);
    delete [] map.src;
    return 0;
}

mapFile loadMap(char *fileName)
{
    string row;
    string mapData;
    int numLine = 0;
    ifstream fin(fileName, ios::in);
    while(getline(fin, row, '\n')){
        mapData += row;
        ++numLine;
    }
    cout << "number of characters: " << mapData.size() << endl;
    fin.close();

    char *mapPtr = new char[mapData.size()];
    for(int i = 0; i < mapData.size(); ++i)
        mapPtr[i] = mapData[i];

    mapFile map = {
        mapPtr,
        numLine
    };

    return map;
}

void printMap(const  char *mapData, int numLine)
{
    for(int row = 0; row < numLine; ++row) {
        for(int col = 0; col < 20; ++col)
            cout << mapData[row * 20 + col];
        cout << endl;
    }
}
