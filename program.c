#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "bmp.h"
#include "constants.h"
#include "utils.h"
#include "statisticsService.h"


const char *USAGE_ERROR = "Usage ./program <fisier_intrare>\n"; // > hmm, idk daca sa folosesc extern sau nu
const char *MEMORY_ALLOCATION_ERROR = "Out of memory\n";
const char *NO_EXTENSION_FOUND = "Extension Error\n";
const char *OPEN_FILE_ERROR = "The file can't be opened\n";
const char *CANT_READ_FROM_FILE = "Can't get the data about the file.\n";
const char *CANT_WRITE_TO_FILE = "Can't write the data to the file.\n";


int main(int argc, char **argv) {
    if (argc != 2) {
        if (sprint(USAGE_ERROR) == ERROR_SPRINT) {
            perror(MEMORY_ALLOCATION_ERROR);
        }
        exit(EXIT_FAILURE);
    }
    if (!has_ok_file_extension(argv[1], ".bmp")) {
        perror(NO_EXTENSION_FOUND);
        exit(EXIT_FAILURE);
    }


    int file_descriptor = open(argv[1], O_RDONLY);
    if (file_descriptor == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    const char *output_file_path = "statistica.txt";
    write_statistics_file(file_descriptor, argv[1], output_file_path);
    
    close(file_descriptor);
    return 0;
}
