// clarifications: error checking & format of tab in commands file
// Processes: are all processes queued in the ready queue, and when timer interrupt occurs, current running is moved to end of ready queue

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
//  CITS2002 Project 1 2023
//  Student1:   23709058   Lewei Xu
//  Student2:   STUDENT-NUMBER2   NAME-2

//  myscheduler (v1.0)

#define MAX_DEVICES                                4
#define MAX_DEVICE_NAME                           20
#define MAX_COMMANDS                              10
#define MAX_COMMAND_NAME                          20
#define MAX_SYSCALLS_PER_PROCESS                  40
#define MAX_RUNNING_PROCESSES                     50

#define DEFAULT_TIME_QUANTUM                     100

#define TIME_CONTEXT_SWITCH                        5
#define TIME_CORE_STATE_TRANSITIONS               10
#define TIME_ACQUIRE_BUS                          20

#define CHAR_COMMENT                             '#'
#define CHAR_DEVICE                              'd'
#define CHAR_TIME_QUANTUM                        't'
#define CHAR_SYSCALL                            '\t'

struct {
    char name[MAX_DEVICE_NAME];
    int r_speed;
    int w_speed;
} devices[MAX_DEVICES];

struct {
    char name[MAX_COMMAND_NAME];
    struct {
        int time;
        char name[6];
        char arg1[12];
        int arg2;
    } syscalls[MAX_SYSCALLS_PER_PROCESS];
} commands[MAX_COMMANDS];

// Need struct/queue that holds process ID

int read_queue[MAX_RUNNING_PROCESSES];

int read_num(char line[], int *line_i) {
    bool found = false;
    int num_i = 0;
    char num[11];
    while (1) {
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

void read_word(char line[], int *line_i, char *word) {
    bool found = false;
    int word_i = 0;
    while (1) {
        if (line[*line_i] != '\t' && line[*line_i] != ' ') {
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

int read_sysconfig(char filename[])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file!");
        exit(EXIT_FAILURE);
    }

    int TQ = DEFAULT_TIME_QUANTUM, device_num = 0;
    char line[128];
    printf("Devices:\n");
    while ((fgets(line, sizeof line, file)) != NULL) {
        if (line[0] == CHAR_COMMENT) {
            continue;
        }
        else if (line[0] == CHAR_DEVICE) {
            char device_name[MAX_DEVICE_NAME];
        //  skip word "device"    
            int line_i = 6;
            read_word(line, &line_i, device_name);
            strcpy(devices[device_num].name, device_name);

            devices[device_num].r_speed = read_num(line, &line_i);
            devices[device_num].w_speed = read_num(line, &line_i);

            printf("%s %dBps %dBps\n", devices[device_num].name, 
            devices[device_num].r_speed, devices[device_num].w_speed);
            device_num++;
        }
        else if (line[0] == CHAR_TIME_QUANTUM) {
            int line_i = 11;
            TQ = read_num(line, &line_i);
        }
    }
    fclose(file);
    printf("Time quantum: %dusecs\n", TQ);
    return TQ;
}

void read_commands(char filename[])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error in opening file!");
        exit(EXIT_FAILURE);
    }

    printf("Commands:\n");
    int command_i = 0;
    char line[128];
    while ((fgets(line, sizeof line, file)) != NULL) {
        if (line[0] == CHAR_COMMENT) {
            continue;
        }
        else if (isalpha(line[0])) {
            command_i++;
            int line_i = 0;
            char command_name[MAX_COMMAND_NAME];
            read_word(line, &line_i, command_name);
            strcpy(commands[command_i].name, command_name);
            printf("%s", commands[command_i].name);
        }
        else {
            int line_i = 0;
            int time = read_num(line, &line_i);
            char syscall_name[6];
            line_i += 4;
            read_word(line, &line_i, syscall_name);
            commands[command_i].syscalls->time = time;
            strcpy(commands[command_i].syscalls->name, syscall_name);
            if (syscall_name == "sleep") {
                int sleep_time = read_num(line, &line_i);
                commands[command_i].syscalls->arg2;
            }
            else if (syscall_name == "spawn") {
                char command_name[MAX_COMMAND_NAME];
                read_word(line, &line_i, command_name);
                strcpy(commands[command_i].syscalls->arg1, command_name);
            }
            else if (syscall_name == "read" || syscall_name == "write") {
                char device_name[MAX_DEVICE_NAME];
                read_word(line, &line_i, device_name);
                strcpy(commands[command_i].syscalls->arg1, device_name);
                int size = read_num(line, &line_i);
                commands[command_i].syscalls->arg2 = size;
            }
            printf("%d %s", commands[command_i].syscalls->time, commands[command_i].syscalls->name);
        }
    }
    fclose(file);
}

void execute_commands(void)
{
// Simulate passage of time, with max of 2000 seconds
}

int main(int argc, char *argv[])
{
//  Ensure correct number of command line arguments
    if(argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//  Read system configuration file and store in structure "devices", returns time quantum
    read_sysconfig(argv[1]);

//  READ THE COMMAND FILE
    read_commands(argv[2]);

//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4