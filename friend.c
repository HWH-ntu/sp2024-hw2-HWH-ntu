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
static char friend_info[MAX_FRIEND_INFO_LEN];   // current process info
static char friend_name[MAX_FRIEND_NAME_LEN];   // current process name
static int friend_value;    // current process value
FILE* read_fp = NULL;

// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(friend_name, root) == 0);
}

// a bunch of prints for you
void print_direct_meet(char *friend_name) {
    fprintf(stdout, "Not_Tako has met %s by himself\n", friend_name);
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

void print_compare_gtr(char *friend_name){
    fprintf(stdout, "Not_Tako is still friends with %s\n", friend_name);
}

void print_compare_leq(char *friend_name){
    fprintf(stdout, "%s is dead to Not_Tako\n", friend_name);
}

void print_final_graduate(){
    fprintf(stdout, "Congratulations! You've finished Not_Tako's annoying tasks!\n");
}

int task_parsor(char* line, char* task_type, char* argmnt1, char* argmnt2, int* item_read) {
    /*
    41: Implementation 4.1 Meet <parent_friend_name> <child_friend_info>
    42: Implementation 4.2 Check <parent_friend_name>
    43: Implementation 4.3 Adopt <parent_friend_name> <child_friend_name>
    44: Implementation 4.4 Graduate <friend_name>
    45: Implementation 4.5 Compare <friend_name> <number>
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
    char line[100];        // Buffer to hold each line read from stdin
    char task_type[35];    // To hold the task type
    char argmnt1[15];      // To hold the first argument
    char argmnt2[15];      // To hold the second argument
    int item_read;
    char meet_child_name[MAX_FRIEND_NAME_LEN];
    int meet_child_value;

    // Read each line from STDIN until the end of input
    while (fgets(line, sizeof(line), stdin) != NULL) {
        // Clear previous arguments
        memset(task_type, 0, sizeof(task_type));
        memset(argmnt1, 0, sizeof(argmnt1));
        memset(argmnt2, 0, sizeof(argmnt2));

        // Determine which task is assigned based on task_type
        int task_no = task_parsor(line, task_type, argmnt1, argmnt2, &item_read);

        // Handle each task based on task_no and number of arguments read
        if (task_no == 41 && item_read == 3) { // Meet
            //printf("Executing 'Meet' with arguments: %s, %s\n", argmnt1, argmnt2);
            // Call the parser to split `argmnt2` into `meet_child_name` and `meet_child_value`
            // if (meet_child_parsor(argmnt2, meet_child_name, &meet_child_value) == 0) {
            //     printf("Child Name: %s, Child Value: %d\n", meet_child_name, meet_child_value);
            // } else {
            //     printf("Error parsing child name and value.\n");
            // }
            
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

    pid_t process_pid = getpid(); // you might need this when using fork()
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [friend_info]\n");
        return 0;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // fflush() prevent buffered I/O, equivalent to fflush() after each stdout, study this as you may need to do it for other friends against their parents
    

    

    // put argument one into friend_info
    strncpy(friend_info, argv[1], MAX_FRIEND_INFO_LEN); //argv[1] 存在 friend info
    
    if(strcmp(argv[1], root) == 0){
        // is Not_Tako
        strncpy(friend_name, friend_info,MAX_FRIEND_NAME_LEN);      // put name into friend_nae
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';        // in case strcmp messes with you
        read_fp = stdin;        // takes commands from stdin
        friend_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
        // is other friends
        // extract name and value from info
        // where do you read from?
        // anything else you have to do before you start taking commands?
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
        fprintf(stderr, "%s received error input : %s\n", friend_name, full_cmd); // use this to print out what you received
    }
    */
   

   // final print, please leave this in, it may bepart of the test case output
    if(is_Not_Tako()){
        print_final_graduate();
    }
    return 0;
}