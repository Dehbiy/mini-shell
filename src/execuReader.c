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
    char * commande;
    pid_t pid;
    struct Cell* next; 
};

struct Cell * BG_PROCESSES = NULL;

void addBgProcess(pid_t pid, char* commande){
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
        printf("[%i]: %s\n", current->pid, current->commande);
        current = current->next;
    }

}

void freeBgProcess(){
    
    if(BG_PROCESSES == NULL) return;

    struct Cell* current = BG_PROCESSES;
    struct Cell* next = current->next;
    
    while(next != NULL){
        free(current->commande);
        free(current);
        current = next;
        next = next->next;
    }

    free(current);
}


void execuReader(struct cmdline* l){
    int status;
    pid_t pid = fork();
    if(pid<0) {
        printf("fork error");
        return;
    }
    if (pid==0) {
        if(strcmp(l->seq[0][0], "jobs") == 0 && l->seq[0][1] == NULL){
            jobs();
            exit(0);
        }
        execvp(l->seq[0][0],l->seq[0]);
        printf(" %s: Command not found\n", l->seq[0][0]);
        printf("try: sudo apt install %s\n", l->seq[0][0]);
        freeBgProcess();
        kill(pid,SIGTERM);
        return;
        
    }
    if(!l->bg){
        waitpid(pid, &status, 0);
        return;
    }
    
    char * commande = malloc(20*sizeof(char));
    
    if(commande == NULL){
        exit(1);
    }
    strcpy(commande, l->seq[0][0]);
    addBgProcess(pid, commande);
    
    printf("process %i is running is the background\n", pid);

}