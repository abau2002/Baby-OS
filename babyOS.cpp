// FILE: babyOS.cpp
// A Bautista, B Franco, E Mora
// OS, Fall 2023, Transy U
//
//	driver for babyOS that handles error checking for the command line and input file
//		if both are valid then it will proceed to attempt to schedule and then page the processes
//  	in the order that they appear in the input file using the indicated algorithms
//
// http://stackoverflow.com/questions/5590381/ddg#5591169
// https://cplusplus.com/reference/vector/vector/
//
//	babyOS is an implementation of the G5Pager and G4CPU in order to simulate
//		the basic resource management of an OS
//
//	To use babyOS	enter at the command line --pagerType {pagerType}, --frames #, --framesize #, --pages #,
//		--schedulerType {schedulerType}, --preemptive, --quanta #, and --verbose to specify options, otherwise the defaults will be used
//		to specify an input file, type the file name as the last argument, otherwise the default will be used
//	ex. ./babyOS --schedulerType SJF --pagerType LRU --preemptive --pages 10 --verbose file.txt
//	Defaults: schedulerType - FCFS, pagerType - FIFO, frames - 3, pages - 8, framesize - 512, quanta - 10, 
//		preemptive - false, verbose - false, file - final.in
//
// The CPU scheduler we recieved would eventually core dump on all algorithms except for FCFS and RR
// 	as such we believed it to be safer to comment out those function calls and print out that those schedulers are unsafe and that FCFS should be used
// RR was fully commented out and so we print out a statement saying that it has yet to be implemented 
// FCFS runs fully without core dumping, however it nearly always returns a wrong answer for average wait time
// 	Since FCFS doesn't core dump we decided to leave it in and print out a message that warns that the average wait time may be incorrect
// 	Also to use FCFS, since it takes in a file name, we had to write the processes without their addresses to a new file and pass it in
// 	Thankfully, we knew the formatting of a file for the CPU scheduler since we worked on one as well
//	With this solution, we have to have an additional error check for if the specified file at the command line matches the one defined by CPU_FILE
//	If it does, we print out a message that that file name is reserved for the babyOS's G4CPU
// For SJF and Priority we did not intend on using the CPU Scheduler function putQueue since there were no comments on it
// 	and its absence seemed to have no effect on the runnability of the scheduler, with or without the function it core dumps
//	When we tested it, we figured that the count could be the number of processes since it'd print out a process block as many times as the count
//	Even passing in a count with this in mind, it did not prevent a core dump
// We cannot have the pager page the processes in the order they were scheduled because only one scheduler works and for the one that does
//	work there is no way we can access the scheduled process order. Additionally, we believed it would go against the idea of trying to integrate
//	G4CPU as we'd be writing our own scheduler and doing their job for them
// 
// The pager we recieved was fully operational
// We did not include symConsts.h in our driver because we wanted all the definitions that it needs
// 	to be in one place: the header associated with the driver
// We also changed the option for the random pagerType to be "Random" instead of "RANDOM" to be in line with how it is defined
//	in symConsts.h so that the random pager would work
// We decided that in our for loop that calls the pager function that we would have PageTable and PageReplacementSimulator
//	be redeclared with each iteration of the loop in order to reset pageFaultCount and the pageTable for the next process.
//	If we don't redeclare the PageTable then the pager will core dump with multiple processes and if we don't redeclare
//	PageReplacementSimulator then each processes's total will actually be a running total
//	ex. P1 faults = 5, P2 faults = 14, P2 itself only had 9 faults, 14 is the running total
//	we think this is likely because the creators of the pager dealt with only one process and so felt no need
//	to reset the pageTable or the pagefaultCount
// We think it'd be better to have the individual totals be correct with no running total than incorrect with a running total 
// We can't get the cumulative total of page faults as we cannot access pageFaultCount since it is a private variable
// We didn't use updateFreeFrame or updateVictimFrame because it was clear that they were meant for use by the pager itself and not to be
//	called explicitly by the driver
//
// The files our group specifically made were: babyOS.cpp, babyOS.h, and the makefile. All others were made by the other two groups.
//

//#include "symConsts.h"
#include "FCFS.h"
#include "pageReplacementSimulator.h"
#include "pageTable.h"
#include "Priority.h"
#include "procManagement.h" 
#include "RoundRobin.h"
#include "SJF.h"

