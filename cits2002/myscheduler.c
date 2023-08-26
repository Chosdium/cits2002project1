// clarifications: error checking & format of tab in commands file
// Processes: are all processes queued in the ready queue, and when timer interrupt occurs, current running is moved to end of ready queue

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
//  CITS2002 Project 1 2023
//  Student1:   23709058   Lewei Xu
//  Student2:   23413154   Edwin Tang

//  myscheduler (v1.0)
//  This program simulates a single-CPU system with the 5-State Model of Process Execution
//  OS exits when all processes finish exeuting

#define MAX_DEVICES                                4
#define MAX_DEVICE_NAME                           20
#define MAX_COMMANDS                              10
#define MAX_COMMAND_NAME                          20
#define MAX_SYSCALLS_PER_PROCESS                  40
#define MAX_RUNNING_PROCESSES                     50
#define MAX_SYSCALL_LENGTH                         6

#define DEFAULT_TIME_QUANTUM                     100

#define TIME_CONTEXT_SWITCH                        5
#define TIME_CORE_STATE_TRANSITIONS               10
#define TIME_ACQUIRE_BUS                          20

#define CHAR_COMMENT                             '#'
#define CHAR_DEVICE                              'd'
#define CHAR_TIME_QUANTUM                        't'
#define CHAR_SYSCALL                            '\t'

//  array of struct holding device info read from sysconfig
//  fields: devicename, read speed, write speed 
struct {
    char name[MAX_DEVICE_NAME];
    int r_speed;
    int w_speed;
} devices[MAX_DEVICES];

//  struct to hold each system call
//  fields: exec_time, system call name, device name accessed/process spawned, size read or written/time specified
struct syscall{
    int exec_time;
    char name[6];
    char deviceOrProcess[12];
    int sizeOrTime;
};

//  array of system calls associated with process
struct syscall current_syscalls[MAX_SYSCALLS_PER_PROCESS];

//  struct array within in struct array, holding info on commands and corresponding system calls
struct {
    char name[MAX_COMMAND_NAME];
    struct syscall syscalls[MAX_SYSCALLS_PER_PROCESS];
    // can remove 
    /* struct {
      int time;
      char name[6];
      char arg1[12];
      int arg2;
    } syscalls[MAX_SYSCALLS_PER_PROCESS];*/
} commands[MAX_COMMANDS];

//  Structure holding process details; fields: name of command, process ID, index to specific system, cumulative time on processor
struct process_details{
    char command_name[MAX_COMMAND_NAME];
    int pid;
    int syscall_index;
    int cumul_time;
};

//  Structure array for running process/ready queue/blocked queue
struct process_details running;
struct process_details ready[MAX_RUNNING_PROCESSES];
struct process_details blocked[MAX_RUNNING_PROCESSES][MAX_DEVICES];

//  A function that returns integer numbers
//  Check indexed char is a integer, append to char array if so and append NULL char when integers are exhausted
int read_num(char line[], int *line_i) {
    bool found = false;
    int num_i = 0;
    char num[11];
    while (true) {
        if (isdigit(line[*line_i])) {
            found = true;
            num[num_i] = line[*line_i];
            num_i++;
        }
        else if (found == true) {
            num[num_i] = '\0';
            return atoi(num);
        }
        (*line_i)++;
    }
}

// Function that reads words
void read_word(char line[], int *line_i, char *word) {
    bool found = false;
    int word_i = 0;
    while (true) {
        //  When indexed a char, found becomes true and char gets assign from line to word
        if (line[*line_i] != '\t' && line[*line_i] != ' ' && line[*line_i] != '\n') {
            found = true;
            word[word_i] = line[*line_i];
            word_i++;
        }
        else if (found == true) {
            word[word_i] = '\0';
            return;
        }
        (*line_i)++;
    }
}

