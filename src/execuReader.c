#include "readcmd.h"
//#include <ctime>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


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
    freeBgProcess();
    kill(pid,SIGTERM);
}
void execuReader(struct cmdline* l){

    int status;
    pid_t pid = fork();
    if(pid<0) {
        printf("fork error");
        return;
    }
    if (pid==0) {
        if (l->seq[1] != NULL) {
            int pipe_fd[2]; // Descripteurs de fichier du pipe

            // Créer un pipe
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            pid_t pidCmd = fork();
            if (pidCmd < 0) {
                printf("fork error");
                return;
            }
            if (pidCmd == 0) {
                dup2(pipe_fd[0],STDIN_FILENO);
                close(pipe_fd[1]);
                close(pipe_fd[0]);
                EXEcute(l,pidCmd,1);
            }
            else {
                dup2(pipe_fd[1],STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                EXEcute(l,pidCmd,0);
            }
        }
        else EXEcute(l,pid,0);        
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