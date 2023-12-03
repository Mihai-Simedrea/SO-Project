#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#include "constants.h"
#include "bmp.h"


void write_statistics_file(int _Fd, const char *_FileName, const char *_Path);
char *__construct_statistics_file(int _Fd, const char *_FileName);




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void write_statistics_file(int _Fd, const char *_FileName, const char *_Path) {
    char *statistics = __construct_statistics_file(_Fd, _FileName);

    int stats_fd = open(_Path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stats_fd == -1) {
        perror(OPEN_FILE_ERROR);
        close(_Fd);
        exit(EXIT_FAILURE);
    }

    if (write(stats_fd, statistics, strlen(statistics)) == -1) {
        perror(CANT_WRITE_TO_FILE);
        close(_Fd);
        close(stats_fd);
        exit(EXIT_FAILURE);
    }
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
char *__construct_statistics_file(int _Fd, const char *_FileName)
{
    struct stat file_info;
    if (fstat(_Fd, &file_info) == -1)
    {
        perror(CANT_READ_FROM_FILE);
        exit(EXIT_FAILURE);
    }

    int32_t height = read_bmp_height(_Fd);
    int32_t width = read_bmp_width(_Fd);

    char modification_time[20];
    strftime(modification_time, sizeof(modification_time), "%d.%m.%Y", localtime(&file_info.st_mtime));

    char* statistics = (char*)malloc(400 * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    snprintf(statistics, 400,
             "nume fisier: %s\n"
             "inaltime: %d\n"
             "lungime: %d\n"
             "dimensiune: %ld octeti\n"
             "identificatorul utilizatorului: %d\n"
             "timpul ultimei modificari: %s\n"
             "contorul de legaturi: %ld\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n",
             _FileName,
             height,
             width,
             file_info.st_size,
             file_info.st_uid,
             modification_time,
             file_info.st_nlink,
             get_permissions(file_info.st_mode),
             get_permissions(file_info.st_mode >> 3),
             get_permissions(file_info.st_mode >> 6));

    return statistics;
}
