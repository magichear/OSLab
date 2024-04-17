#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<sys/wait.h>
//#include<sys/time.h>
#include "/home/zxy/oslab/myconfig/oslab2.h"

/*Pre-declare func name*/
int Check(int argc, char *argv[]);
void print_info(int symbol_table[], double rate[], struct my_processinfo info[], int refresh_times, int process_cnt);
void cs(struct my_processinfo info[], int number[], double rate[]);

/*main*/
int main(int argc, char *argv[]) {
	// Init
	int seconds = 1;
	int output_number[20];		// Symbol table
	int fork_pid = 0;
	int refresh_times = 0;
	int process_cnt = 0;
	double rate[20];		// CPU rate
	struct my_processinfo info[1000];
	
	// Set seconds
	if ((seconds = Check(argc, argv)) <= 0) {
		printf("[ERROR]: Wrrong numbers of parameters!\n\n");
		exit(1);
	}
	
	/* Use child fork to support -d parameter*/
	while(1) {	// No need to quit like top rightnow
		system("clear");
		
		fork_pid = fork();
		if (fork_pid < 0){
			printf("[Fatal!] Fork error!\n");
			exit(1);
		}
		else if (fork_pid == 0) {
			sleep(seconds);  // Wait -d seconds
			exit(0);
		}
		else {
			// System call
			syscall(333, &info, 0);
			syscall(332, &process_cnt);
			
			// Calculate & Sort CPU occupancy
			cs(info,output_number,rate);
			
			// Print process infomation
			print_info(output_number,rate,info,refresh_times, process_cnt);
				
			// Wait for the time to end
			wait(NULL);				
		}
		refresh_times++;

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
			if ((double)seconds != atof(argv[2]))
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

void cs(struct my_processinfo info[], int number[], double rate[]) {
	// Init
	int i = 0;
	/*used to sort*/
	int j = 0;
	double max = 0;
	double prev_max = 100;
	int point = 0;
	
	// Calculate
	for(i = 0; i <20; i++){
		rate[i] = (double)(info[i].process_runtime) / info[i].process_startime;
	}
	
	// Sort
	for (i = 0; i<20 ; i++){
		for(j = 0; j<20 ; j++) {
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
	for(int i = 0; i < 20; i++){
		printf("%-3d %-16s %-9ld %-6.2f %-16.2f   --[%d]\n",info[symbol_table[i]].pid, info[symbol_table[i]].comm, info[symbol_table[i]].state, rate[symbol_table[i]],  (double)((info[symbol_table[i]].process_runtime)/1000000000.0),i+1);
	}
}








