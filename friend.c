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
static char check_findout_success_msg[26] = "Just check out the node.\n";
static char check_findout_fail_msg[24] = "Cannot find this node.\n";
static char check_has_output_msg[34] = "A node has printed out its name.\n";
static char check_no_output_msg[35] = "No node has printed out its name.\n";
static char graduate_terminate_msg[33] = "There is node to be terminated.\n";
static char graduate_NO_terminate_msg[36] = "There is NO node to be terminated.\n";
static char graduate_parentwait_msg[28] = "Tell parent to wait child.\n";

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
    if (strcmp(task_type, "Check_Print") ==0) return 46;
    if (strcmp(task_type, "Terminate") ==0) return 47;
    
    return -1; // Return -1 if task is not recognized
}
// Parses `argmnt2` into `meet_child_name` and `meet_child_value`
int friendinfo_parsor(char* friend_info, char* child_name, int* child_value) {
    char child_number[10] = {0};
    int token_read = sscanf(friend_info, "%[^_]_%s", child_name, child_number);
    
    if (token_read != 2) {
        return -1; // Parsing failed
    }

    *child_value = atoi(child_number); // Convert child_value string to integer
    return 0; // Success
}

int Meet(char* parent_name, friend* friends, int* next_empty_pos, char* argmnt2) {//argmnt2 is child_info now
    int childRD_parentWR[2] = {0};
    int childWR_parentRD[2] = {0};
    char child_name[MAX_FRIEND_NAME_LEN] = {0};
    int child_value = 0;

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
    const pid_t pid = fork(); // pid有自己的data type，不要用int
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
        friends[(*next_empty_pos)%MAX_QUEUE_SIZE] = new_child;
        (*next_empty_pos)++;
        return 0; // Success
    }
}

int check(char* argmnt1, friend* friends, int start_pos, int next_empty_pos){
    //這邊是處理print，被找到的那個node會發出print1~print7給底下的node
    //每經過一層child node會把print的數字減一。拿到print0的那個node會print
    //要判斷前面那個node有沒有印出去決定要不要印空格
    printf("%s\n", current_info); //被找到的那個人要印自己

    for(int j=0;j<=7;j++){
        int print_space = 0;
        for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
            char wr_buff[MAX_CMD_LEN]= {0};
            snprintf(wr_buff, sizeof(wr_buff), "Check_Print %d %d\n", j, print_space);
            if(write(friends[i].read_fd, wr_buff, strlen(wr_buff)) != strlen(wr_buff)){ // parent write to child node, child read，傳整行command下去
                perror("For loop write down to child fail.\n");
            }
            //write給child後馬上read，去avoid race condition
            char rd_buff[MAX_CMD_LEN] = {0};
            if(read(friends[i].write_fd, rd_buff, sizeof(rd_buff))<0){ //parent要等child write一個訊息給他
                perror("Read from child fail.\n");
            }
            if(strcmp(rd_buff, check_has_output_msg)==0 ){ //比較child 給的msg，node是不是已經印了
                print_space = 1;
            }
        }
        if(print_space == 1){ // 如果有印出，每行的最後要補\n
            printf("\n");
        }    
    }
    return 0;
}