#include "babyOS.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdlib.h>
#include <vector>

using namespace std;

int main(int argc, char **argv){
  bool flags[MAX_FLAGS];
  char pagerType[INPUT_MAX], frames[INPUT_MAX], pages[INPUT_MAX], frameSize[INPUT_MAX];
  char schedulerType[INPUT_MAX], quanta[INPUT_MAX], fileName[INPUT_MAX];
  vector<PCB> processes;
  queue<PCB>  addressQueue;
  int fileIndex;
  ifstream inputFile;
  FCFS fcfs;
  // Priority pri;
  // SJF sjf;
  
  // ensures a default is present in case a value is not specified
  strcpy(pagerType,DEFAULT_PAGER_TYPE);
  strcpy(schedulerType,DEFAULT_SCHEDULER_TYPE);
  strcpy(frames,DEFAULT_FRAMES);
  strcpy(frameSize,DEFAULT_SIZE);
  strcpy(pages,DEFAULT_PAGES);
  strcpy(quanta,DEFAULT_QUANTA);
  strcpy(fileName,DEFAULT_FILE);

  for(int i=0;i<MAX_FLAGS;i++) flags[i]=false;
  
  // if fileIndex is -1, an error has occurred
  // if fileIndex is 0 then the default will be used
  // otherwise the file has been specified and will be used
  fileIndex = commandErrorCheck(argc, argv);
  if(fileIndex == -1){
    exit(1);
  }
  else if(fileIndex){
    strcpy(fileName,argv[fileIndex]);
  }
  if(!strcmp(fileName,CPU_FILE)){
    cout << "\tApologies, this file name is reserved for babyOS's CPU Scheduler. Please use a different one.\n";
    exit(1);
  }
  
  // counts of how many times each option is entered and index of a repeat
  int repeatIndex = 0;
  int pagerTypeOptions=0,pageOptions=0,frameOptions=0,sizeOptions=0;
  int schedulerTypeOptions=0,preemptiveOptions=0,quantaOptions=0,verboseOptions=0;
  
  // stores the value of each specified argument so that it can be used in page replacement and cpu scheduling
  // if any repeats are found, then it will gracefully exit
  for(int i=1;i<argc;i++){
    if(!strcmp(argv[i],PAGER_TYPE)){
      pagerTypeOptions>=1 ? repeatIndex=i : pagerTypeOptions++;
      strcpy(pagerType,argv[i+1]);
    }
    else if(!strcmp(argv[i],PAGES)){
      pageOptions>=1 ? repeatIndex=i : pageOptions++;
      strcpy(pages,argv[i+1]);
    }
    else if(!strcmp(argv[i],FRAMES)){
      frameOptions>=1 ? repeatIndex=i : frameOptions++;  
      strcpy(frames,argv[i+1]);             
    }
    else if(!strcmp(argv[i],FRAME_SIZE)){
      sizeOptions>=1 ? repeatIndex=i : sizeOptions++;
      strcpy(frameSize,argv[i+1]);            
    }
    else if(!strcmp(argv[i],SCHEDULER_TYPE)){
      schedulerTypeOptions>=1 ? repeatIndex=i : schedulerTypeOptions++;
      strcpy(schedulerType,argv[i+1]);
    }
    else if(!strcmp(argv[i],PREEMPTIVE)){
      preemptiveOptions>=1 ? repeatIndex=i : preemptiveOptions++;
      flags[PREEMPTIVE_FLAG] = true;
    }
    else if(!strcmp(argv[i],QUANTA)){
      quantaOptions>=1 ? repeatIndex=i : quantaOptions++;     
      strcpy(quanta,argv[i+1]);
      flags[QUANTA_FLAG] = true;              
    }
    else if(!strcmp(argv[i],VERBOSE)){
      verboseOptions>=1 ? repeatIndex=i : verboseOptions++;
      flags[VERBOSE_FLAG] = true;
    }
    // if a repeat is found, its index will be saved
    // since repeatIndex will change from 0, it will evaluate as true
    if(repeatIndex){
      cout << "\tERROR: " << argv[i] << " has been entered multiple times\n";
      exit(1);
    }
  }
  
  // makes sure that input numbers and flags are valid, if not we gracefully exit
  if(inputErrorCheck(pages,frames,frameSize,schedulerType,quanta,flags)) exit(1);
  
  inputFile.open(fileName);
  if(!inputFile){
    cout << "\tERROR: File not opened\n";
    exit(1);
  }
  
  PCB block;
  string fileInput,arrival,burst,priority;
  bool sameProcess;
  
  inputFile >> fileInput;
  while(!inputFile.eof()){
    if(idErrorCheck(fileInput)) exit(1);
    block.pid = atoi(fileInput.substr(2,fileInput.length()).c_str());
    
    inputFile >> arrival;
    inputFile >> burst;
    inputFile >> priority;
    if(loadErrorCheck(block.pid,arrival,burst,priority)) exit(1);
    block.arrival = atoi(arrival.c_str());
    block.burst = atoi(burst.c_str());
    block.priority = atoi(priority.c_str());
    
    sameProcess = true;
    inputFile >> fileInput;
    for(int i=0;i<block.burst;i++){
      // if a pid is seen when trying to read in addresses, stop pushing to the address queue
      if(fileInput.find(PID_FORM)!=string::npos) sameProcess=false;
      if(!inputFile.eof() && sameProcess){
        if(addressErrorCheck(block.pid,atoi(frameSize),atoi(pages),fileInput)) exit(1);
        block.addresses.push(atoi(fileInput.c_str()));
        inputFile >> fileInput;
      }
    }
    if(block.burst!=block.addresses.size() || (!inputFile.eof() && fileInput.find(PID_FORM)==string::npos)){
      cout << "\tERROR: " << PID_FORM << block.pid << " must have a number of memory addresses equivalent to its burst time [1 address: 1 time unit]\n";
      exit(1);
    }
    
    processes.push_back(block);
    // will clear block's queue of addresses to prepare for next process
    while (!block.addresses.empty()) block.addresses.pop();
  }
  inputFile.close();
  
  // writes processes to a separate file without their addresses
  ofstream processesFile(CPU_FILE);
  for(int i = 0; i < processes.size(); i++) {
    processesFile << "P_" << processes.at(i).pid << " "
		  << processes.at(i).arrival << " "
		  << processes.at(i).burst << " "
		  << processes.at(i).priority << endl;
  }
  processesFile.close();
  
  // FCFS gives erroneous answers
  cout << "\nProcesses scheduling...\n\n";
  if (strcmp(schedulerType, FIRST_COME_FIRST_SERVE) == 0) {
    fcfs.loadProcessesFromFile(CPU_FILE);
    fcfs.execute(flags[VERBOSE_FLAG]);
    cout << "\tPlease be aware average wait does not seem to be correct most of the time.\n";
    cout << "Scheduling successful!\n";
  }
  // this will run partially and then core dump, so we commented it out
  else if (strcmp(schedulerType, SHORTEST_JOB_FIRST) == 0) {
    if (flags[PREEMPTIVE_FLAG]) {
      // sjf.putQueue(CPU_FILE,4,true);
      // sjf.loadProcessesFromFile(CPU_FILE, true);
      // sjf.executePremtion();
      cout << "\tUnfortunately Baby OS will core dump if preemptive SJF is used. Please use FCFS.\n";
    }
    else{
      // sjf.putQueue(CPU_FILE,4,false);
      // sjf.loadProcessesFromFile(CPU_FILE, false);
      // sjf.execute();
      cout << "\tUnfortunately Baby OS will core dump if nonpreemptive SJF is used. Please use FCFS.\n";
    }
    cout << "Scheduling failed.\n";
  }
  // this will run partially and then core dump, so we commented it out
  else if (strcmp(schedulerType, PRIORITY) == 0) {
    if (flags[PREEMPTIVE_FLAG]) {
      // pri.putQueue(CPU_FILE,4,true);
      // pri.loadProcessesFromFile(CPU_FILE, true);
      // pri.executePremtion();
      cout << "\tUnfortunately Baby OS will core dump if preemptive Priority is used. Please use FCFS.\n";
    }
    else {
      // pri.putQueue(CPU_FILE,4,false);
      // pri.loadProcessesFromFile(CPU_FILE, false);
      // pri.execute();
      cout << "\tUnfortunately Baby OS will core dump if nonpreemptive Priority is used. Please use FCFS.\n";
    }
    cout << "Scheduling failed.\n";
  } 
  // round robin was entirely commented out in the header, so we couldn't implement it
  else if (strcmp(schedulerType, ROUND_ROBIN) == 0) {
    cout << "\tUnfortunately Baby OS does not have RR implemented yet. Please use FCFS.\n";
    cout << "Scheduling failed.\n";
  }
  
  cout << "Processes paging...\n";
  if (strcmp(pagerType, FIRST_IN_FIRST_OUT) == 0 ||
      strcmp(pagerType, LEAST_RECENT_USED) == 0 ||
      strcmp(pagerType, MOST_FREQUENT_USED) == 0 ||
      strcmp(pagerType, RANDOM) == 0) {
    
    int frameInt = atoi(frames);  
    int pageInt = atoi(pages);    
    int frameSizeInt = atoi(frameSize); 
    string pid;
    for (int i = 0; i < processes.size(); i++) {
      // redeclare the PageTable and PageReplacementSimulator in order to reset pageFaultCount and the pagetable for the next process 
      PageReplacementSimulator pageSimulator;
      PageTable pageTable(pageInt);
      pid = to_string(processes[i].pid);   
      pageSimulator.simulation(processes[i].addresses, pid, pageTable, frameInt, pageInt, frameSizeInt, flags[VERBOSE_FLAG], pagerType);
    }
    cout << "\n\tPlease note that processes are paged in the order in which they appear in \"" << fileName << "\"\n";
    cout << "Paging successful!\n\n";
  }
  return 0;
}

