#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "hw2.h"

#define ERR_EXIT(s) perror(s), exit(errno);

/*
If you need help from TAs,
please remember :
0. Show your efforts
    0.1 Fully understand course materials
    0.2 Read the spec thoroughly, including frequently updated FAQ section
    0.3 Use online resources
    0.4 Ask your friends while avoiding plagiarism, they might be able to understand you better, since the TAs know the solution, 
        they might not understand what you're trying to do as quickly as someone who is also doing this homework.
1. be respectful
2. the quality of your question will directly impact the value of the response you get.
3. think about what you want from your question, what is the response you expect to get
4. what do you want the TA to help you with. 
    4.0 Unrelated to Homework (wsl, workstation, systems configuration)
    4.1 Debug
    4.2 Logic evaluation (we may answer doable yes or no, but not always correct or incorrect, as it might be giving out the solution)
    4.3 Spec details inquiry
    4.4 Testcase possibility
5. If the solution to answering your question requires the TA to look at your code, you probably shouldn't ask it.
6. We CANNOT tell you the answer, but we can tell you how your current effort may approach it.
7. If you come with nothing, we cannot help you with anything.
*/

// somethings I recommend leaving here, but you may delete as you please
static char root[MAX_FRIEND_INFO_LEN] = "Not_Tako";     // root of tree
// static char friend_info[MAX_FRIEND_INFO_LEN];   // current process info
static char current_info[MAX_FRIEND_INFO_LEN];
// static char friend_name[MAX_FRIEND_NAME_LEN];   // current process name
static char current_name[MAX_FRIEND_NAME_LEN];   // current process name
static int friend_value;    // current process value
//FILE* read_fp = NULL;
static char child_process_ok_msg[43] = "Child writes to parent: Child process OK!\n";

// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(current_name, root) == 0);
}

// a bunch of prints for you
void print_direct_meet(char *current_name) {
    fprintf(stdout, "Not_Tako has met %s by himself\n", current_name);
}

void print_indirect_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako has met %s through %s\n", child_friend_name, parent_friend_name);
}

void print_fail_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako does not know %s to meet %s\n", parent_friend_name, child_friend_name);
}

void print_fail_check(char *parent_friend_name){
    fprintf(stdout, "Not_Tako has checked, he doesn't know %s\n", parent_friend_name);
}

void print_success_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s has adopted %s\n", parent_friend_name, child_friend_name);
}

void print_fail_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s is a descendant of %s\n", parent_friend_name, child_friend_name);
}

void print_compare_gtr(char *current_name){
    fprintf(stdout, "Not_Tako is still friends with %s\n", current_name);
}

void print_compare_leq(char *current_name){
    fprintf(stdout, "%s is dead to Not_Tako\n", current_name);
}

void print_final_graduate(){
    fprintf(stdout, "Congratulations! You've finished Not_Tako's annoying tasks!\n");
}

int task_parsor(char* line, char* task_type, char* argmnt1, char* argmnt2, int* item_read) {
    /*
    41: Implementation 4.1 Meet <parent_friend_name> <child_friend_info>
    42: Implementation 4.2 Check <parent_friend_name>
    43: Implementation 4.3 Adopt <parent_friend_name> <child_friend_name>
    44: Implementation 4.4 Graduate <current_name>
    45: Implementation 4.5 Compare <current_name> <number>
    */
    // Parse the line into task_type, argmnt1, and argmnt2
    *item_read = sscanf(line, "%s %s %s", task_type, argmnt1, argmnt2);

    // Determine task number based on task_type
    if (strcmp(task_type, "Meet") == 0) return 41;
    if (strcmp(task_type, "Check") == 0) return 42;
    if (strcmp(task_type, "Adopt") == 0) return 43;
    if (strcmp(task_type, "Graduate") == 0) return 44;
    if (strcmp(task_type, "Compare") == 0) return 45;
    
    return -1; // Return -1 if task is not recognized
}
// Parses `argmnt2` into `meet_child_name` and `meet_child_value`
int meet_child_parsor(char* argmnt2, char* meet_child_name, int* meet_child_value) {
    char child_value[10];
    int token_read = sscanf(argmnt2, "%[^_]_%s", meet_child_name, child_value);
    
    if (token_read != 2) {
        return -1; // Parsing failed
    }

    *meet_child_value = atoi(child_value); // Convert child_value string to integer
    return 0; // Success
}

