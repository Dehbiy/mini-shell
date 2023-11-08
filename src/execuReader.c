#include "readcmd.h"
#include <unistd.h>

void execuReader(struct cmdline l){
    pid_t pid = fork();
    if(pid<0) {
        printf("fork error");
        return;
    }
    else if (pid==0) {
        execvp(l.seq[0][0],l.seq[0]);
    }
}