bool addressErrorCheck(int pid,int pageSize, int pages, string address){
  if(!integerCheck(address)){
    cout << "\tERROR: Address " << address << " of " << PID_FORM << pid << " must be a non-negative integer\n";
    return true;  
  }
  // check if a process's address is possible within given frames and size
  else if((atoi(address.c_str())/pageSize) >= pages){
    cout << "\tERROR: Address " << address << " of " << PID_FORM << pid << " cannot exist within " << pages << " " << pageSize << " byte pages\n";
    return true;
  }
  return false;
}

bool loadErrorCheck(int pid, string arrival, string burst, string priority){
  bool error = false;
  if(!integerCheck(arrival) || atoi(arrival.c_str()) < 0){
    cout << "\tERROR: Arrival time of " << PID_FORM << pid << " must be a non-negative integer\n";
    error = true;
  }
  if(!integerCheck(burst) || atoi(burst.c_str()) <= 0){
    cout << "\tERROR: CPU burst of " << PID_FORM << pid << " must be a non-negative integer\n";
    error = true;
  }
  if(!integerCheck(priority) || (atoi(priority.c_str()) > MAX_PRIORITY || atoi(priority.c_str()) < MIN_PRIORITY)){
    cout << "\tERROR: Priority of " << PID_FORM << pid << " must be an integer from " << MIN_PRIORITY << " to " << MAX_PRIORITY << "\n";
    error = true;
  }
  return error;
}

