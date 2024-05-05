
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>      // 修改输出小数点格式
#include "mm.h"
#include "memlib.h"
#include "config.h"
#include "zipf.hpp"

#define MAX_ITEMS 50000
#define LOOP_NUM 20
#define SEED 10000
#define WORKLOAD_TYPE 16
#define malloc mm_malloc
#define free mm_free
unsigned int workload_size[WORKLOAD_TYPE] = {12, 16, 24, 32, 48, 64, 96, 100, 128, 192, 256, 384 , 500, 512, 768 , 1024};

long malloc_time_cnt;

extern size_t user_malloc_size ;
extern size_t heap_size ;

/*A simplified workload storage index*/
struct workload_base{
    void** addr;
};

/*Generation of string with length*/
char* gen_random_string(int length)
{
	int flag, i;
	char* string;
    struct timeval curtime;
    gettimeofday(&curtime, NULL);
    long sec1 = curtime.tv_sec, usec1 = curtime.tv_usec;
    if ((string = (char*) malloc(length)) == NULL )
	{
		std::cerr << "Malloc failed at genRandomString!" << std::endl;
		return NULL ;
	}
    gettimeofday(&curtime, NULL);
    long sec2 = curtime.tv_sec, usec2 = curtime.tv_usec;

    malloc_time_cnt += (sec2-sec1)*1000 + (usec2-usec1)/1000;

    for (i = 0; i < length - 1; i++)
	{
		flag = rand() % 3;
		switch (flag)
		{
			case 0:
				string[i] = 'A' + rand() % 26;
				break;
			case 1:
				string[i] = 'a' + rand() % 26;
				break;
			case 2:
				string[i] = '0' + rand() % 10;
				break;
			default:
				string[i] = 'x' ;
				break;
		}
	}
	string[length - 1] = '\0' ;
	return string;
}

/* Create the workload index */
int workload_create(struct workload_base* workload){
    srand(SEED);
    mem_init();
    if (mm_init() < 0)
	{
		fprintf(stderr, "mm_init failed.\n");
		return 0;
	}
    workload->addr = (void**)malloc(sizeof(void*)*MAX_ITEMS);
    memset(workload->addr, 0, sizeof(void*)*MAX_ITEMS);
    return 0;
}

/* Insert strings up to 100% of MAX_ITEMS */
int workload_insert(struct workload_base *workload){
    unsigned int size, total=0;
    for(int i=0;i<MAX_ITEMS;i++){
        if(workload->addr[i] == 0){
            size= workload_size[rand()%WORKLOAD_TYPE];
            workload->addr[i] = gen_random_string(size);
            total += size;
        }
    }
    return 0;
}

/* Sort strings */
int workload_swap(struct workload_base *workload){
    for(int i=1;i<MAX_ITEMS;i++){
        void *temp;
        temp = workload->addr[i];
        workload->addr[i] = workload->addr[i-1];
        workload->addr[i-1] = temp;
    }
    return 0;
}

/* Read strings, at a zipfian distribution */
int workload_read(struct workload_base *workload){
    char reader[1025];
    zipf_distribution<int,double> zipf(MAX_ITEMS-1, 0.99);
    std::mt19937 generator2(SEED);
    for(int i=0;i<MAX_ITEMS*10;i++){
        strcpy(reader, (char*)workload->addr[zipf(generator2)]);
    }
    return 0;
}

/* Randomly delete 80% of strings */
int workload_delete(struct workload_base *workload){
    for(int i=0;i<MAX_ITEMS;i++){
        if(rand()%5!=0){
            free(workload->addr[i]);
            workload->addr[i]=0;
        }
    }
    return 0;
}

/* Run workload */
void* workload_run(void *workload){
    struct timeval cur_time;
    for(int loop=0; loop<LOOP_NUM; loop++){
        gettimeofday(&cur_time, NULL);
        long sec1=cur_time.tv_sec,usec1=cur_time.tv_usec;
        workload_insert((struct workload_base*)workload);
        workload_swap((struct workload_base*)workload);
        workload_read((struct workload_base*)workload);
        std::cout<< "\n------------------------------------\n" <<std::endl;
        std::cout << "before free: " << std::fixed << std::setprecision(3) << get_utilization();    // 输出三位小数
        workload_delete((struct workload_base *)workload);
        std::cout << "; after free: " << std::fixed << std::setprecision(3) << get_utilization() << std::endl;
        gettimeofday(&cur_time, NULL);
        long sec2=cur_time.tv_sec,usec2=cur_time.tv_usec;
        std::cout<<"    time of loop "<< loop <<" : "<<(sec2-sec1)*1000 + (usec2-usec1)/1000 << "ms" << std::endl;        
    }
    std::cout << "\n------------------------------------\nMalloc average use: "<<((double)malloc_time_cnt/(LOOP_NUM+1))<<" ms"<< std::endl;
    return NULL;
}

/* Run monitor */
void* monitor_run(void *argv){
    double util;
    std::ofstream fout;
    fout.open("./mem_util.csv", std::ios::out);
    long timer=0;
    fout<<"time\tutil"<<std::endl;
    while(1){
        util=get_utilization();
        fout<<timer<<"\t"<<util<<std::endl;
        timer++;
        sleep(1);
    }
    fout.close();
}

int main(){
    int error;
    struct workload_base workload;
    if(error = workload_create(&workload)){
        std::cerr << "workload creat error:" << error << std::endl;    
    }         
    pthread_t monitor_pid; 
    // pthread_create(&monitor_pid, NULL, monitor_run, NULL);
    malloc_time_cnt = 0;
    workload_run(&workload);
    // pthread_cancel(monitor_pid);
    return 0;
}

