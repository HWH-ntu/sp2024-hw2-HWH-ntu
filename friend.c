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
static int current_value;    // current process value
//FILE* read_fp = NULL;
static char child_process_ok_msg[43] = "Child writes to parent: Child process OK!\n";
static char meet_success_msg[29] = "Meet is successfully done!\n";
static char meet_fail_msg[13]= "Meet fail.\n";

// Is Root of tree
static inline bool is_Root() {
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

#include <string.h>

void normalize_newline(char *line) {
    size_t len = strlen(line);

    // Check if the line ends with "\r\n"
    if (len >= 2 && line[len - 2] == '\r' && line[len - 1] == '\n') {
        line[len - 2] = '\n';   // Replace '\r' with '\n'
        line[len - 1] = '\0';   // Null-terminate after '\n'
    }
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
    normalize_newline(line); // Remove \r if neccessary
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
int friendinfo_parsor(char* friend_info, char* child_name, int* child_value) {
    char child_number[10];
    int token_read = sscanf(friend_info, "%[^_]_%s", child_name, child_number);
    
    if (token_read != 2) {
        return -1; // Parsing failed
    }

    *child_value = atoi(child_number); // Convert child_value string to integer
    return 0; // Success
}

int Meet(char* parent_name, friend* friends, int* next_empty_pos, char* argmnt2) {//argmnt2 is child_info now
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
    const pid_t pid = fork();
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

        if (dup2(childWR_parentRD[1],(TEMP_HIGHER_FD+WRITE_TO_PARENT_FD))==-1){
            perror("dup2 failed for TEMP_HIGHER_FD+WRITE_TO_PARENT_FD");
            _exit(1);
        }
        if (dup2(childRD_parentWR[0],(TEMP_HIGHER_FD+READ_FROM_PARENT_FD))==-1){
            perror("dup2 failed for TEMP_HIGHER_FD+READ_FROM_PARENT_FD");
            _exit(1);
        }

        close(childWR_parentRD[1]);
        close(childRD_parentWR[0]);

        if(dup2((TEMP_HIGHER_FD+WRITE_TO_PARENT_FD), WRITE_TO_PARENT_FD) == -1){
            perror("dup2 failed for WRITE_TO_PARENT_FD");
            _exit(1);
        }
        if(dup2((TEMP_HIGHER_FD+READ_FROM_PARENT_FD), READ_FROM_PARENT_FD) == -1){
            perror("dup2 failed for READ_FROM_PARENT_FD");
            _exit(1);
        }

        close(TEMP_HIGHER_FD+WRITE_TO_PARENT_FD);
        close(TEMP_HIGHER_FD+READ_FROM_PARENT_FD);

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
        if(read(childWR_parentRD[0], buf, sizeof(buf)) <=0 ){//read 沒有從pipe讀到東西
            int status = 0;
            if(waitpid(pid, &status, 0) == -1){
                perror("Wait fail: Parent wait for the child fail.\n");
            }
        } else if (strcmp(buf, child_process_ok_msg)!=0) {
            fprintf(stderr,"%s\n",buf);
            fflush(stderr); // 提早flush才可以先看到這個msg，而不是最後internal buffer滿的時候才印出來
            perror("Child didn't reply with correct msg: child_process_ok_msg.\n");
        }
        // Call the parser to split `argmnt2` into `meet_child_name` and `meet_child_value`
        if (friendinfo_parsor(argmnt2, child_name, &child_value) == 0) {
            //printf("Child Name: %s, Child Value: %d\n", child_name, child_value);
        } else {
            fprintf(stderr, "Error parsing child name and value.\n");
        }

        // Store child information in the parent’s data structure (if needed)
        friend new_child;
        new_child.pid = pid;
        new_child.read_fd = childRD_parentWR[1];
        new_child.write_fd = childWR_parentRD[0];
        strncpy(new_child.name, child_name, sizeof(child_name));
        new_child.value = child_value;

        // Assuming 'friends' array is used to store child nodes in order
        friends[(*next_empty_pos)%MAX_CHILDREN] = new_child;
        (*next_empty_pos)++;
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
    int pid;
    // will implement a circular queue
    friend friends[8] = {0};
    int start_pos; // for circular queue
    int next_empty_pos; // for circular queue

    FILE* read_fp; // 這個process是要跟parent溝通的fd

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
        current_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
        // is other friends
        // extract name and value from info
        friendinfo_parsor(argv[1], current_name, &current_value);
        //"Child process不知道read_fp是個pipe"
        read_fp = fdopen(READ_FROM_PARENT_FD, "r"); // Child read from parent 把fd開成3 #define PARENT_READ_FD 3
        fcntl(READ_FROM_PARENT_FD, F_SETFD, FD_CLOEXEC);// cloexec保證在執行exec時會關掉
        
        //Child 寫給 parent表示，這個child process有建成功
        if(write(WRITE_TO_PARENT_FD, child_process_ok_msg, strlen(child_process_ok_msg)) != strlen(child_process_ok_msg)){
            perror("Child process write to the pipe fail.\n");
        }
    }

    // Read each line from STDIN until the end of input
    while (fgets(line, sizeof(line), read_fp) != NULL) { //靠pipe(read_fp)傳每一行command下去
        char task_type[MAX_CMD_LEN] = {0};    // To hold the task type
        char argmnt1[MAX_CMD_LEN] = {0};      // To hold the first argument
        char argmnt2[MAX_CMD_LEN] = {0};      // To hold the second argument
        int item_read;
        //char meet_child_name[MAX_FRIEND_NAME_LEN] = {0};
        //int meet_child_value;

        // Determine which task is assigned based on task_type
        int task_no = task_parsor(line, task_type, argmnt1, argmnt2, &item_read);

        // Handle each task based on task_no and number of arguments read
        if (task_no == 41 && item_read == 3) { // Meet
            char child_name[MAX_FRIEND_NAME_LEN] = {0};
            int child_value = 0;
            friendinfo_parsor(argmnt2, child_name, &child_value);
            //printf("Executing 'Meet' with arguments: %s, %s\n", argmnt1, argmnt2);
            if (strcmp(current_name, argmnt1) == 0 ){ // 現在這個process如果就是argument1（除了root以外），要將成功作meet的訊息傳上去，否則將訊息傳下去
                if (Meet(argmnt1, friends, &next_empty_pos, argmnt2) == 0) { // if meet success, 那就往上傳消息
                    if(!is_Root()){ // 除了root以外，都要將成功作meet的訊息傳上去
                        if(write(WRITE_TO_PARENT_FD ,meet_success_msg, strlen(meet_success_msg)) != strlen(meet_success_msg)){
                            perror("Write meet_success_msg to parent fail.\n");
                        }
                    } else { // root
                        print_direct_meet(child_name);
                    }
                } else { // Meet Function Fail: Meet return -1
                    perror("Meet Fail: Meet return -1.\n"); // “不是”沒有這個node存在，而是meet失敗
                }
            } else { // strcmp(current_name, argmnt1) != 0; argment1: parent_friend_name
                // 這邊要做DFS，訊息要傳下去到circular queue裡的child node
                bool Is_success = false;
                for (int i = start_pos%MAX_CHILDREN ; i != (next_empty_pos%MAX_CHILDREN) ; i = (i + 1) % MAX_CHILDREN){
                    if(write(friends[i].read_fd, line, strlen(line)) != strlen(line)){ // parent write to child node, child read，傳整行command下去
                        perror("For loop write down to child fail.\n");
                    }
                    char buf[MAX_CMD_LEN] = {0};
                    if(read(friends[i].write_fd, buf, sizeof(buf)) <0){ //parent要等child write一個訊息給他
                        perror("Read from child fail.\n");
                    } 
                    if(strcmp(buf, meet_success_msg)==0 ){ //比較child 給的msg，某個subtree的node有處理好meet了
                        if(!is_Root()){ //除了root以外要傳訊息上去
                            if(write(WRITE_TO_PARENT_FD,meet_success_msg, strlen(meet_success_msg)) != strlen(meet_success_msg)){
                                perror("Write meet_success_msg to parent fail.\n");
                            }
                        } else { //Root要處理成功的訊息
                            print_indirect_meet(argmnt1, child_name);
                        }
                        Is_success = true;
                        break; //某個subtree的node已經處理完meet，就不用再找下去
                    }
                } 
                // current process的child 都沒有該process
                // 給一個boolean
                if(!Is_success){
                    if(!is_Root()){
                        if(write(WRITE_TO_PARENT_FD, meet_fail_msg, strlen(meet_fail_msg)) != strlen(meet_fail_msg)){
                            perror("Write meet_fail_msg to parent fail.\n");
                        }
                    } else {
                        print_fail_meet(argmnt1, child_name);
                    }
                }
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
    // if(is_Root()){
    //     print_final_graduate();
    // }
    return 0;
}