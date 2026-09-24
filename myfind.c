#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> //Unix Standard
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <dirent.h> //Directory Entries

#define FILE_FOUND         0
#define FILE_NOT_FOUND     1
#define FILE_SEARCH_ERROR -1

typedef struct {
    char *search_path;    // The search path. First argument after argument 0 that does is not an option, namely no -R or -i.
    char **files;         // The files extracted from the command. (array of strings)

    int R_enabled;        // Wether or not the option -R is passed as a parameter
    int i_enabled;        // Wether or not the option -R is passed as a parameter(case insensitive)
    int num_of_files;     // The number of filenames extracted from the command
  
} SearchParams;

static void parseArguments(int argc, char *argv[], SearchParams *sp);
static int findFile(SearchParams *sp, const int active_file);
static int searchFolder(char path[], const char file[], const int recursive, const int case_insensitive);
static void print_usage(char *programm_name);
static void freeSearchParams(SearchParams *sp);

/* 
    To print the path, when a file is found, we reallocate memory for the search_path variable of the SearchParams struct
    and copy the current path into this char array. That is happening in the searchFolders function.
*/

/*To ensure that the output from multiple child processes does not interleave and remains readable in full lines, 
we use a single fprintf() call ending with a newline character (\n) */

int main(int argc, char *argv[]) {

    SearchParams sp = { 0 };
    parseArguments(argc, argv, &sp);

    // Here we must fork so many times as num_of_files. Every process should search for a file.
    pid_t pid;
    for (int i = 0; i < sp.num_of_files; i++) {

        pid = fork();

        if (pid == -1) {
            return EXIT_FAILURE;
        } else if (pid == 0) {
            // Child process
            int found = findFile(&sp, i);    // Here we have to search for the files. This part of the code will be run only from the child processes.
            freeSearchParams(&sp);
            exit(found);    // exit with the return value of the function findFile. The return value will be captured by the parent process.
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
/* Getopt */
static void parseArguments(int argc, char *argv[], SearchParams *sp){
    int c;
    unsigned short Counter_Option_R = 0;
    unsigned short Counter_Option_i = 0;
    int error = 0;

    while ((c = getopt(argc, argv, "Ri")) != EOF) { //Catching flags
        switch (c) {
            case 'R':
                if (Counter_Option_R) { //same flag can not be used multiple
                    error = 1;
                    break;
                }
                Counter_Option_R++;
                sp->R_enabled = 1; //Enable
                break;
            case 'i':
                if (Counter_Option_i) { 
                    error = 1;
                    break;
                }
                Counter_Option_i++;
                sp->i_enabled = 1;
                break;
            case '?': // for unsupported flag
                error = 1;
                break;
            default: 
                assert(0); 
        }
    }
    
    if (error) { 
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    //Extracting the search path
    if (optind < argc) {
        
        sp->search_path = malloc(strlen(argv[optind]) + 1);
        if (sp->search_path == NULL) {
            fprintf(stderr, "Memory allocation error!\n");
            exit(EXIT_FAILURE);
        }
        strcpy(sp->search_path, argv[optind]);
        optind++; 
    }

    //Extracting the files to search
    sp->num_of_files = argc - optind;
    if (sp->num_of_files > 0) {
        sp->files = malloc(sizeof(char*) * sp->num_of_files);
        if (sp->files == NULL) { exit(EXIT_FAILURE); }
        
        int file_index = 0;
        while (optind < argc) {
            sp->files[file_index] = malloc(strlen(argv[optind]) + 1);
            if (sp->files[file_index] == NULL) { exit(EXIT_FAILURE); }
            strcpy(sp->files[file_index], argv[optind]);
            file_index++;
            optind++;
        }
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}


static int findFile(SearchParams *sp, const int active_file) {
    // Index the file we are looking for and pass it to searchPath, to search for it in folders. Pass the flags needed also.
    if (searchFolder(sp->search_path, sp->files[active_file], sp->R_enabled, sp->i_enabled) == FILE_FOUND) {
        fprintf(stdout, "<%d>: <%s>: <%s>\n", getpid(), sp->files[active_file], absolute_found_path);
        return FILE_FOUND;
    }

    return FILE_NOT_FOUND;
}
static int searchFolder(char path[], const char file[], const int recursive, const int case_insensitive) {
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
                        char *temp = realloc(path, strlen(new_path));
                        if (!temp) {
                            fprintf(stderr, "Failed to allocate memory for the found search_path");
                            return FILE_SEARCH_ERROR;
                        }
                        path = temp;
                        strncpy(path, new_path, strlen(new_path));    // Copy the new_path to search_path.
                        closedir(dir);
                        return FILE_FOUND;
                    }
                }
            } else {
                if (case_insensitive) {
                    if (strcasecmp(entry->d_name, file) == 0) {
                        closedir(dir);
                        return FILE_FOUND;
                    }
                } else {
                    if (strcmp(entry->d_name, file) == 0) {
                        closedir(dir);
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

    }
