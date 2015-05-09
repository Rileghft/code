#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

#define gettid() syscall(__NR_gettid)
int sum; //share data
void *runner(void *param);

using namespace std;

int main(int argc, char *argv[])
{
    pthread_t tid;          //thread id
    pthread_attr_t attr;    //set of thread attributes

    if(argc != 2) {
        cout << "usage: a.out <integer value>\n";
        return -1; 
    }
    if(atoi(argv[1]) < 0) {
        cout << argv[1] << " must be >= 0\n";
        return -1;
    }

    //初始化thread參數
    pthread_attr_init(&attr);
    //建立thread
    pthread_create(&tid, &attr, runner, argv[1]);
    //等待thread離開
    pthread_join(tid, NULL);

    cout << "sum = " << sum << endl;
}

//thread執行函式
void *runner(void *param)
{
    int i, upper = atoi((const char *)param);
    sum = 0;
    for(i = 1; i <= upper; i++) 
        sum += i;
    cout << "Thread ID: " << (long int)gettid() << endl; 
    pthread_exit(0);
}