int Meet(char *parent_name, friend* friends, int* next_empty_pos, char* argmnt2) {//argmnt2 is child_info now
    int childRD_parentWR[2];
    int childWR_parentRD[2];
    char child_name[MAX_FRIEND_NAME_LEN] = {0};
    int child_value;

    // Create pipes for communication
    if (pipe(childRD_parentWR) == -1) {
        perror("pipe to child failed");
        return -1;
    }
    // Set FD_CLOEXEC for childRD_parentWR pipe
    fcntl(childRD_parentWR[0], F_SETFD, FD_CLOEXEC);
    fcntl(childRD_parentWR[1], F_SETFD, FD_CLOEXEC);

    if (pipe(childWR_parentRD) == -1) {
        perror("pipe to parent failed");
        close(childRD_parentWR[0]);
        close(childRD_parentWR[1]);
        return -1;
    }
    // Set FD_CLOEXEC for childWR_parentRD pipe
    fcntl(childWR_parentRD[0], F_SETFD, FD_CLOEXEC);
    fcntl(childWR_parentRD[1], F_SETFD, FD_CLOEXEC);

    // Fork the process to create a child process
    int pid = fork();
    if (pid == -1) { // fork fail
        perror("fork failed");
        close(childRD_parentWR[0]);
        close(childRD_parentWR[1]);
        close(childWR_parentRD[0]);
        close(childWR_parentRD[1]);
        return -1;
    } else if (pid == 0) { // child
        // Child process
        close(childRD_parentWR[1]); // Close the write end of childRD_parentWR
        close(childWR_parentRD[0]); // Close the read end of childWR_parentRD

        // Redirect standard input and output to the designated file descriptors
        if (dup2(childWR_parentRD[1], PARENT_READ_FD) == -1) { // 這邊會關掉current和原本parent的pipe fd，所以不用再用cloexec，之後adopt中main可能要補cloexec
            perror("dup2 failed for PARENT_READ_FD");
            _exit(1); //exit後所有fd都關掉，包含所有pipe都關掉，parent read 到 EOF，child結束，parent必須要wait child不然會變zombie
        }
        
        if (dup2(childRD_parentWR[0], PARENT_WRITE_FD) == -1) {
            perror("dup2 failed for PARENT_WRITE_FD");
            _exit(1); // child 的 exit要有底線，因為要直接離開去kernel，不要碰到parent STD IO
        }

        // Close original pipe ends after duplicating
        close(childWR_parentRD[1]);
        close(childRD_parentWR[0]);

        // Execute the same program to represent the child node process
        if(execl("./friend", "friend", argmnt2, NULL)==-1){//argmnt2 is argument1, is child_info now
            perror("execl failed"); // execl only returns on failure
            _exit(1);
        }
        perror("unexpected error lead to child process fail");
        _exit(1); // unexpected error
    } else {
        // Parent process
        close(childRD_parentWR[0]); // Close the read end of childRD_parentWR
        close(childWR_parentRD[1]); // Close the write end of childWR_parentRD

        // Avoid child to become zombie process: 如果child 成功建成就會給一個child_process_ok_msg，如果失敗了child 會exit，parent要wait child
        char buf[MAX_CMD_LEN] = {0};
        if(read(PARENT_READ_FD, buf, sizeof(buf)) == EOF){
            int status = 0;
            if(waitpid(pid, &status, 0) == -1){
                perror("Wait fail: Parent wait for the child fail.\n");
            }
        }
        // Call the parser to split `argmnt2` into `meet_child_name` and `meet_child_value`
        if (meet_child_parsor(argmnt2, child_name, child_value) == 0) {
            printf("Child Name: %s, Child Value: %d\n", child_name, child_value);
        } else {
            printf("Error parsing child name and value.\n");
        }

        // Store child information in the parent’s data structure (if needed)
        friend new_child;
        new_child.pid = pid;
        new_child.read_fd = childRD_parentWR[1];
        new_child.write_fd = childWR_parentRD[0];
        strncpy(new_child.name, child_name, MAX_FRIEND_NAME_LEN);
        new_child.value = child_value;

        // Assuming 'friends' array is used to store child nodes in order
        if (next_empty_pos < MAX_CHILDREN) {
            friends[*next_empty_pos++] = new_child;
        } else {
            fprintf(stderr, "Error: Maximum number of child nodes reached.\n");
            close(childWR_parentRD[0]);
            close(childRD_parentWR[1]);
            return -1;
        }

        // Print success messages
        if (strcmp(parent_name, "Not_Tako") == 0) {
            print_direct_meet(child_name);
        } else {
            print_indirect_meet(parent_name, child_name);
        }

        return 0; // Success
    }
}