bool integerCheck(string integerString){
  int integer = atoi(integerString.c_str());
  string comparisonString, zeros = "";
  
  // makes a string of the prepended zeros and stops once a non-zero character is found
  for(int i=0;i<integerString.length();i++){
    if(integerString[i]=='0'){
      zeros += integerString[i];
    }
    else{
      i=integerString.length();
    }
  }
  // adds the zeros in front of the converted integer to account for any integers with prepended zeros
  comparisonString = zeros + to_string(integer);
  // if the integerString is a positive integer prepended with zero(s) or not, it will be accepted
  // OR if the integerString is just a bunch of zeros then it will also be accepted
  if(!strcmp(zeros.c_str(),integerString.c_str()) || !strcmp(comparisonString.c_str(),integerString.c_str())) return true;
  return false;
}

bool inputErrorCheck(string pages, string frames, string frameSize, string schedulerType, string quanta, bool *flags){
  bool error = false;
  if(!strcmp(schedulerType.c_str(),FIRST_COME_FIRST_SERVE) && flags[PREEMPTIVE_FLAG]){
    cout << "\tERROR: FCFS cannot be preemptive\n";
    error = true;
  }
  if(!strcmp(schedulerType.c_str(),ROUND_ROBIN) && !flags[PREEMPTIVE_FLAG]){
    cout << "\tERROR: RR must be preemptive\n";
    error = true;
  }
  if(strcmp(schedulerType.c_str(),ROUND_ROBIN) && flags[QUANTA_FLAG]){
    cout << "\tERROR: Only RR should have a quanta specification\n";
    error = true;
  }
  if(flags[QUANTA_FLAG] && (!integerCheck(quanta) || (atoi(quanta.c_str()) <= 0))){
    cout << "\tERROR: Quanta must be a positive integer\n";
    error = true;
  }
  if(!integerCheck(pages) || atoi(pages.c_str()) <= 0){
    cout << "\tERROR: Pages must be a positive integer\n";
    error = true;
  }
  if(!integerCheck(frames) || (atoi(frames.c_str()) <= 0 || atoi(frames.c_str()) > MAX_FRAMES)){
    cout << "\tERROR: Frames must be a positive integer between 1 and " << MAX_FRAMES << "\n";
    error = true;
  }
  if(!integerCheck(frameSize) || atoi(frameSize.c_str()) <= 0){
    cout << "\tERROR: Frame size must be a positive integer\n";
    error = true;
  }
  else{
    unsigned int frameBytes = atoi(frameSize.c_str());
    if((frameBytes & (frameBytes - 1)) != 0){
      cout << "\tERROR: Frame size must be a power of two\n";
      error = true;
    }
  }
  return error;   
}

