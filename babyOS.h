// FILE: babyOS.h
// A Bautista, B Franco, E Mora
// OS, Fall 2023, Transy U
//
//
// 
//

#ifndef BABY_H
#define BABY_H

#include <string>
#include <limits.h>
#include <queue>

using namespace std;

#define INPUT_MAX CHAR_MAX
#define MAX_FRAMES 65536
#define PID_FORM "P_"
#define MIN_PRIORITY 0
#define MAX_PRIORITY 100

#define PAGER_TYPE "--pagerType"
#define FRAMES "--frames"
#define FRAME_SIZE "--framesize"
#define PAGES "--pages"
#define FIRST_IN_FIRST_OUT "FIFO"
#define LEAST_RECENT_USED "LRU"
#define MOST_FREQUENT_USED "MFU"
#define RANDOM "RANDOM"

#define SCHEDULER_TYPE "--schedulerType"
#define PREEMPTIVE "--preemptive"
#define QUANTA "--quanta"
#define VERBOSE "--verbose"
#define FIRST_COME_FIRST_SERVE "FCFS"
#define SHORTEST_JOB_FIRST "SJF"
#define PRIORITY "Priority"
#define ROUND_ROBIN "RR"

#define DEFAULT_SCHEDULER_TYPE FIRST_IN_FIRST_OUT
#define DEFAULT_PAGER_TYPE FIRST_COME_FIRST_SERVE
#define DEFAULT_FRAMES "3"
#define DEFAULT_PAGES "8"
#define DEFAULT_SIZE "512"
#define DEFAULT_QUANTA "10"
#define DEFAULT_FILE "final.in"

enum{
	PREEMPTIVE_FLAG,
	QUANTA_FLAG,
	VERBOSE_FLAG,
	MAX_FLAGS
};

struct PCB{
	int pid, arrival, burst, priority;
	queue<int> addresses;
};

// checks if an address is an integer and if it can exist in the number of pages of size pageSize
// the pid is taken in to print the pid where the error was found
// returns false if no error were found
bool addressErrorCheck(int pid,int pageSize, int pages, string address);

// checks if arrival, burst, and priority are non-negative integers and if priority is 0-100
// the pid is taken in to print the pid where the error was found
// returns false if no error were found
bool loadErrorCheck(int pid, string arrival, string burst, string priority);

// checks if the given integerString is indeed an integer, returns true if it is an integer
bool integerCheck(string integerString);

// ensures that entered input options don't conflict
// checks if pages, frames, frameSize, and quanta are posotive integers
// frameSize at most can be MAX_FRAMES
// returns false if no error were found
bool inputErrorCheck(string pages, string frames, string frameSize, string schedulerType, string quanta, bool *flags);

// checks if all options that require inputs have inputs, if there are any invalid options, and if entered types are valid
// returns -1 if an error was found
// returns 0 if no errors were found and no file name was specified
// returns the index of the file name in argv if no errors were found and the file name was specified
// file name can currently only be at the last index of argv
int commandErrorCheck(int argc, char** argv);

// checks if the process id is valid and in the proper format, returns false if no errors were found
bool idErrorCheck(string fileInput);

#endif // BABY_H