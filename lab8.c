#include <stdio.h>
#include <stdlib.h>

int main(void) {
    pid_t pid;
    int status;

    for (int j = 0; j < 5; j++) {
        pid = fork();

        if (pid < 0) {
            perror("error fork");
            exit(-1);
        }
        else if (pid == 0) {
            for (int i = 0; i < 10; i++) {
                printf("pid = %d", getpid());
            }
            exit(j+1);
        }
    }

    while(pid = wait(&status) != -1) {
        if (WIFEXITED(status)) {
            printf("pid = %d, status = %d", pid, WEXITSTATUS(status));
        }
    }

    return 0;
}