int commandErrorCheck(int argc, char** argv){
  bool error = false;
  int fileIndex = 0;
  if(!strcmp(argv[argc-1],PAGER_TYPE) || !strcmp(argv[argc-1],SCHEDULER_TYPE) || !strcmp(argv[argc-1],PAGES) || !strcmp(argv[argc-1],FRAMES) || !strcmp(argv[argc-1],FRAME_SIZE) || !strcmp(argv[argc-1],QUANTA)){
    cout << "\tERROR: " << argv[argc-1] << " requires an input\n";
    error = true;
  }
  else{
    for(int i=1;i<argc;i++){
      // if one of the inputs is not a valid option and does not come after an option that requires an input then there's an error
      if(strcmp(argv[i],PAGER_TYPE) && strcmp(argv[i],SCHEDULER_TYPE) && strcmp(argv[i],PAGES) && strcmp(argv[i],FRAMES) && strcmp(argv[i],FRAME_SIZE) && strcmp(argv[i],QUANTA) && strcmp(argv[i],PREEMPTIVE) && strcmp(argv[i],VERBOSE)){
        if(strcmp(argv[i-1],PAGER_TYPE) && strcmp(argv[i-1],SCHEDULER_TYPE) && strcmp(argv[i-1],PAGES) && strcmp(argv[i-1],FRAMES) && strcmp(argv[i-1],FRAME_SIZE) && strcmp(argv[i-1],QUANTA)){
          if(i!=argc-1){
            cout << "\tERROR: " << argv[i] << " is not a valid option\n";
            error = true;
          }
          else{
            fileIndex = i;
          }
        }
      }
      else if(!strcmp(argv[i],PAGER_TYPE)){
        if(strcmp(argv[i+1],FIRST_IN_FIRST_OUT) && strcmp(argv[i+1],LEAST_RECENT_USED) && strcmp(argv[i+1],MOST_FREQUENT_USED) && strcmp(argv[i+1],RANDOM)){
          cout << "\tERROR: " << argv[i+1] << " is not a valid pager type {FIFO|LRU|MFU|Random}\n";
          error = true;
        }
      }
      else if(!strcmp(argv[i],SCHEDULER_TYPE)){
        if(strcmp(argv[i+1],FIRST_COME_FIRST_SERVE) && strcmp(argv[i+1],SHORTEST_JOB_FIRST) && strcmp(argv[i+1],PRIORITY) && strcmp(argv[i+1],ROUND_ROBIN)){
          cout << "\tERROR: " << argv[i+1] << " is not a valid scheduler type {FCFS|SJF|Priority|RR}\n";
          error = true;
        }
      }
    }
  }
  if(error){
    return -1;
  }
  return fileIndex;
}

bool idErrorCheck(string fileInput){
  bool error = false;
  if(fileInput.find(PID_FORM) != 0 || fileInput.length() < 3){
    cout << "\tERROR: Process id " << fileInput << " is not of form " << PID_FORM << "#\n";
    error = true;
  }
  else if(!integerCheck(fileInput.substr(2,fileInput.length()).c_str()) || atoi(fileInput.substr(2,fileInput.length()).c_str()) < 0){
    cout << "\tERROR: Process id " << fileInput << " must be a non-negative integer\n";
    error = true;
  }
  return error;
}
