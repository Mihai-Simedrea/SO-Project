#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>

#include "bmp.h"
#include "constants.h"
#include "utils.h"
#include "statisticsService.h"


const char *USAGE_ERROR = "Usage ./program <director_intrare>\n"; // > hmm, idk daca sa folosesc extern sau nu
const char *MEMORY_ALLOCATION_ERROR = "Out of memory\n";
const char *NO_EXTENSION_FOUND = "Extension Error\n";
const char *OPEN_FILE_ERROR = "The file can't be opened\n";
const char *CANT_READ_FROM_FILE = "Can't get the data about the file.\n";
const char *CANT_WRITE_TO_FILE = "Can't write the data to the file.\n";
const char *CANT_OPEN_DIRECTORY = "Can't open the directory\n";


int main(int argc, char **argv) {
    if (argc != 2) {
        if (sprint(USAGE_ERROR) == ERROR_SPRINT) {
            perror(MEMORY_ALLOCATION_ERROR);
        }
        exit(EXIT_FAILURE);
    }

    DIR *current_dir = opendir(argv[1]);
    if (current_dir == NULL) {
        perror(CANT_OPEN_DIRECTORY);
    }

    // write_statistics_file(file_descriptor, argv[1]);
    write_statistics_file(current_dir, "statistica.txt", argv[1]);
    
    // close(file_descriptor);
    closedir(current_dir);
    return 0;
}
