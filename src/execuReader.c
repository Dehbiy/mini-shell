#include "readcmd.h"
//#include <ctime>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdbool.h"


struct Cell{
    char ** commande;
    pid_t pid;
    struct Cell* next; 
};

struct Cell * BG_PROCESSES = NULL;

void addBgProcess(pid_t pid, char** commande){
    struct Cell *process = malloc(sizeof(struct Cell));

    if(process == NULL){
        printf("Failed to allocate memory for the new process\n");
        exit(EXIT_FAILURE);
    }
    process->pid = pid;
    process->commande = commande;
    process->next = BG_PROCESSES;
    BG_PROCESSES = process;

}

void jobs(){
    struct Cell* current = BG_PROCESSES;
    while(current != NULL){
        int indx_sub_commande = 0;
        printf("[%i]: ", current->pid);
        while (current->commande[indx_sub_commande] != NULL)
        {
            printf("%s ", current->commande[indx_sub_commande]);
            indx_sub_commande++;
        }
        printf("\n");
        
        current = current->next;
    }

}

void signalHandler(int signal){
    printf("Recieved SIGCHILD signal\n");
}

void freeBgProcess(){
    
    if(BG_PROCESSES == NULL) return;

    struct Cell* current = BG_PROCESSES;
    struct Cell* next = current->next;
    
    while(next != NULL){
        int indx_sub_commande = 0;
        while (current->commande[indx_sub_commande] != NULL)
        {
            free(current->commande[indx_sub_commande]);
            indx_sub_commande++;
        }
        free(current->commande);
        free(current);
        current = next;
        next = next->next;
    }
    int indx_sub_commande = 0;
    while (current->commande[indx_sub_commande] != NULL){
            free(current->commande[indx_sub_commande]);
            indx_sub_commande++;
        }
    free(current->commande);
    free(current);
}

void EXEcute(struct cmdline* l, pid_t pid, int num_seq) {
    if(strcmp(l->seq[num_seq][0], "jobs") == 0 && l->seq[num_seq][1] == NULL){
        jobs();
        exit(0);
    }
    execvp(l->seq[num_seq][0],l->seq[num_seq]);
    printf(" %s: Command not found\n", l->seq[num_seq][0]);
    printf("try: sudo apt install %s\n", l->seq[num_seq][0]);
    exit(0);
}

void setInput(struct cmdline* l){
    if(l->in){
        int fileDescriptor = open(l->in, O_RDONLY);
        dup2(fileDescriptor, STDIN_FILENO);
    }

}


void setOutput(struct cmdline* l){
    if(l->out){
        int fileDescriptor = open(l->out, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
        dup2(fileDescriptor, STDOUT_FILENO);
    }
}


void changeSTDIN(struct cmdline* l,int * pipedes, bool shouldSetOutput){
    if(shouldSetOutput){
        setOutput(l);
    }
    dup2(pipedes[0],STDIN_FILENO);
    //close(pipedes[1]);
    //close(pipedes[0]);
}


void changeSTDOUT(struct cmdline* l,int * pipedes, bool shouldSetInput){
    if(shouldSetInput){
        setInput(l);
    }
    dup2(pipedes[1],STDOUT_FILENO);
    //close(pipedes[0]);
    //close(pipedes[1]);
}


void execuReader(struct cmdline* l){

    signal(SIGCHLD, signalHandler);
    int status;
    pid_t pid = fork();
    
    if(pid<0) {
        printf("fork error");
        return;
    }
    if (pid==0) {
        if (l->seq[1] != NULL) {

            int pipe_fd1[2]; // Descripteurs de fichier du pipe
            int pipe_fd2[2];
            
            if (pipe(pipe_fd1) == -1 || pipe(pipe_fd2) == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            pid_t pidCmd = fork();
            if (pidCmd < 0) {
                printf("fork error");
                return;
            }

            if (pidCmd == 0) {
                bool shouldSetOutput = l->seq[2] == NULL;
                changeSTDIN(l, pipe_fd1, shouldSetOutput);

                if(l->seq[2] != NULL){
                    changeSTDOUT(l, pipe_fd2, false);
                }

                EXEcute(l, pidCmd, 1);
            }
            else {
                changeSTDOUT(l, pipe_fd1, true);
                EXEcute(l,pidCmd,0);
            }
            waitpid(-1, NULL, 0);

        

            
            int num_seq;
            if(l->seq[2] != NULL){
                for(num_seq = 2; l->seq[num_seq + 1] && l->seq[num_seq + 2] != NULL; num_seq+=2){
                    if (pipe(pipe_fd1) == -1){
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }
                    pid_t pidCmd = fork();
                    if (pidCmd < 0) {
                        printf("fork error");
                        return;
                    }
                    if (pidCmd == 0) {
                        changeSTDIN(l, pipe_fd1, false);
                        changeSTDOUT(l, pipe_fd2, false);
                        EXEcute(l, pidCmd, num_seq + 1);
                    }
                    else {
                        changeSTDIN(l, pipe_fd2, false);
                        changeSTDOUT(l, pipe_fd1, false);
                        EXEcute(l,pidCmd,num_seq);
                    }
                }


                if(l->seq[num_seq + 1] == NULL){
                    pid_t pidCmd = fork();
                    if (pidCmd < 0) {
                        printf("fork error");
                        return;
                    }
                    if (pidCmd == 0) {
                        exit(0);
                    }
                    else {
                        changeSTDIN(l, pipe_fd2, true);
                        EXEcute(l,pidCmd,num_seq);
                    }
                }
                else{
                    pid_t pidCmd = fork();
                    if (pidCmd < 0) {
                        printf("fork error");
                        return;
                    }
                    if (pidCmd == 0) {
                        changeSTDIN(l, pipe_fd1, true);
                        EXEcute(l, pidCmd, num_seq + 1);
                    }
                    else {
                        changeSTDIN(l, pipe_fd2, false);
                        changeSTDOUT(l, pipe_fd1, false);
                        EXEcute(l,pidCmd,num_seq);
                    }
                }      

            }

            close(pipe_fd1[0]);
            close(pipe_fd1[1]);
            close(pipe_fd2[0]);
            close(pipe_fd2[1]);




        }
        else {
            setInput(l);
            setOutput(l);
            EXEcute(l,pid,0);   
        }
    }

    if(!l->bg){
        waitpid(pid, &status, 0);
        return;
    }
    int size_seq = 0;
    while (l->seq[0][size_seq]!=NULL)
    {
        size_seq++;
    }

    char ** commande = malloc((size_seq+1)*sizeof(char *));
    if(commande == NULL){
        exit(1);
    }
    for (int i=0;i<=size_seq;i++) {
        commande[i] = malloc(20*sizeof(char));
        if(commande[i] == NULL){
            exit(1);
        }
        if (l->seq[0][i]== NULL) {
            free(commande[i]);
            commande[i] = NULL;
            break;
        }
        strcpy(commande[i], l->seq[0][i]);
    }
    addBgProcess(pid, commande);
    printf("process %i is running is the background\n", pid);

}