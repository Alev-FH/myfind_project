#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    pid_t pid;
    pid = fork();
    int error = 0;

    if (pid == -1) {
        return EXIT_FAILURE;
    } else if (pid == 0) {
        printf("Child process: %d\n", getpid());
        if (argc != 1) {
            printf("arguments: ");
            for (int i = 0; i < argc; i++) {
                printf(" %s ", argv[i]);
            }
            printf("\n");
        }
        // Here we must implement the search.
    } else {
        printf("Parent process: %d\n", getpid());
        pid_t childpid;
        while ((childpid = waitpid(-1, NULL, WNOHANG))) {
            if ((childpid == -1) && (error != EINTR)) {
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}