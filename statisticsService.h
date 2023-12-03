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


void write_statistics_file(DIR *_Dir, const char *_StatsFile, const char *_DirPath);
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath);
void __construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName);
void __construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName);
void __construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
void __construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void write_statistics_file(DIR *_Dir, const char *_StatsFile, const char *_DirPath) {
    // char *statistics = __construct_statistics_file();
    __check_file_types_from_directory(_Dir, _DirPath);

    int stats_fd = open(_StatsFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stats_fd == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    // if (write(stats_fd, statistics, strlen(statistics)) == -1) {
    //     perror(CANT_WRITE_TO_FILE);
    //     close(stats_fd);
    //     exit(EXIT_FAILURE);
    // }
}




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath) {
    struct dirent *dir_entry;
    struct stat file_stat;
    char full_directory_path[1000];  // > remove magic number

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
                __construct_bmp_image_statistics(file_stat, dir_entry->d_name, full_directory_path);
            } else {
                __construct_regular_file_statistics(file_stat, dir_entry->d_name);
            }
        } else if (S_ISDIR(file_stat.st_mode)) {
            __construct_directory_statistics(file_stat, dir_entry->d_name);
        } else if (S_ISLNK(file_stat.st_mode)) {
            __construct_symbolic_link_statistics(file_stat, dir_entry->d_name, full_directory_path);
        } else {
            printf("%s is of unknown type.\n", dir_entry->d_name);  // > Maybe throw error or something
        }
    }
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName) {
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
             "drepturi de acces altii: %s\n",
             _EntryFileName,
             _FileStat.st_uid,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    printf("%s\n", statistics);
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName) {
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
             "drepturi de acces altii: %s\n",
             _EntryFileName,
             _FileStat.st_size,
             _FileStat.st_uid,
             modification_time,
             _FileStat.st_nlink,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    printf("%s\n", statistics);
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
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
             "drepturi de acces altii: %s\n",
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

    printf("%s\n", statistics);
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
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
             "drepturi de acces altii: %s\n",
             _EntryFileName,
             _FileStat.st_size,
             target_stat_file.st_size,
             get_permissions(_FileStat.st_mode),
             get_permissions(_FileStat.st_mode >> 3),
             get_permissions(_FileStat.st_mode >> 6));

    printf("%s\n", statistics);
}
