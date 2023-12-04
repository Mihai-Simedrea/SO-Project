// > sa nu uit de free la memorie ms

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
#include <sys/wait.h>

#include "constants.h"
#include "bmp.h"


void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath);
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath);
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
void __write_into_statistics_file(int _StatsFd, const char *_Statistics);
void __handle_entry(char *_FullDirectoryPath, const char *_DirPath, const char *_OutputDirPath, struct dirent *_DirEntry, struct stat _FileStat, const char *_Stats, int lines, int pipefd[]);



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

        int pipefd[2];
        int lines = 0;

        if (pipe(pipefd) == -1) {
            perror("Pipe failed"); // > replace with defined error code
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        int status;

        if (pid < 0) {
            perror(FORK_OPERATION_ERROR);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            __handle_entry(full_directory_path, _DirPath, _OutputDirPath, dir_entry, file_stat, statistics, lines, pipefd);
            exit(0);
        }

        while((pid = wait(&status)) != -1) {
            if (WIFEXITED(status)) {
                close(pipefd[1]);
                read(pipefd[0], &lines, sizeof(lines));
                close(pipefd[0]);

                printf("Number of lines in the file: %d\n", lines);
                printf("pid = %d, status = %d\n", pid, WEXITSTATUS(status));
            }
        }
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


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __handle_entry(char *_FullDirectoryPath, const char *_DirPath, const char *_OutputDirPath, struct dirent *_DirEntry, struct stat _FileStat, const char *_Stats, int lines, int pipefd[]) {
    snprintf(_FullDirectoryPath, 1000, "%s/%s", _DirPath, _DirEntry->d_name); // > remove magic number

    if (lstat(_FullDirectoryPath, &_FileStat) == -1) {
        perror(CANT_READ_FROM_FILE);
        exit(EXIT_FAILURE);
    }

    // fiecare if un fork

    if (S_ISREG(_FileStat.st_mode)) {
        if (has_ok_file_extension(_FullDirectoryPath, ".bmp")) {
            _Stats = __construct_bmp_image_statistics(_FileStat, _DirEntry->d_name, _FullDirectoryPath);
            __convert_to_grayscale(_FullDirectoryPath); // > should I start another process from here?
        } else {
            _Stats = __construct_regular_file_statistics(_FileStat, _DirEntry->d_name);
        }
    } else if (S_ISDIR(_FileStat.st_mode)) {
        _Stats = __construct_directory_statistics(_FileStat, _DirEntry->d_name);
    } else if (S_ISLNK(_FileStat.st_mode)) {
        _Stats = __construct_symbolic_link_statistics(_FileStat, _DirEntry->d_name, _FullDirectoryPath);
    } else {
        printf("%s is of unknown type.\n", _DirEntry->d_name);  // > Maybe throw error or something
    }

    char statistics_file[500]; // > remove magic number
    snprintf(statistics_file, sizeof(statistics_file), "%s/%s_statistics.txt", _OutputDirPath, _DirEntry->d_name); // > it shows the extension in `d_name`, remove it later

    int stats_fd = open(statistics_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stats_fd == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    __write_into_statistics_file(stats_fd, _Stats);
    close(stats_fd);

    // > Maybe the below code could be extracted into a function
    char system_command[600];  // > remove magic number
    snprintf(system_command, sizeof(system_command), "./script.sh %s > statistics.txt", statistics_file);    
    system(system_command);

    int system_fd = open("statistics.txt", O_RDONLY);
    if (system_fd == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    char read_lines[1000]; // > remove magic number
    ssize_t bytes_read = read(system_fd, read_lines, sizeof(read_lines) - 1);
    if (bytes_read == -1) {
        perror(OPEN_FILE_ERROR); // > not sure about this perror
        exit(EXIT_FAILURE);
    }

    read_lines[bytes_read] = '\0';

    lines = atoi(read_lines);
    close(pipefd[0]);
    write(pipefd[1], &lines, sizeof(lines));
    close(pipefd[1]);

    close(system_fd);
}