//   Function that reads the system configuration file and stores its info into struct array defined earlier
int read_sysconfig(char filename[])
{
    //  Open file for reading
    FILE *file = fopen(filename, "r");
    //  Account for read error
    if (file == NULL) {
        printf("Error opening file!");
        exit(EXIT_FAILURE);
    }
    //  Copy detail of each device
    int TQ = DEFAULT_TIME_QUANTUM, device_num = 0;
    char line[128];
    printf("Devices:\n");
    while ((fgets(line, sizeof line, file)) != NULL) {   
        //  Skip comment lines
        if (line[0] == CHAR_COMMENT) {
            continue;
        }
        //  Read details for lines starting with device
        else if (line[0] == CHAR_DEVICE) {
            char device_name[MAX_DEVICE_NAME];    
            int line_i = 6;      //  skip word "device"
            read_word(line, &line_i, device_name);
            strcpy(devices[device_num].name, device_name);

            devices[device_num].r_speed = read_num(line, &line_i);
            devices[device_num].w_speed = read_num(line, &line_i);

            printf("%s %dBps %dBps\n", devices[device_num].name, 
            devices[device_num].r_speed, devices[device_num].w_speed);
            device_num++;
        }
        //  Change time quantum to value in file if it exists
        else if (line[0] == CHAR_TIME_QUANTUM) {
            int line_i = 11;
            TQ = read_num(line, &line_i);
        }
    }
    fclose(file);
    printf("Time quantum: %dusecs\n", TQ);
    return TQ;
}

// Function that reads command file 
void read_commands(char filename[])
{
    FILE *file = fopen(filename, "r");
    //  Account for file open error
    if (file == NULL) {
        printf("Error in opening file!");
        exit(EXIT_FAILURE);
    }

    int command_index = -1;
    char line[128];
    int syscall_i;
    while ((fgets(line, sizeof line, file)) != NULL) {
        //  Ignore comment lines
        if (line[0] == CHAR_COMMENT) {
            continue;
        }

        //  If first char is a letter, store command name
        else if (isalpha(line[0])) {
            command_index++;
            int line_i = 0;
            char command_name[MAX_COMMAND_NAME];
            read_word(line, &line_i, command_name);
            strcpy(commands[command_index].name, command_name);
            syscall_i = 0;
        }

        //  If first char is tab, store details of system call
        else if (line[0] == '\t') {
            int line_i = 0;
            int time = read_num(line, &line_i);
            char syscall_name[6];
            line_i += 4;
            read_word(line, &line_i, syscall_name);
            commands[command_index].syscalls[syscall_i].exec_time = time;
            strcpy(commands[command_index].syscalls->name, syscall_name);
            
            //  For sleep time
            if (strcmp(syscall_name, "sleep") == 0) {
                int sleep_time = read_num(line, &line_i);
                commands[command_index].syscalls[syscall_i].sizeOrTime = sleep_time;
            }
            //   For spawn
            else if (strcmp(syscall_name, "spawn") == 0) {
                char command_name[MAX_COMMAND_NAME];
                read_word(line, &line_i, command_name);
                strcpy(commands[command_index].syscalls[syscall_i].deviceOrProcess, command_name);
            }

            //  For read and write
            else if (strcmp(syscall_name, "read") == 0 || strcmp(syscall_name, "write") == 0) {
                char device_name[MAX_DEVICE_NAME];
                read_word(line, &line_i, device_name);
                strcpy(commands[command_index].syscalls[syscall_i].deviceOrProcess, device_name);
                int size = read_num(line, &line_i);
                commands[command_index].syscalls[syscall_i].sizeOrTime = size;
            }
            syscall_i++;
        }
    }

    /* for (int i = 0; i < MAX_COMMANDS; i++) {
        for (int j = 0; j < MAX_SYSCALLS_PER_PROCESS; j++) {
            printf("%s", commands[i].syscalls[0].name);
        }
    }*/
    fclose(file);
}

/* This is wrong btw
void commands_dump(void) {
     printf("Commands:\n");
     for (int command_index = 0; command_index < MAX_COMMANDS; command_index++) {
         printf("%s", commands[command_index].name);
         printf("\t%dusecs %s\n", commands[command_index].syscalls->time, commands[command_index].syscalls->name);
     }
}*/

//  Function that shifts process array index forward
//  simulate process moving from ready to running
void shift_queue(void){
    for (size_t i = 0; i < MAX_COMMANDS; i++){
        ready[i] = ready[i+1];
    }
}

//  Function which places struct into queue, need a global or static variable to hold index so running processes can be put into queue
void place_into_queue(){
    // ...
}

//  ?Function that simulates spawn system call?
void sim_spawn(int queue_i, char command_name[], int pid, int time) {
    printf("@%08d   Spawn '%s': pid=%d NEW->READY\n", time, command_name, pid);
    strcpy(ready[queue_i].command_name, commands[queue_i].name);
    ready[queue_i].pid = pid;
    ready[queue_i].syscall_index = 0;
    ready[queue_i].cumul_time = 0;
}


