#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct {
    char **files;         // The files extracted from the command
    char **options;       // The options extracted from the command
    int R_enabled;        // Wether or not the option -R is passed as a parameter 
    int i_enabled;        // Wether or not the option -R is passed as a parameter
    int num_of_files;     // The number of filenames extracted from the command
    int num_of_options;   // The number of options extracted from the command
} SearchParams;

static void parseArguments(int argc, char *argv[], SearchParams *sp);
static void print_usage(char *programm_name);
static void freeSearchParams(SearchParams *sp);

int main(int argc, char *argv[]) {

    SearchParams sp = { 0 };
    parseArguments(argc, argv, &sp);

    // Print the SearchParams variables to see if the parseArguments function initializes them corectly.
    printf("Recursive option        : %s\n", sp.R_enabled == 1 ? "true" : "false");
    printf("Case insensitive option : %s\n", sp.i_enabled == 1 ? "true" : "false");
    printf("num_of_files            : %d\n", sp.num_of_files);
    printf("num_of_options          : %d\n", sp.num_of_options);

    // Pint the files for testing. This will be removed.
    for (int i = 0; i < sp.num_of_files; i++) {
        printf("files: %s\n", sp.files[i]);
    }

    pid_t pid;
    int error = 0;

    // Here we must fork so many times as num_of_files. Every process should search for a file.
    for (int i = 0; i < sp.num_of_files; i++) {

        pid = fork();

        if (pid == -1) {
            return EXIT_FAILURE;
        } else if (pid == 0) {
            printf("Child process: %d\n", getpid());
            break;        // Break the loop when we are inside the child. We don't want the child to continue the for loop and fork again.
        } else {
            printf("Parent process: %d\n", getpid());
            pid_t childpid;
            while ((childpid = waitpid(-1, NULL, WNOHANG))) {
                if ((childpid == -1) && (error != EINTR)) {
                    break;
                }
            }
        }
    }

    freeSearchParams(&sp);

    return EXIT_SUCCESS;
}
/* Parses the argv array, initializing a SearchParams struct with the extracted values and other usefull informations. */
static void parseArguments(int argc, char *argv[], SearchParams *sp) {
    sp->files   = malloc(sizeof(double));
    sp->options = malloc(sizeof(double));

    // This is to help us increase the size of the pointers array
    unsigned int files_inc     = 1;
    unsigned int options_inc   = 1;

    // This is to help us index the pointers of the pointers array.
    unsigned int files_index   = 0;
    unsigned int options_index = 0;

    if (argc > 1) {

        for (int i = 1; i < argc; i++) {
            if (strncmp(argv[i], "-", 1) == 0) {

                // Check for specific flags
                if (strcmp(argv[i], "-R") == 0) {
                    sp->R_enabled = 1;
                } else if (strcmp(argv[i], "-i") == 0) {
                    sp->i_enabled = 1;
                } else {
                    printf("Option %s is not supported, it will be ignored!\n", argv[i]);
                    print_usage(argv[0]);
                    continue;
                }

                char **temp = realloc(sp->options, sizeof(double) * options_inc);        // Increase the size of the pointers array by +1 pointer
                if (temp == NULL) {
                    fprintf(stderr, "Failed to reallocate memory for options array!\n");
                    exit(-1);
                }
                sp->options = temp;
                sp->options[options_index] = strdup(argv[i]);         // Allocate memory and copy the option argument to the pointers array options_index position.

                options_inc++;
                options_index++;
                sp->num_of_options++;
            } else {
                char **temp = realloc(sp->files, sizeof(double) * files_inc);        // Increase the size of the pointers array by +1 pointer
                if (temp == NULL) {
                    fprintf(stderr, "Failed to reallocate memory for files array!\n");
                    exit(-1);
                }
                sp->files = temp;
                sp->files[files_index] = strdup(argv[i]);        // Allocate memory and copy the file argument to the pointers array files_index position.

                files_inc++;
                files_index++;
                sp->num_of_files++;
            }
        }
    }
}
static void print_usage(char *programm_name) {
    printf("Usage: %s [-R] [-i] [dateiname 1 dateiname n]\n\n", programm_name);
    return;
}
static void freeSearchParams(SearchParams *sp) {
    if (sp->num_of_files) {
        for (int i = 0; i < sp->num_of_files; i++) {
            free(sp->files[i]);
        }
        free(sp->files);
        sp->files = NULL;
    }

    if (sp->num_of_options) {
        for (int i = 0; i < sp->num_of_options; i++) {
            free(sp->options[i]);
        }
        free(sp->options);
        sp->options = NULL;
    }
}
