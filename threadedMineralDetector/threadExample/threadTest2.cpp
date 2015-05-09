#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;

#define gettid() syscall(__NR_gettid)
struct info{
    unsigned long tid;
    unsigned long n;
    unsigned long num;
};

void *factorial(void *param);

int main()
{
    unsigned long n = 0;
    unsigned long num = 0;
    cout << "請輸入一個階層大小n!(n < 22): ";
    cin >> n; 

    pthread_t tid;
    pthread_attr_t attr;
    info param = {gettid(), n, num};
    info *rValue;

    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, factorial, (void *)&param);
    pthread_join(tid, (void **)&rValue);
    info *rData = (info *)rValue;
    cout << n << "! = " <<  rData->num << endl;
    delete rValue;

    return 0;
}

void *factorial(void *param)
{
    info *paramPointer = (info *)param;
    unsigned long tid = paramPointer->tid;
    unsigned long n = paramPointer->n;
    unsigned long num = paramPointer->num; 
    
    if(n == 0) {
        info *rValue = new info;
        rValue->tid = gettid();
        rValue->n = 0;
        rValue->num = 1;
        pthread_exit(rValue);
    }
    else {
        pthread_t tid;
        pthread_attr_t attr;
        info *param = new info;
        param->tid = gettid();
        param->n = n - 1;
        param->num = num;
        info *report;
        info *rValue = new info;

        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, factorial, (void *)param);
        pthread_join(tid, (void **)&report);

        rValue->tid = gettid();
        rValue->n = n;
        rValue->num = report->num * n;

        delete report;
        delete param;
        pthread_exit(rValue);
    }
}
