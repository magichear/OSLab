#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<sys/wait.h>
#include <sys/mman.h>	// 共享内存
//#include<sys/time.h>
#include "/home/zxy/oslab/myconfig/oslab2.h"

#define SIZE 360
#define EACH_SIZE 30
#define PRINT_SIZE 20

/*Pre-declare func name*/
int Check(int argc, char *argv[]);
int get_processes_data(struct my_processinfo *info, int *ps_cnt);
void cs(struct my_processinfo info[], int number[], double rate[], int total);
void print_info(int symbol_table[], double rate[], struct my_processinfo info[], int refresh_times, int process_cnt);

/*main*/
int main(int argc, char *argv[]) {
	// Init
	/* Parameter */
	int seconds = 1;
	/* PID */
	int fork_pid = -1;
	int pidA = -1;
	int pidB = -1;
	int pidC = -1;
	/* Sort assist*/
	double rate[SIZE];		// CPU rate
	int output_number[PRINT_SIZE];	// Symbol table
	/* Loop assist */
	int process_cnt = 0;
	int loop_cnt = 0;		// 循环读取进程的次数
	int refresh_times = 0;
	struct my_processinfo *info = mmap(NULL, SIZE * sizeof(struct my_processinfo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);	

	// Set seconds
	if ((seconds = Check(argc, argv)) <= 0) {
		printf("[ERROR]: Wrrong numbers of parameters!\n\n");
		exit(1);
	}
	
	/* Use child fork to support -d parameter*/
	while(1) {	// No need to quit like top rightnow
		system("clear");

		if (info == MAP_FAILED) {
    			perror("mmap");
    			exit(1);
    		}		
		fork_pid = fork();
		if (fork_pid < 0){
			printf("[Fatal!] Fork error!\n");
			exit(1);
		}
		else if (fork_pid == 0) {
			sleep(seconds);  // Wait -d seconds
			exit(255);
		}
		/* 
		   关键路径为取数据与排序阶段，
		   若关键路径快了，会等待休眠进程结束再输出
		   若休眠进程先结束，也得等待关键路径结束才能输出
		 */
		else {		           // 多进程获取进程数据
			get_processes_data(info, &process_cnt);
			// Calculate & Sort CPU occupancy
			cs(info,output_number,rate, process_cnt);
			// Print process infomation
			print_info(output_number,rate,info,refresh_times, process_cnt);			
		}
		// 等待倒计时子进程结束
		while(wait(NULL) > 0);
		refresh_times++;
	}
		// 释放共享内存
		if (munmap(info, SIZE * sizeof(struct my_processinfo))){
    			perror("munmap");
    			exit(1);
		}		
	return 0;
}
/*Other func*/
int Check(int argc, char *argv[]){
	// Init
	int seconds = 0;
	
	// Default situation
	if (argc == 1) {
		seconds = 1;
	}
	// Individualized parameter -d
	else if (argc == 3) {
		if ((argv[1][0] == '-') && (argv[1][1] == 'd') && (argv[1][2] == '\0')) {
			seconds = atoi(argv[2]);
			if ((double)seconds != atof(argv[2]))  	// 可以排除小数输入
				seconds = 0;
		}
		else {
			seconds = 0;
		}
	}
	// Check the correctness of parameters
	else {
		seconds =  0;
	}	
	return seconds;
}

int get_processes_data(struct my_processinfo *info, int *ps_cnt) {
	// PID init
	int pidA = -1;
	/* Loop assist */
	int process_cnt = 0;            // 进程总数
	int loop_cnt = 0;		// 循环读取进程的次数
	int total_cnt = 0;		// 总共需要循环的次数，与process_cnt有关

	int *child_type = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	int type_return = 0;

	if (child_type == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
	}

    syscall(332, &process_cnt);			// 计算进程总数量
    *ps_cnt = process_cnt;
    if (process_cnt > SIZE){
        printf("\n进程数过多\n");
        printf("可以多开进程处理，但本实验没必要\n");
        printf("建议重启虚拟机以减少进程数量\n");
        system("pause");
        while(wait(NULL) > 0);	// 会等待所有的子进程结束（包括main函数中）
        exit(1);
    }
    total_cnt = process_cnt/EACH_SIZE + 1;		// 计算需要循环的次数
    while(loop_cnt < total_cnt) {
        // 子进程
        pidA = fork();
        if (pidA < 0){
            printf("[Fatal!] Fork error!\n");
	    *child_type = 1;
            exit(1);
        }
        else if (pidA == 0) {
            // 设置起始地址（内存共享）		
            struct my_processinfo *child_infoA = info + loop_cnt * EACH_SIZE;

            // 系统调用
            if(syscall(333, child_infoA, loop_cnt)){
                printf("[Fatal!] Syscall333 error!\n");
		*child_type = 1;
            }
            // 退出
            exit(255);
        }
        loop_cnt++;
        waitpid(pidA, NULL, 0);
    }	// while END
	type_return = *child_type;
	if (munmap(child_type, sizeof(int)) == -1) {
    		perror("munmap");
    		exit(1);
	}
	return type_return;
}


void cs(struct my_processinfo info[], int number[], double rate[], int total) {
	// Init
	int i = 0;
	/*used to sort*/
	int j = 0;
	double max = 0;
	int point = 0;
	
	// Calculate
	/* 只有total个进程 */
	for(i = 0; i <total; i++){
		rate[i] = (double)(info[i].process_runtime) / info[i].process_startime;
	}
	
	// Sort
	/* 每轮找到一个最大的并标记，将其位置放入number数组中 */
	for (i = 0; i<PRINT_SIZE; i++){		    // 只输出最大的二十个
		for(j = 0; j<total; j++) {  // 每轮找到一个最大的
			if ((info[j].sorted == 0) && (rate[j] >= max)){
				point = j;
				max = rate[j];
			}
		}
		number[i] = point;
		info[point].sorted = 1;
		max = 0;
	}
}

void print_info(int symbol_table[], double rate[], struct my_processinfo info[], int refresh_times, int process_cnt){
	printf("PID COMM             ISRUNNING %%CPU   TIME            %d process | %dth refresh\n",process_cnt, refresh_times);
	for(int i = 0; i < PRINT_SIZE; i++){
		printf("%-4d %-16s %-9ld %-6.2f %-16.2f   --[%d]\n",info[symbol_table[i]].pid, info[symbol_table[i]].comm, info[symbol_table[i]].state, rate[symbol_table[i]],  (double)((info[symbol_table[i]].process_runtime)/1000000000.0),i+1);
	}
}