// Currently only to put first process onto running
// Put process from ready into running
/*int context_change(int pid, int time) {
    printf("@%08d   Context switch: pid=%d READY->RUNNING\n", time, pid);
    running = ready[0];
    shift_queue();
    printf("@%08d   clock + %d\n", time, 5);
    return (time + 5);
};*/

//  Function which accounts for most state changes
int context_change(int pid, int time, char *change) {
    //  Ready to Running
    if (strcmp(change, "RERU") == 0){
        printf("@%08d   Context switch: pid=%d READY->RUNNING\n", time, pid);
        running = ready[0];
        shift_queue();
        printf("@%08d   clock + %d\n", time, 5);
        return (time + 5);
    }

    //  Running to Ready
    else if (strcmp(change, "RURE")==0){
        printf("@%08d   Context switch: pid=%d RUNNING->READY\n", time, pid);
        // ...
        return (time + 10);
    }
    
    //  Running to Blocked
    else if (strcmp(change, "RUBL")==0){
        printf("@%08d   Context switch: pid=%d RUNNING->READY\n", time, pid);
        // ...
        return (time + 10);
    }

    //  Blocked to Ready
    else if (strcmp(change, "BLRE")==0){
        printf("@%08d   Context switch: pid=%d BLOCKED->READY\n", time, pid);
        // ...
        return (time + 10);
    }

    //other cases 
    else{
        return time;
    }
}

//  
void get_current_syscalls(char command_name[]) {
    for (int i = 0; i < MAX_COMMANDS; i++) {
        if (strcmp(commands[i].name, command_name) == 0) {
            for (int j = 0; j < MAX_SYSCALLS_PER_PROCESS; j++) {
                current_syscalls[j] = commands[i].syscalls[j];
            }
            //printf("%s\n", commands[i].syscalls[0].name);
            return;
        }
    }
}

// 
int get_time_interval(void) {
    if (running.syscall_index == 0) {
        return current_syscalls[0].exec_time;
    }
    else {
        return (current_syscalls[running.syscall_index].exec_time 
        - current_syscalls[running.syscall_index-1].exec_time);
    }
}


// Function simulating the system/most important function
void execute_commands(int t_quantum)
{
    //  Define variables used for entire duration
    int t_total = 0, t_process = 0, n_processes = 0, pid = 0;
    printf("@%08d   REBOOTING\n", t_total);
    //  Spawn the very first command
    sim_spawn(0, commands[0].name, pid, t_total);
    t_total = context_change(pid, t_total, "RERU");
    n_processes++;
    //  Simulate passage of time, with max of 2000 seconds
    while(t_total < 20) {
        get_current_syscalls(running.command_name);
        int t_interval = get_time_interval();
        
        //  The process has been on the CPU long enough for the next syscall to execute
        // ?Isn't this when a process has called the exit system call?
        if (t_process == t_interval && t_process <= t_quantum) {
            if (strcmp(current_syscalls[running.syscall_index].name, "exit") == 0) {
                printf("@%08d   System-call 'exit': pid=%d RUNNING->EXIT\n", t_total, pid);
                n_processes--;
            }
        }
        
        //  When time taken y process is less than time quantum
        if (t_process < t_quantum) {
            printf("@%08d   .\n", t_total);
            running.cumul_time += 1;
            t_process += 1;
        }
        
        //  
        else if (t_process == t_quantum) {
            printf("@%08d   Time quantum expired: pid=%d RUNNING->READY\n", t_total, pid);
            //context_switch();
        }
        
        //
        if (n_processes == 0) {
            printf("@%08d   n_processes=0 SHUTTING DOWN\n", t_total);
            printf("measurements %d %d\n", t_total, calc_percentage());
            exit(EXIT_SUCCESS);
        }
        t_total++;
    }
}

int main(int argc, char *argv[])
{
//  Ensure correct number of command line arguments
    if(argc != 3) {
        fprintf(stderr, "Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//  Read system configuration file and store in structure "devices", returns time quantum
    int time_quantum = read_sysconfig(argv[1]);

//  READ THE COMMAND FILE
    read_commands(argv[2]);
//    commands_dump();

//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands(time_quantum);

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