/* terminate child pseudo code
void clean_child(){
    close(child read_fd);
    close(child write_fd);
    call wait() or waitpid() to reap child; // this is blocking
}

*/

/* remember read and write may not be fully transmitted in HW1?
void fully_write(int write_fd, void *write_buf, int write_len);

void fully_read(int read_fd, void *read_buf, int read_len);

please do above 2 functions to save some time
*/

int main(int argc, char *argv[]) {
    char line[MAX_CMD_LEN];        // Buffer to hold each line read from stdin
    char child_name[MAX_FRIEND_NAME_LEN];
    int child_value;
    int pid;
    // will implement a circular queue
    friend friends[8] = {0};
    int start_pos; // for circular queue
    int next_empty_pos; // for circular queue

    FILE* read_fp;
    FILE* write_fp;

    pid_t process_pid = getpid(); // you might need this when using fork()
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [current_info]\n");
        return 0;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // fflush() prevent buffered I/O, equivalent to fflush() after each stdout, study this as you may need to do it for other friends against their parents
    
    // put argument one into current_info
    strncpy(current_info, argv[1], MAX_FRIEND_INFO_LEN); //argv[1] 存在 friend info
    
    //Identification Confirmation
    if(strcmp(argv[1], root) == 0){
        // is Not_Tako
        strncpy(current_name, current_info, MAX_FRIEND_NAME_LEN);      // put name into friend_nae
        current_name[MAX_FRIEND_NAME_LEN - 1] = '\0';        // in case strcmp messes with you
        read_fp = stdin;        // takes commands from stdin
        friend_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
        // is other friends
        // extract name and value from info
        meet_child_parsor(argv[1], child_name, &child_value);
        strncpy(current_name, child_name, MAX_FRIEND_NAME_LEN);
        read_fp = fdopen(PARENT_READ_FD, "r"); // #define PARENT_READ_FD 3
        fcntl(PARENT_READ_FD, F_SETFD, FD_CLOEXEC);// cloexec保證在執行exec時會關掉
        write_fp = fdopen(PARENT_WRITE_FD, "w"); // #define PARENT_WRITE_FD 4
        fcntl(PARENT_WRITE_FD, F_SETFD, FD_CLOEXEC);

        friend_value = child_value;

        if(write(PARENT_READ_FD, child_process_ok_msg, strlen(child_process_ok_msg)) != strlen(child_process_ok_msg)){
            perror("Child process write to the pipe fail.\n");
        }

    }

    // Read each line from STDIN until the end of input
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char task_type[35] = {0};    // To hold the task type
        char argmnt1[15] = {0};      // To hold the first argument
        char argmnt2[15] = {0};      // To hold the second argument
        int item_read;
        //char meet_child_name[MAX_FRIEND_NAME_LEN] = {0};
        //int meet_child_value;

        // Determine which task is assigned based on task_type
        int task_no = task_parsor(line, task_type, argmnt1, argmnt2, &item_read);

        // Handle each task based on task_no and number of arguments read
        if (task_no == 41 && item_read == 3) { // Meet
            //printf("Executing 'Meet' with arguments: %s, %s\n", argmnt1, argmnt2);

            if (Meet(argmnt1, friends, &next_empty_pos, argmnt2) != 0) {
                print_fail_meet(argmnt1, child_name);
            } else {
                fprintf(stderr, "Error parsing child name and value.\n");
            }

            
        } else if (task_no == 42 && item_read == 2) { // Check
            //printf("Executing 'Check' with argument: %s\n", argmnt1);
            
        } else if (task_no == 43 && item_read == 3) { // Adopt
            //printf("Executing 'Adopt' with arguments: %s, %s\n", argmnt1, argmnt2);
            
        } else if (task_no == 44 && item_read == 2) { // Graduate
            //printf("Executing 'Graduate' with argument: %s\n", argmnt1);
            
        } else if (task_no == 45 && item_read == 3) { // Compare
            //printf("Executing 'Compare' with arguments: %s, %s\n", argmnt1, argmnt2);
            
        } else {
            //printf("Invalid task or incorrect number of arguments: %s", line);
        }
    }

    //TODO:
    /* you may follow SOP if you wish, but it is not guaranteed to consider every possible outcome

    1. read from parent/stdin
    2. determine what the command is (Meet, Check, Adopt, Graduate, Compare(bonus)), I recommend using strcmp() and/or char check
    3. find out who should execute the command (extract information received)
    4. execute the command or tell the requested friend to execute the command
        4.1 command passing may be required here
    5. after previous command is done, repeat step 1.
    */

    // Hint: do not return before receiving the command "Graduate"
    // please keep in mind that every process runs this exact same program, so think of all the possible cases and implement them

    /* pseudo code
    if(Meet){
        create array[2]
        make pipe
        use fork.
            Hint: remember to fully understand how fork works, what it copies or doesn't
        check if you are parent or child
        as parent or child, think about what you do next.
            Hint: child needs to run this program again
    }
    else if(Check){
        obtain the info of this subtree, what are their info?
        distribute the info into levels 1 to 7 (refer to Additional Specifications: subtree level <= 7)
        use above distribution to print out level by level
            Q: why do above? can you make each process print itself?
            Hint: we can only print line by line, is DFS or BFS better in this case?
    }
    else if(Graduate){
        perform Check
        terminate the entire subtree
            Q1: what resources have to be cleaned up and why?
            Hint: Check pseudo code above
            Q2: what commands needs to be executed? what are their orders to avoid deadlock or infinite blocking?
            A: (tell child to die, reap child, tell parent you're dead, return (die))
    }
    else if(Adopt){
        remember to make fifo
        obtain the info of child node subtree, what are their info?
            Q: look at the info you got, how do you know where they are in the subtree?
            Hint: Think about how to recreate the subtree to design your info format
        A. terminate the entire child node subtree
        B. send the info through FIFO to parent node
            Q: why FIFO? will usin pipe here work? why of why not?
            Hint: Think about time efficiency, and message length
        C. parent node recreate the child node subtree with the obtained info
            Q: which of A, B and C should be done first? does parent child position in the tree matter?
            Hint: when does blocking occur when using FIFO?(mkfifo, open, read, write, unlink)
        please remember to mod the values of the subtree, you may use bruteforce methods to do this part (I did)
        also remember to print the output
    }
    else if(full_cmd[1] == 'o'){
        Bonus has no hints :D
    }
    else{
        there's an error, we only have valid commmands in the test cases
        fprintf(stderr, "%s received error input : %s\n", current_name, full_cmd); // use this to print out what you received
    }
    */
   

   // final print, please leave this in, it may bepart of the test case output
    if(is_Not_Tako()){
        print_final_graduate();
    }
    return 0;
}