int terminate_msg_release(const char* argmnt1, friend* friends, int* start_pos, int* next_empty_pos){// 結束那個node
    // 找到那個node之後，要跟其下的children node散布需要terminate的消息
    for (int i = (*start_pos)%MAX_QUEUE_SIZE ; i != ((*next_empty_pos)%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
        char wr_buff[MAX_CMD_LEN]= "Terminate\n";
        if(write(friends[i].read_fd, wr_buff, strlen(wr_buff)) != strlen(wr_buff)){ // parent write to child node, child read，傳整行command下去
            perror("For loop write down to child fail.\n");
        }
        //write給child後馬上read，去avoid race condition
        char rd_buff[MAX_CMD_LEN] = {0};
        if(read(friends[i].write_fd, rd_buff, sizeof(rd_buff))<0){ //parent要等child write一個訊息給他
            perror("Read from child fail.\n");
        }
        if(strcmp(rd_buff, graduate_parentwait_msg)==0 ){ //如果收到的msg是graduate_parentwait_msg的話，代表這個child接下來要terminate了，那麼要parenr wait child process
            int status = 0;
            if(waitpid(friends[i].pid, &status, 0) == -1){ // waitpid成功會回傳child 的pid
                perror("Waitpid fail.\n");
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char line[MAX_CMD_LEN] = {0};        // Buffer to hold each line read from stdin
    // will implement a circular queue
    friend friends[MAX_QUEUE_SIZE] = {0};
    int start_pos = 0; // for circular queue
    int next_empty_pos = 0; // for circular queue

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
        int item_read = 0;
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
                for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
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
        } 
        if (task_no == 42 && item_read == 2 || ( is_Root() && task_no == 44 && item_read == 2)) { // Current process 是 root; Graduate要先執行一次Check
            //printf("Executing 'Check' with argument: %s\n", argmnt1);
            if (strcmp(current_name, argmnt1) == 0 ){ // 現在這個process如果就是argument1（除了root以外），要將成功作check的訊息傳上去，否則將訊息傳下去
                if (check(argmnt1, friends, start_pos, next_empty_pos)==0) { // if check success, 那就往上傳消息
                    if(!is_Root()){ // 除了root以外，都要將成功作check的訊息傳上去
                        if(write(WRITE_TO_PARENT_FD ,check_findout_success_msg, strlen(check_findout_success_msg)) != strlen(check_findout_success_msg)){
                            perror("Write check_findout_success_msg to parent fail.\n");
                        }
                    } 
                } else { // Check Function Fail: check return -1
                    perror("Check Fail: Check return -1.\n"); // “不是”沒有這個node存在，而是check失敗
                }
            } else { // strcmp(current_name, argmnt1) != 0; argment1: parent_friend_name
                // 這邊要做DFS，訊息要傳下去到circular queue裡的child node
                bool Is_success = false;
                for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
                    char newbuf[MAX_CMD_LEN] = {0};
                    snprintf(newbuf, sizeof(newbuf), "Check %s\n", argmnt1);
                    if(write(friends[i].read_fd, newbuf, strlen(newbuf)) != strlen(newbuf)){ // parent write to child node, child read，傳整行command下去
                        perror("For loop write down to child fail.\n");
                    }
                    char buf[MAX_CMD_LEN] = {0};
                    if(read(friends[i].write_fd, buf, sizeof(buf)) <0){ //parent要等child write一個訊息給他
                        perror("Read from child fail.\n");
                    } 
                    if(strcmp(buf, check_findout_success_msg)==0){ 
                        if(!is_Root()){ //除了root以外要傳訊息上去
                            if(write(WRITE_TO_PARENT_FD,check_findout_success_msg, strlen(check_findout_success_msg)) != strlen(check_findout_success_msg)){
                                perror("Write check_findout_success_msg to parent fail.\n");
                            }
                        } 
                        Is_success = true;
                        break; //某個subtree的node已經處理完check，就不用再找下去
                    }
                } 
                // current process的child 都沒有該process
                // 給一個boolean
                if(!Is_success){
                    if(!is_Root()){
                        if(write(WRITE_TO_PARENT_FD, check_findout_fail_msg, strlen(check_findout_fail_msg)) != strlen(check_findout_fail_msg)){
                            perror("Write check_findout_fail_msg to parent fail.\n");
                        }
                    } else {
                        print_fail_check(argmnt1);
                        if(task_no == 44 && item_read == 2){ //如果沒有node，就不用去刪subtree
                            continue;
                        }
                    }
                }
            }
        } 
        
        if(task_no == 46 && item_read == 3) { 
            //這是Check_Print的command. argment1 是0~7 (print0~print7), argument2 是 1 or 0(有沒有要印space)
            int arg1 = atoi(argmnt1);
            int arg2 = atoi(argmnt2); //print_space

            if(arg1 == 0){
                if(arg2 == 1){
                    printf(" ");
                }
                printf("%s",current_info);
                if(write(WRITE_TO_PARENT_FD, check_has_output_msg, strlen(check_has_output_msg))<0){ //Line275
                    perror("Write to parent check_has_output_msg fail.\n");
                }
            } else { // argmnt1 != 0
                for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
                    char buffer[MAX_CMD_LEN] = {0};
                    snprintf(buffer, sizeof(buffer), "Check_Print %d %d\n", arg1 - 1, arg2);
                    if(write(friends[i].read_fd, buffer, strlen(buffer)) != strlen(buffer)){ // parent write to child node, child read，傳整行command下去
                        perror("Check:For loop write down to child fail.\n");
                    }
                    char buf[MAX_CMD_LEN] = {0};
                    if(read(friends[i].write_fd, buf, sizeof(buf)) <0){ // 去read descendent有沒有印出了
                        //parent要等child write一個訊息給他
                        perror("Read from child fail.\n");
                    } 

                    if(strcmp(buf, check_has_output_msg) ==0) {//統整
                        arg2 = 1;//print_space = 1
                    } 
                }
                //所有的child處理完後，去確定前面所有跑過的node（“不一定是其下的subtree，而是前面跑過已經變黑的那個node的結果的累積")是不是已經有output了
                if(arg2 == 1){
                    if(write(WRITE_TO_PARENT_FD, check_has_output_msg, strlen(check_has_output_msg))<0){
                        perror("Write to parent check_has_output_msg fail.\n");
                    }
                } else {
                    if(write(WRITE_TO_PARENT_FD, check_no_output_msg, strlen(check_no_output_msg))<0){
                        perror("Write to parent check_no_output_msg fail.\n");
                    }
                }
            }
        } 
        
        if (task_no == 43 && item_read == 3) { // Adopt
            //printf("Executing 'Adopt' with arguments: %s, %s\n", argmnt1, argmnt2);

        } 
        
        if (task_no == 44 && item_read == 2) { // Graduate
            // printf("[%s] Executing 'Graduate' with argument: %s\n", current_info , argmnt1);
            // 已經在上面執行了一次check
            //******** GRADUATE ********//
            // 用DFS的方式，由上往下找node，找到了之後結束那個subtree
            if (strcmp(current_name, argmnt1) == 0 ){ // 現在這個process如果就是argument1（除了root以外），要DFS將訊息先傳到最底部
                // 現在這個node就是要找的那個node，要繼續DFS到最底部，再一路拆上來
                if (terminate_msg_release(argmnt1, friends, &start_pos, &next_empty_pos)==0) { // if terminate_node success, 那就往上傳消息
                    if(!is_Root()){ // 除了root以外，都要將成功作terminate_node的訊息傳上去
                        if(write(WRITE_TO_PARENT_FD ,graduate_parentwait_msg, strlen(graduate_parentwait_msg)) != strlen(graduate_parentwait_msg)){
                            perror("Write graduate_parentwait_msg to parent fail.\n");
                        }
                        exit(0); //這邊是exit本人
                    } else {
                        print_final_graduate();
                        break;
                    }
                } else { // terminate_msg_release Function Fail: terminate_msg_release return -1
                    perror("terminate_msg_release Fail: terminate_msg_release return -1.\n"); // “不是”沒有這個node存在，而是terminate_node失敗
                }
            } else {
                // 還沒找到那個node，繼續往下找
                // strcmp(current_name, argmnt1) != 0; argment1: parent_friend_name
                // 這邊要做DFS，訊息要傳下去到circular queue裡的child node
                bool Is_success = false;
                for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
                    if(write(friends[i].read_fd, line, strlen(line)) != strlen(line)){ // parent write to child node, child read，傳整行command下去
                        fprintf(stderr, "current info: [%s][%s]\n", current_info, friends[i].info);
                        perror("Graduate_else:For loop write down to child fail.\n");
                    }
                    char buf[MAX_CMD_LEN] = {0};
                    if(read(friends[i].write_fd, buf, sizeof(buf)) <0){ //parent要等child write一個訊息給他
                        perror("Read from child fail.\n");
                    }
                    if(strcmp(buf, graduate_terminate_msg)==0){ 
                        if(!is_Root()){ //除了root以外要傳訊息上去
                            if(write(WRITE_TO_PARENT_FD,graduate_terminate_msg, strlen(graduate_terminate_msg)) != strlen(graduate_terminate_msg)){
                                perror("Write graduate_terminate_msg to parent fail.\n");
                            }
                        } 
                        Is_success = true;
                        break; //某個subtree的node已經處理完check，就不用再找下去
                    } else if(strcmp(buf, graduate_parentwait_msg)==0) {
                        int status = 0;
                        if(waitpid(friends[i].pid, &status, 0) == -1){ // waitpid成功會回傳child 的pid
                            perror("Waitpid fail.\n");
                        }

                        close(friends[i].read_fd);
                        close(friends[i].write_fd);
                        
                        for (int t = i; t != (next_empty_pos%MAX_QUEUE_SIZE); t = (t + 1) % MAX_QUEUE_SIZE) {
                            friends[t] = friends[(t + 1) % MAX_QUEUE_SIZE];
                        }
                        next_empty_pos = (next_empty_pos - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
                    }
                } 
                // current process的child 都沒有該process
                // 給一個boolean
                if(!Is_success){
                    if(!is_Root()){
                        if(write(WRITE_TO_PARENT_FD, graduate_NO_terminate_msg, strlen(graduate_NO_terminate_msg)) != strlen(graduate_NO_terminate_msg)){
                            perror("Write graduate_NO_terminate_msg to parent fail.\n");
                        }
                    }
                }
                
            }       
        } 
        
        if (task_no == 47 && item_read ==1){ // Graduate Terminate_node
            //printf("Executing 'task_no == 47' with argument: %s\n", argmnt1);
            for (int i = start_pos%MAX_QUEUE_SIZE ; i != (next_empty_pos%MAX_QUEUE_SIZE) ; i = (i + 1) % MAX_QUEUE_SIZE){
                char buffer[MAX_CMD_LEN] = "Terminate\n";
                if(write(friends[i].read_fd, buffer, strlen(buffer)) != strlen(buffer)){ // parent write to child node, child read，傳整行command下去
                    perror("Graduate_Terminate:For loop write down to child fail.\n");
                }
                char buf[MAX_CMD_LEN] = {0};
                if(read(friends[i].write_fd, buf, sizeof(buf)) <0){ // 去read descendent有沒有印出了
                    //parent要等child write一個訊息給他
                    perror("Read from child fail.\n");
                } 

                if(strcmp(buf, graduate_parentwait_msg)==0 ){ //如果收到的msg是graduate_parentwait_msg的話，那麼要wait child process
                    int status = 0;
                    if(waitpid(friends[i].pid, &status, 0) == -1){ // waitpid成功會回傳child 的pid
                        perror("Waitpid fail.\n");
                    }
                }
            }
            // WRITE_TO_PARENT_FD 激活parent，讓parent從583行開始跑，跑到waitpid
            if(write(WRITE_TO_PARENT_FD, graduate_parentwait_msg, strlen(graduate_parentwait_msg))<0){
                perror("Write to parent graduate_parentwait_msg fail.\n");
            }
            exit(0);
        } 
        
        if (task_no == 45 && item_read == 3) { // Compare
            //printf("Executing 'Compare' with arguments: %s, %s\n", argmnt1, argmnt2);
            
        } 

        // if(task_no not included in above numbers){
        //     //printf("Invalid task or incorrect number of arguments: %s", line);
        // }
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