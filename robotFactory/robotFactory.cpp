#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <semaphore.h>

using namespace std;

//data structure
enum component {battery, sensor, wifi, crawler};
struct producerReport {
    unsigned producerID = 0;
    unsigned numMaked = 0;
};
//global variable
pthread_t mainTid;
bool mode = false;
bool availableComponent[4];
vector<producerReport> report;
unsigned compCount[2][4];
unsigned numFinishRobot = 0;
sem_t sem;
//function template
void *dispatcher(void *param);
bool compEmpty();
void *producer(void *ownComponent);
void dispatcherMsg(unsigned &notGetComp, unsigned &id);
void producerMsg(component *ownComp, unsigned &numMaked);
bool COMPARE(producerReport a, producerReport b);

int main(int argc, char *argv[])
{
    report.resize(4);
    //pthread attribute
    mainTid = pthread_self();
    pthread_t tid;
    pthread_attr_t attr;
    //semaphore initialization
    sem_init(&sem , 0, 1);
    //producer threads
    for(unsigned i = 0; i < 4; ++i) {
        component *producerComp = new component;
        *producerComp = (component)i;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, producer, (void *)producerComp);
    }
    //dispatcher threads
    string dispatcherName1 = "";
    string dispatcherName2 = " B";
    unsigned d1 = 0;
    unsigned d2 = 1;
    if(argc == 2)
        if(atoi(argv[1]) == 2) {
            mode = true;
            dispatcherName1 = " A"; 
            pthread_attr_init(&attr);
            pthread_create(&tid, &attr, dispatcher, (void *)&d2);
        }
    dispatcher((void*)&d1);
    sem_destroy(&sem);
    printf( "-----Summarize-----\n");
    printf("Dispatcher%s: %d batteries, %d sensors, %d wifis, %d crawlers\n", dispatcherName1.c_str(), compCount[0][0], compCount[0][1], compCount[0][2], compCount[0][3]);
    if(argc == 2)
        if(atoi(argv[1]) == 2)
            printf("Dispatcher%s: %d batteries, %d sensors, %d wifis, %d crawlers\n", dispatcherName2.c_str(), compCount[1][0], compCount[1][1], compCount[1][2], compCount[1][3]);

    //summary output
    sort(report.begin(), report.end(), COMPARE);
    for(producerReport &info: report)
        producerMsg((component *)&info.producerID, info.numMaked);
    return 0;
}

void *dispatcher(void *param)
{
    unsigned notGetComp = 0;
    unsigned id = *(unsigned *)param;
    for(unsigned i = 0; i < 4; ++i)
        compCount[id][i] = 0;
    while(numFinishRobot != 40) {
        sem_wait(&sem);
        if(numFinishRobot == 40)
            break;
        if(compEmpty()) {
            notGetComp = rand() % 4;
            for(unsigned i = 0; i < 4; ++i) {
                availableComponent[i] = 1;
                ++compCount[id][i];
            }
            availableComponent[notGetComp] = 0;
            --compCount[id][notGetComp];
            dispatcherMsg(notGetComp, id);
        }
        
        sem_post(&sem);
    }
    sem_post(&sem);
    if(pthread_self() == mainTid)
        return NULL;
    pthread_exit(NULL);
}

bool inline compEmpty()
{
    return !(availableComponent[0] | availableComponent[1] | availableComponent[2] | availableComponent[3]);
}

void *producer(void *ownComponent)
{
    unsigned ownComp = *((unsigned *)ownComponent);
    report[ownComp].producerID = ownComp;
    report[ownComp].numMaked = 0;
    while(true) {
        sem_wait(&sem);
        if(numFinishRobot == 40) {
            delete (component *)ownComponent;
            break;
        }
        if(!availableComponent[ownComp] && !compEmpty()) {
            availableComponent[0] = availableComponent[1] = availableComponent[2] = availableComponent[3] = 0;
            ++numFinishRobot;
            ++report[ownComp].numMaked;
            producerMsg((component *)ownComponent, numFinishRobot);
        }
        sem_post(&sem);
    }
    sem_post(&sem);
    pthread_exit(NULL);
}

void dispatcherMsg(unsigned &notGetComp, unsigned &id)
{
    bool compDistribute[4] = {1, 1, 1, 1};
    string name = ""; 
    if(mode)
        name = (id == 0)? " A" : " B";
    compDistribute[notGetComp] = 0;
    if(compDistribute[0])
        cout << "Dispatcher" << name << ": battery" << endl;
    if(compDistribute[1])
        cout << "Dispatcher" << name << ": sensor" << endl;
    if(compDistribute[2])
        cout << "Dispatcher" << name << ": wifi" << endl;
    if(compDistribute[3])
        cout << "Dispatcher" << name << ": crawler" << endl;
}

void producerMsg(component *ownComp, unsigned &numMaked)
{
    string ownCompName;
    switch(*ownComp)
    {
        case battery:
            ownCompName = "(battery)";
            break;
        case sensor:
            ownCompName = "(sensor)";
            break;
        case wifi:
            ownCompName = "(wifi)";
            break;
        case crawler:
            ownCompName = "(crawler)";
            break;
    };    
    if(mainTid == pthread_self())
        printf("Producer %s: make %d robot(s)\n", ownCompName.c_str(), numMaked);
    else
        printf("Producer %s: OK, %d robot(s)\n", ownCompName.c_str(), numMaked);
}

bool COMPARE(producerReport a, producerReport b)
{
    return (a.numMaked > b.numMaked);
}
