// FILE: babyOS.cpp
// A Bautista, B Franco, E Mora
// OS, Fall 2023, Transy U
//
// driver for Group 3 babyOS 
//
// http://stackoverflow.com/questions/5590381/ddg#5591169
// https://cplusplus.com/reference/vector/vector/
// baby os is an implementation of other group's code that implements page replacement algorithms 
// it also incorporates CPU Scheduling 
// CPU Scheduling unfortunately was not a great implementation so a lot of the schedulers do not work, or only partially work
// CPU scheduler FCFS is the only scheduler that runs fully so it is the only one we allow to run
// however it does not return an accurate Average wait time
// Pager implementatiopn is fully functional and page replacements algorithms all work
// these are FIFO, LRU, Random, and MFU
// in the command line insert --pagerType --schedulerType --frameSize --pages -- --quanta --fileName
// only add the ones you will use otherwise defaults are present

#include "FCFS.h"
#include "pageReplacementSimulator.h"
#include "pageTable.h"
#include "Priority.h"
#include "procManagement.h" 
#include "RoundRobin.h"
#include "SJF.h"
#include "symConsts.h"

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
  Priority pri;
  FCFS fcfs;
  SJF sjf;
  PageReplacementSimulator pageSimulator;
  
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
  
  // counts of how many times each option is entered and index of a repeat
  int repeatIndex = 0;
  int pagerTypeOptions=0,pageOptions=0,frameOptions=0,sizeOptions=0;
  int schedulerTypeOptions=0,preemptiveOptions=0,quantaOptions=0,verboseOptions=0;

  // stores the value of each specified argument so that it can be used in page replacement and cpu scheduling
  // if any repeats are found, then it will gracefully exit
  for(int i=1;i<argc;i++){
    if(!strcmp(argv[i],PAGER_TYPE)){
      pagerTypeOptions>1 ? repeatIndex=i : pagerTypeOptions++;
      strcpy(pagerType,argv[i+1]);
    }
    else if(!strcmp(argv[i],PAGES)){
      pageOptions>1 ? repeatIndex=i : pageOptions++;
      strcpy(pages,argv[i+1]);
    }
    else if(!strcmp(argv[i],FRAMES)){
      frameOptions>1 ? repeatIndex=i : frameOptions++;  
      strcpy(frames,argv[i+1]);             
    }
    else if(!strcmp(argv[i],FRAME_SIZE)){
      sizeOptions>1 ? repeatIndex=i : sizeOptions++;
      strcpy(frameSize,argv[i+1]);            
    }
    else if(!strcmp(argv[i],SCHEDULER_TYPE)){
      schedulerTypeOptions>1 ? repeatIndex=i : schedulerTypeOptions++;
      strcpy(schedulerType,argv[i+1]);
    }
    else if(!strcmp(argv[i],PREEMPTIVE)){
      preemptiveOptions>1 ? repeatIndex=i : preemptiveOptions++;
      flags[PREEMPTIVE_FLAG] = true;
    }
    else if(!strcmp(argv[i],QUANTA)){
      quantaOptions>1 ? repeatIndex=i : quantaOptions++;     
      strcpy(quanta,argv[i+1]);
      flags[QUANTA_FLAG] = true;              
    }
    else if(!strcmp(argv[i],VERBOSE)){
      verboseOptions>1 ? repeatIndex=i : verboseOptions++;
      flags[VERBOSE_FLAG] = true;
    }

    // if a repeat is found, its index will be saved
    // since repeatIndex will change from 0, it will evaluate as true
    if(repeatIndex){
      cout << "\tERROR: " << argv[i] << " has been entered multiple times\n";
      exit(1);
    }
  }

  //makes sure that input numbers and flags are valid, if not we gracefully exit
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

    queue<int> tempQueue = block.addresses;

    processes.push_back(block);
    while (!block.addresses.empty()) block.addresses.pop();
        processes.back().addresses = tempQueue;
    }

  inputFile.close();

  cout << "\nYay!\n";
  ofstream processesFile("processes.txt");
  int size;
  string CPUFile = "processes.txt";
  for(int i = 0; i < processes.size(); i++) {
      cout << PID_FORM << processes.at(i).pid << " " << processes.at(i).arrival << " " << processes.at(i).burst << " " << processes.at(i).priority << endl;
      processesFile << "P_" << processes.at(i).pid << " "
                    << processes.at(i).arrival << " "
                    << processes.at(i).burst << " "
                    << processes.at(i).priority << endl;

      // copy of addresses for print
      queue<int> printQueue = processes.at(i).addresses;

      while (!printQueue.empty()) {
          cout << printQueue.front() << endl;
          printQueue.pop();
      }
      cout << endl << endl;
  }
  
  processesFile.close();

  // for pager, while for subsequent processes it says that it has a number of page faults, but 
  // it as actually the number of the previous faults plus the faults of itself, ex. P1 faults = 5, P2 faults = 14, P2 itself only had 9 faults.
  if (strcmp(pagerType, FIRST_IN_FIRST_OUT) == 0 ||
    strcmp(pagerType, LEAST_RECENT_USED) == 0 ||
    strcmp(pagerType, MOST_FREQUENT_USED) == 0 ||
    strcmp(pagerType, RIGHT_RANDOM) == 0) {

    for (int i = 0; i < processes.size(); i++) {
        queue<int> addressQueue = processes[i].addresses;
        string pid = to_string(processes[i].pid);
        int frameInt = atoi(frames);  
        int pageInt = atoi(pages);    
        int frameSizeInt = atoi(frameSize); 

        PageTable pageTable(pageInt); 
        pageSimulator.simulation(addressQueue, pid, pageTable, frameInt, pageInt, frameSizeInt, flags[VERBOSE_FLAG], pagerType);
    }
  }

  // now for scheduler, apparently none of these work :(
  //FCFS returns errorneouys answers. The average wait time is off, but we can not tell why
  // We ran several test cases and they did not consistently return erroneuous answers 
  // take this output with a grain of salt
  if (strcmp(schedulerType, FIRST_COME_FIRST_SERVE) == 0) {
    fcfs.loadProcessesFromFile(CPUFile);
    fcfs.execute();
  }
    
    // this scheduler core dumps it will successfully start and partially schedule the processes
    // however it will core dump eventually even with preemption
    // we are saving you from a core dump
  else if (strcmp(schedulerType, SHORTEST_JOB_FIRST) == 0) {
    if (flags[PREEMPTIVE_FLAG]) { 
      sjf.loadProcessesFromFile(CPUFile, true);
      sjf.executePremtion();
    }
    else{
      sjf.loadProcessesFromFile(CPUFile, false);
      sjf.execute();
    }

  } else if (strcmp(schedulerType, PRIORITY) == 0) {
    if (flags[PREEMPTIVE_FLAG]) {
      pri.loadProcessesFromFile(CPUFile, true);
      pri.executePremtion();
    }
    else {
      pri.loadProcessesFromFile(CPUFile, false);
      pri.execute();
    }
  } 
    
   else if (strcmp(schedulerType, ROUND_ROBIN) == 0) {
 	cout << "Unfortunately Baby OS hasn't hit adolescence so we can not do round robin. This will be supported in a future version" << endl;
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
      cout << "\tERROR: Priority of " << PID_FORM << pid << " must be an integer from 0 to 100\n";
      error = true;
  }
  return error;
}

bool integerCheck(string integerString){
  int integer = atoi(integerString.c_str());
  if(!strcmp(to_string(integer).c_str(),integerString.c_str())) return true;
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
        if(strcmp(argv[i+1],FIRST_IN_FIRST_OUT) && strcmp(argv[i+1],LEAST_RECENT_USED) && strcmp(argv[i+1],MOST_FREQUENT_USED) && strcmp(argv[i+1],RIGHT_RANDOM)){
          cout << "\tERROR: " << argv[i+1] << " is not a valid pager type {FIFO|LRU|MFU|RANDOM}\n";
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
    cout << "\tERROR: Process id is not of form " << PID_FORM << "#\n";
    error = true;
  }
  else if(!integerCheck(fileInput.substr(2,fileInput.length()).c_str()) || atoi(fileInput.substr(2,fileInput.length()).c_str()) < 0){
    cout << "\tERROR: Process id must be a non-negative integer\n";
    error = true;
  }
  return error;
}
