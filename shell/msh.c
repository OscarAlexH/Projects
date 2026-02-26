#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_SIZE 32 //this is for the size
#define MAX_ARGS 10  // this is for the amount of arguments
#define SIZE_HISTORY 50 // for the amount of vlaues in the history it will store

char history[SIZE_HISTORY][MAX_COMMAND_SIZE]; 
int count_history =0; // a counter for the amount of history values 

void add_history(const char *command){ // this is funciton for the history command 
    strncpy(history[count_history % SIZE_HISTORY], command, MAX_COMMAND_SIZE -1); 
    history[count_history % SIZE_HISTORY][MAX_COMMAND_SIZE -1] = '\0'; 
    count_history++;
}

void print_history(){
    int start = (count_history < SIZE_HISTORY) ? 0: count_history % SIZE_HISTORY; //for the staring index
    int counts = (count_history < SIZE_HISTORY) ? count_history : SIZE_HISTORY; //for the amount it will print for history

    for (int i = 0; i < counts; i++){   // loop through the history and find the index to then print the numner and command
        int index = (start + i) % SIZE_HISTORY;
        printf("[%d] %s\n", i + 1 , history[index]);
    }
}




int main() {
    char input_string[MAX_COMMAND_SIZE]; //buffer

    while (1) {
        printf("msh> ");  //msh prompt
        fflush(stdout); 

        // reads user input
        if (fgets(input_string, sizeof(input_string), stdin) == NULL) {
            break;  // Exit if input fails
        }

        // get rid off  newline character
        size_t input_length = strlen(input_string);
        if (input_length > 0 && input_string[input_length - 1] == '\n') {
            input_string[input_length - 1] = '\0';
        }

        // ignore empty input
        if (strlen(input_string) == 0) {
            continue;
        }

        //  for exit command
        if (strcmp(input_string, "exit") == 0) {
            printf("Exiting msh...\n");
            break;
        }

        // tokenize the input into command and arguments
        char *token;
        char *args[MAX_ARGS];  // a array to store command and arguments
        int arg_count = 0;

        token = strtok(input_string, " ");  // get the first token command
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");  // for the next token command
        }
        args[arg_count] = NULL;  // null term the array for execvp()

        // for cd command 
        if (strcmp(args[0], "cd") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd failed");
                }
            }
            continue;  
        }

        // foring a child process
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            continue;
        }

        if (pid == 0) {
            // child does the command
            if (execvp(args[0], args) == -1) {
                perror("Error");
                exit(1);
            }

        } else {
            // parent waits for the child to finish
            int status;
            waitpid(pid, &status, 0);
        }

        if (strcmp(input_string, "history")== 0){
            print_history();
            continue;
        }

        add_history(input_string);

        //  this is for the ! for the last command used
if (input_string[0] == '!') {
    // parse the command number from the input
    int cmd_number = atoi(&input_string[1]) - 1;  // make zero index
    
    // check if the command number is good
    if (cmd_number >= 0 && cmd_number < SIZE_HISTORY && history[cmd_number][0] != '\0') {
        strncpy(input_string, history[cmd_number], MAX_COMMAND_SIZE);
        input_string[MAX_COMMAND_SIZE - 1] = '\0';  //  null termination
    }
        printf("Re-running command: %s\n", input_string); 

        // tokenize the command again to prepare for execution
        char *token;
        char *args[MAX_ARGS];  
        int arg_count = 0;

        token = strtok(input_string, " ");  
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");  
        }
        args[arg_count] = NULL;  

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            continue;
        }

        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                exit(1);
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    continue;  // Skip the rest of the loop after handling history command
}

        

    }

    return 0;
}
