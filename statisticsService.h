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


void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character);
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character);
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
void __write_into_statistics_file(int _StatsFd, const char *_Statistics, int _PipeFds[][2], uint32_t _ChildCount);
void __wait_all_processes(pid_t child_pids[], uint32_t child_count, int pipe_fds[][2]);




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character) {
    __check_file_types_from_directory(_Dir, _DirPath, _OutputDirPath, _Character);
}




/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character) {
    struct dirent *dir_entry;
    struct stat file_stat;
    char full_directory_path[1000];  // > remove magic number
    char *statistics = (char*)malloc(400 * sizeof(char));
    pid_t pid;
    pid_t child_pids[1000];
    uint32_t child_count = 0;
    int pipe_fds[1000][2];

    while ((dir_entry = readdir(_Dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_directory_path, 1000, "%s/%s", _DirPath, dir_entry->d_name); // > remove magic number

        if (lstat(full_directory_path, &file_stat) == -1) {
            perror(CANT_READ_FROM_FILE);
            exit(EXIT_FAILURE);
        }

        char statistics_file[500]; // > remove magic number
        snprintf(statistics_file, sizeof(statistics_file), "%s/%s_statistics.txt", _OutputDirPath, dir_entry->d_name); // > it shows the extension in `d_name`, remove it later

        int stats_fd = open(statistics_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (stats_fd == -1) {
            perror(OPEN_FILE_ERROR);
            exit(EXIT_FAILURE);
        }

        if (pipe(pipe_fds[child_count]) == -1) {
            perror("Pipe failed"); // > replace with defined error code
            exit(EXIT_FAILURE);
        }

        // I used position 99 for child to child communication
        if (pipe(pipe_fds[99]) == -1) {
            perror("pipe failed"); // > replace with defined error code
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(file_stat.st_mode)) {
            pid = fork();

            if (pid == 0) {
                if (has_ok_file_extension(full_directory_path, ".bmp")) {
                    statistics = __construct_bmp_image_statistics(file_stat, dir_entry->d_name, full_directory_path);
                } else {
                    statistics = __construct_regular_file_statistics(file_stat, dir_entry->d_name);
                }
                __write_into_statistics_file(stats_fd, statistics, pipe_fds, child_count);
                exit(0);
            } else if (pid < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
            } else {
                if (has_ok_file_extension(full_directory_path, ".bmp")) {
                    pid_t grayscale_pid = fork();
                    child_pids[child_count++] = grayscale_pid;

                    if (grayscale_pid == 0) {
                        __convert_to_grayscale(full_directory_path);
                        exit(0);
                    } else if (grayscale_pid < 0) {
                        perror("fork error");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    // i am in parent process
                    pid_t second_pid = fork();

                    if (second_pid == 0) {
                        char stats_content[100000];
                        close(pipe_fds[99][1]);
                        read(pipe_fds[99][0], stats_content, sizeof(stats_content));
                        close(pipe_fds[99][0]);

                        char system_command[100000];
                        snprintf(system_command, sizeof(system_command) * 100, "./script.sh %c << \"%s\" > temp.txt", 'a', stats_content);
                        system(system_command);

                        int system_fd = open("temp.txt", O_RDONLY);
                        if (system_fd == -1) {
                            perror(OPEN_FILE_ERROR);
                            exit(EXIT_FAILURE);
                        }

                        char read_lines[100];
                        if (read(system_fd, read_lines, sizeof(read_lines)) == -1) {
                            perror(OPEN_FILE_ERROR);
                            exit(EXIT_FAILURE);
                        }

                        uint32_t correct_lines = atoi(read_lines);
                        close(system_fd);

                        printf("Au fost identificate in total %d propozitii corecte care contin caracterul %c\n", correct_lines, _Character);

                        exit(0);
                    } else if (second_pid < 0) {
                        perror("fork error");
                        exit(EXIT_FAILURE);
                    } else {
                        // parent process again
                    }
                }
            }
            
        } else if (S_ISDIR(file_stat.st_mode)) {
            pid = fork();

            if (pid == 0) {
                statistics = __construct_directory_statistics(file_stat, dir_entry->d_name);
                __write_into_statistics_file(stats_fd, statistics, pipe_fds, child_count);
                exit(0);
            } else if (pid < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
            } else {
                // parent process

                pid_t second_pid = fork();

                if (second_pid == 0) {
                    char stats_content[100000];
                    close(pipe_fds[99][1]);
                    read(pipe_fds[99][0], stats_content, sizeof(stats_content));
                    close(pipe_fds[99][0]);

                    char system_command[100000];
                    snprintf(system_command, sizeof(system_command) * 100, "./script.sh %c << \"%s\" > temp.txt", 'a', stats_content);
                    system(system_command);

                    int system_fd = open("temp.txt", O_RDONLY);
                    if (system_fd == -1) {
                        perror(OPEN_FILE_ERROR);
                        exit(EXIT_FAILURE);
                    }

                    char read_lines[100];
                    if (read(system_fd, read_lines, sizeof(read_lines)) == -1) {
                        perror(OPEN_FILE_ERROR);
                        exit(EXIT_FAILURE);
                    }

                    uint32_t correct_lines = atoi(read_lines);

                    printf("Au fost identificate in total %d propozitii corecte care contin caracterul %c\n", correct_lines, _Character);

                    close(system_fd);
                    exit(0);
                } else if (second_pid < 0) {
                    perror("fork error");
                    exit(EXIT_FAILURE);
                } else {
                    // parent process again
                }
            }
        } else if (S_ISLNK(file_stat.st_mode)) {
            pid = fork();

            if (pid == 0) {
                statistics = __construct_symbolic_link_statistics(file_stat, dir_entry->d_name, full_directory_path);
                __write_into_statistics_file(stats_fd, statistics, pipe_fds, child_count);
                exit(0);
            } else if (pid < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
            } else {
                // parent process

                pid_t second_pid = fork();

                if (second_pid == 0) {
                    char stats_content[100000];
                    close(pipe_fds[99][1]);
                    read(pipe_fds[99][0], stats_content, sizeof(stats_content));
                    close(pipe_fds[99][0]);

                    char system_command[100000];
                    snprintf(system_command, sizeof(system_command) * 100, "./script.sh %c << \"%s\" > temp.txt", 'a', stats_content);
                    system(system_command);

                    int system_fd = open("temp.txt", O_RDONLY);
                    if (system_fd == -1) {
                        perror(OPEN_FILE_ERROR);
                        exit(EXIT_FAILURE);
                    }

                    char read_lines[100];
                    if (read(system_fd, read_lines, sizeof(read_lines)) == -1) {
                        perror(OPEN_FILE_ERROR);
                        exit(EXIT_FAILURE);
                    }

                    uint32_t correct_lines = atoi(read_lines);

                    printf("Au fost identificate in total %d propozitii corecte care contin caracterul %c\n", correct_lines, _Character);

                    close(system_fd);
                    exit(0);
                } else if (second_pid < 0) {
                    perror("fork error");
                    exit(EXIT_FAILURE);
                } else {
                    // parent process again
                }
            }
        } else {
            printf("%s is of unknown type.\n", dir_entry->d_name);  // > Maybe throw error or something
        }

        child_pids[child_count++] = pid;
        close(stats_fd);
    }

    __wait_all_processes(child_pids, child_count, pipe_fds);
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __wait_all_processes(pid_t child_pids[], uint32_t child_count, int pipe_fds[][2]) { // > refactor param namings
    int status;
    for (uint32_t index = 0; index < child_count; ++index) {
        waitpid(child_pids[index], &status, 0); // > should something be checked here?
        if (WIFEXITED(status)) {
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", child_pids[index], WEXITSTATUS(status));

            close(pipe_fds[index][1]);
            int lines_written;
            read(pipe_fds[index][0], &lines_written, sizeof(lines_written));
            printf("Child %d: Lines written = %d\n", index + 1, lines_written);

            close(pipe_fds[index][0]);
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
void __write_into_statistics_file(int _StatsFd, const char *_Statistics, int _PipeFds[][2], uint32_t _ChildCount) {
    uint32_t lines_written = 0;

    const char *ptr = _Statistics;
    while (*ptr != '\0') {
        if (*ptr == '\n') {
            lines_written++;
        }
        ptr++;
    }
    lines_written -= 1;

    if (write(_StatsFd, _Statistics, strlen(_Statistics)) == -1) {
        perror(CANT_WRITE_TO_FILE);
        close(_StatsFd);
        exit(EXIT_FAILURE);
    }

    close(_PipeFds[_ChildCount][0]);
    write(_PipeFds[_ChildCount][1], &lines_written, sizeof(lines_written));
    close(_PipeFds[_ChildCount][1]);

    close(_PipeFds[99][0]);
    write(_PipeFds[99][1], _Statistics, sizeof(_Statistics) * strlen(_Statistics));
    close(_PipeFds[99][1]);
}
