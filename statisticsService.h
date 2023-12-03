#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#include "constants.h"
#include "bmp.h"


void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath);
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath);
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
void __write_into_statistics_file(int _StatsFd, const char *_Statistics);




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath) {
    __check_file_types_from_directory(_Dir, _DirPath, _OutputDirPath);
}




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath) {
    struct dirent *dir_entry;
    struct stat file_stat;
    char full_directory_path[1000];  // > remove magic number
    char *statistics = (char*)malloc(400 * sizeof(char));

    while ((dir_entry = readdir(_Dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_directory_path, sizeof(full_directory_path), "%s/%s", _DirPath, dir_entry->d_name);

        if (lstat(full_directory_path, &file_stat) == -1) {
            perror(CANT_READ_FROM_FILE);
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(file_stat.st_mode)) {
            if (has_ok_file_extension(full_directory_path, ".bmp")) {
                statistics = __construct_bmp_image_statistics(file_stat, dir_entry->d_name, full_directory_path);
            } else {
                statistics = __construct_regular_file_statistics(file_stat, dir_entry->d_name);
            }
        } else if (S_ISDIR(file_stat.st_mode)) {
            statistics = __construct_directory_statistics(file_stat, dir_entry->d_name);
        } else if (S_ISLNK(file_stat.st_mode)) {
            statistics = __construct_symbolic_link_statistics(file_stat, dir_entry->d_name, full_directory_path);
        } else {
            printf("%s is of unknown type.\n", dir_entry->d_name);  // > Maybe throw error or something
        }

        char statistics_file[500]; // > remove magic number
        snprintf(statistics_file, sizeof(statistics_file), "%s/%s_statistics.txt", _OutputDirPath, dir_entry->d_name); // it shows the extension in `d_name`, remove it later

        int stats_fd = open(statistics_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (stats_fd == -1) {
            perror(OPEN_FILE_ERROR);
            exit(EXIT_FAILURE);
        }

        __write_into_statistics_file(stats_fd, statistics);
        close(stats_fd);
    }
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName) {
    char* statistics = (char*)malloc(400 * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    snprintf(statistics, 400,
             "nume director: %s\n"
             "identificatorul utilizatorului: %d\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             _EntryFileName,
             _FileStat.st_uid,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    return statistics;
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName) {
    char* statistics = (char*)malloc(400 * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    char modification_time[20];
    strftime(modification_time, sizeof(modification_time), "%d.%m.%Y", localtime(&_FileStat.st_mtime));

    snprintf(statistics, 400,
             "nume fisier: %s\n"
             "dimensiune: %ld octeti\n"
             "identificatorul utilizatorului: %d\n"
             "timpul ultimei modificari: %s\n"
             "contorul de legaturi: %ld\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             _EntryFileName,
             _FileStat.st_size,
             _FileStat.st_uid,
             modification_time,
             _FileStat.st_nlink,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    return statistics;
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
    char* statistics = (char*)malloc(400 * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    int file_descriptor = open(_FullDirectoryPath, O_RDONLY);
    if (file_descriptor == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    int32_t height = read_bmp_height(file_descriptor);
    int32_t width = read_bmp_width(file_descriptor);
    close(file_descriptor);

    char modification_time[20];
    strftime(modification_time, sizeof(modification_time), "%d.%m.%Y", localtime(&_FileStat.st_mtime));

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
             "drepturi de acces altii: %s\n\n",
             _EntryFileName,
             height,
             width,
             _FileStat.st_size,
             _FileStat.st_uid,
             modification_time,
             _FileStat.st_nlink,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    return statistics;
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
    char* statistics = (char*)malloc(400 * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    struct stat target_stat_file;
    if (stat(_FullDirectoryPath, &target_stat_file) == -1) {
        perror(CANT_READ_FROM_FILE);
        exit(EXIT_FAILURE);
    }

    snprintf(statistics, 400,
             "nume fisier: %s\n"
             "dimensiune: %ld octeti\n"
             "dimensiune fisier: %ld octeti\n"
             "drepturi de acces user: %s\n"
             "drepturi de acces grup: %s\n"
             "drepturi de acces altii: %s\n\n",
             _EntryFileName,
             _FileStat.st_size,
             target_stat_file.st_size,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    return statistics;
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __write_into_statistics_file(int _StatsFd, const char *_Statistics) {
    if (write(_StatsFd, _Statistics, strlen(_Statistics)) == -1) {
        perror(CANT_WRITE_TO_FILE);
        close(_StatsFd);
        exit(EXIT_FAILURE);
    }
}
