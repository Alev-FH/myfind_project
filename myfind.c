#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#define FILE_FOUND         0
#define FILE_NOT_FOUND     1
#define FILE_SEARCH_ERROR -1

typedef struct {
    char *search_path;    // The search path. First argument after argument 0 that does is not an option, namely no -R or -i.
    char **files;         // The files extracted from the command.
    char **options;       // The options extracted from the command
    int R_enabled;        // Wether or not the option -R is passed as a parameter 
    int i_enabled;        // Wether or not the option -R is passed as a parameter
    int num_of_files;     // The number of filenames extracted from the command
    int num_of_options;   // The number of options extracted from the command
    int file_found;       // The index of the file that has been found.
} SearchParams;

static void parseArguments(int argc, char *argv[], SearchParams *sp);
static int findFile(SearchParams *sp, const int active_file);
static int searchFolder(const char path[], const char file[], const int recursive, const int case_insensitive);
static void print_usage(char *programm_name);
static void freeSearchParams(SearchParams *sp);

int main(int argc, char *argv[]) {

    SearchParams sp = { 0 };
    parseArguments(argc, argv, &sp);

    // Print the SearchParams variables to see if the parseArguments function initializes them corectly.
    printf("path                    : %s\n", sp.search_path);
    printf("Recursive option        : %s\n", sp.R_enabled == 1 ? "true" : "false");
    printf("Case insensitive option : %s\n", sp.i_enabled == 1 ? "true" : "false");
    printf("num_of_files            : %d\n", sp.num_of_files);
    printf("num_of_options          : %d\n", sp.num_of_options);

    // Pint the files for testing. This will be removed.
    for (int i = 0; i < sp.num_of_files; i++) {
        printf("files: %s\n", sp.files[i]);
    }

    // Here we must fork so many times as num_of_files. Every process should search for a file.
    pid_t pid;
    for (int i = 0; i < sp.num_of_files; i++) {

        pid = fork();

        if (pid == -1) {
            return EXIT_FAILURE;
        } else if (pid == 0) {
            printf("Child process: %d\n", getpid());
            // Here we have to search for the files. This part of the code will be run only from the child processes.
            exit(findFile(&sp, i));    // exit with the return value of the function findFile. The return value will be captured by the parent process.
        } else {
            printf("Parent process: %d\n", getpid());
        }
    }

    // Here we have to wait for the children to finish execution. We first make sure that we are in the parent process. pid > 0.
    if (pid > 0) {
        int status;
        int errors_occured = 0;
        int remaining_children = sp.num_of_files;

        while (remaining_children > 0) {
            pid_t child_pid = wait(&status);

            if (child_pid == -1) {
                if (errno == EINTR) {        // Check if user or an external source terminated the parent waiting process.
                    continue;
                }

                perror("wait(): An error has occured!\n");
                errors_occured++;
                break;
            }

            remaining_children--;

            if (WIFEXITED(status)) {        // Check the return value of the child process.
                int exit_code = WEXITSTATUS(status);

                if (exit_code != FILE_FOUND && exit_code != FILE_NOT_FOUND) {
                    fprintf(stderr, "An error has occured!\n");
                    errors_occured++;
                }
            } else if (WIFSIGNALED(status)) {        // Check if child process is terminated because of a signal
                fprintf(stderr, "Child %d terminated unexpectedly. STATUS: %d\n", child_pid, WTERMSIG(status));
                errors_occured++;
            }
        }

        freeSearchParams(&sp);        // Free the SearchParams here because the parent is responsible for that.

        if (errors_occured) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;        // After parent have waited for all children, exit.
    }

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
    unsigned int path_aquired  = 0;

    if (argc > 1) {

        for (int i = 1; i < argc; i++) {
            if (strncmp(argv[i], "-", 1) == 0) {

                // Check for specific flags
                if (strcmp(argv[i], "-R") == 0) {
                    sp->R_enabled = 1;
                } else if (strcmp(argv[i], "-i") == 0) {
                    sp->i_enabled = 1;
                } else {
                    fprintf(stdout, "Option %s is not supported, it will be ignored!\n", argv[i]);
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
                if (path_aquired == 0) {        // The first non option argument after the argument 0 is always the path.
                    sp->search_path = strdup(argv[i]);
                    path_aquired++;
                    continue;
                }

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
static int findFile(SearchParams *sp, const int active_file) {
    // Index the file we are looking for and pass it to searchPath, to search for it in folders. Pass the flags needed also.
    if (searchFolder(sp->search_path, sp->files[active_file], sp->R_enabled, sp->i_enabled) == FILE_FOUND) {
        sp->file_found = active_file;
        fprintf(stdout, "File %d %s found\n", sp->file_found, sp->files[sp->file_found]);
        return FILE_FOUND;
    }

    fprintf(stdout, "File not found\n");
    sp->file_found = -1;
    return FILE_NOT_FOUND;
}
static int searchFolder(const char path[], const char file[], const int recursive, const int case_insensitive) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "Could not open directory %s\n", path);
        return FILE_SEARCH_ERROR;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if ((strncmp(entry->d_name, ".", strlen(entry->d_name)) != 0) && ((strncmp(entry->d_name, "..", strlen(entry->d_name)) != 0))) {

            if (entry->d_type != DT_UNKNOWN && entry->d_type == DT_DIR) {        // Check if entry is a folder and enable recursivness if active.
                if (recursive) {
                    char new_path[PATH_MAX];
                    int length = strlen(path) + strlen(entry->d_name) + 2;        // 1 for the null termination and 1 for the format / at next line of code.
                    snprintf(new_path, length, "%s%s/", path, entry->d_name);        // Format a new path and pass it again to the function to be searched recursively.
                    
                    if (searchFolder(new_path, file, recursive, case_insensitive) == FILE_FOUND) {
                        return FILE_FOUND;
                    }
                }
            } else {
                if (case_insensitive) {
                    if (strncasecmp(entry->d_name, file, strlen(entry->d_name)) == 0) {
                        return FILE_FOUND;
                    }
                } else {
                    if (strncmp(entry->d_name, file, strlen(entry->d_name)) == 0) {
                        return FILE_FOUND;
                    }   
                }
            }
        }
    }

    closedir(dir);
    return FILE_NOT_FOUND;
}
static void print_usage(char *programm_name) {
    printf("Usage: %s [path] [-R] [-i] [dateiname 1 dateiname n]\n\n", programm_name);
}
static void freeSearchParams(SearchParams *sp) {
    free(sp->search_path);

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
