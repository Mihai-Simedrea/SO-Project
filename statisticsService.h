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

#define CHILD_TO_CHILD_COMMUNICATION_PORT 999
#define CHILD_PIDS_SIZE 1000
#define STATISTICS_FILE_LENGTH 400
#define SYSTEM_COMMAND_LENGTH 100000


void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character);
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character);
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName);
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath);
void __write_into_statistics_file(int _StatsFd, const char *_Statistics, int _PipeFds[][2], uint32_t _ChildCount);
void __wait_all_processes(pid_t _ChildPids[], uint32_t _ChildCount, int _PipeFds[][2]);




/**
 * Generates statistics for files in a directory and writes the output to files to an output directory.
 *
 * @param _Dir A pointer to the directory structure object.
 * @param _DirPath The path to the directory to be analyzed.
 * @param _OutputDirPath The path to the output directory where the statistics file will be saved.
 * @param _Character The character for the bash script.
 * 
 * Note: This function relies on an internal function '__check_file_types_from_directory' 
 * to process the directory contents and generate statistics.
 */
void write_statistics_file(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character) {
    __check_file_types_from_directory(_Dir, _DirPath, _OutputDirPath, _Character);
}




/**
 * Analyzes files within a specified directory, generates statistics,
 * and manages child processes to perform various file operations.
 *
 * @param _Dir A pointer to the directory structure object.
 * @param _DirPath The path to the directory whose files are to be analyzed.
 * @param _OutputDirPath The path to the output directory where statistics files will be saved.
 * @param _Character The character for the regex.
 * 
 */
void __check_file_types_from_directory(DIR *_Dir, const char *_DirPath, const char *_OutputDirPath, char _Character) {
    struct dirent *dir_entry;
    struct stat file_stat;
    uint32_t full_directory_path_size = sizeof(_DirPath) + sizeof(dir_entry->d_name) + 2;
    char full_directory_path[full_directory_path_size];
    char *statistics = (char*)malloc(STATISTICS_FILE_LENGTH * sizeof(char));
    pid_t pid;
    pid_t child_pids[CHILD_PIDS_SIZE];
    uint32_t child_count = 0;
    int pipe_fds[CHILD_PIDS_SIZE][2];
    uint32_t statistics_file_size = sizeof(_OutputDirPath) + sizeof(dir_entry->d_name) + 21;
    char statistics_file[statistics_file_size];

    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    while ((dir_entry = readdir(_Dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_directory_path, full_directory_path_size, "%s/%s", _DirPath, dir_entry->d_name);

        if (lstat(full_directory_path, &file_stat) == -1) {
            perror(CANT_READ_FROM_FILE);
            exit(EXIT_FAILURE);
        }

        snprintf(statistics_file, statistics_file_size, "%s/%s_statistics.txt", _OutputDirPath, dir_entry->d_name);

        int stats_fd = open(statistics_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (stats_fd == -1) {
            perror(OPEN_FILE_ERROR);
            exit(EXIT_FAILURE);
        }

        if (pipe(pipe_fds[child_count]) == -1) {
            perror(PIPE_ERROR);
            exit(EXIT_FAILURE);
        }

        if (pipe(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT]) == -1) {
            perror(PIPE_ERROR);
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
                perror(FORK_OPERATION_ERROR);
                exit(EXIT_FAILURE);
            } else {
                if (has_ok_file_extension(full_directory_path, ".bmp")) {
                    pid_t grayscale_pid = fork();
                    child_pids[child_count++] = grayscale_pid;

                    if (grayscale_pid == 0) {
                        __convert_to_grayscale(full_directory_path);
                        exit(0);
                    } else if (grayscale_pid < 0) {
                        perror(FORK_OPERATION_ERROR);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    pid_t second_pid = fork();

                    if (second_pid == 0) {
                        char stats_content[STATISTICS_FILE_LENGTH];
                        close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][1]);
                        read(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0], stats_content, sizeof(stats_content));
                        close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0]);

                        char system_command[SYSTEM_COMMAND_LENGTH];
                        snprintf(system_command, sizeof(system_command), "./script.sh %c << \"%s\" > temp.txt", _Character, stats_content);
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
                        perror(FORK_OPERATION_ERROR);
                        exit(EXIT_FAILURE);
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
                perror(FORK_OPERATION_ERROR);
                exit(EXIT_FAILURE);
            } else {
                pid_t second_pid = fork();

                if (second_pid == 0) {
                    char stats_content[STATISTICS_FILE_LENGTH];
                    close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][1]);
                    read(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0], stats_content, sizeof(stats_content));
                    close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0]);

                    char system_command[SYSTEM_COMMAND_LENGTH];
                    snprintf(system_command, sizeof(system_command), "./script.sh %c << \"%s\" > temp.txt", _Character, stats_content);
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
                    perror(FORK_OPERATION_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
        } else if (S_ISLNK(file_stat.st_mode)) {
            pid = fork();

            if (pid == 0) {
                statistics = __construct_symbolic_link_statistics(file_stat, dir_entry->d_name, full_directory_path);
                __write_into_statistics_file(stats_fd, statistics, pipe_fds, child_count);
                exit(0);
            } else if (pid < 0) {
                perror(FORK_OPERATION_ERROR);
                exit(EXIT_FAILURE);
            } else {
                pid_t second_pid = fork();

                if (second_pid == 0) {
                    char stats_content[STATISTICS_FILE_LENGTH];
                    close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][1]);
                    read(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0], stats_content, sizeof(stats_content));
                    close(pipe_fds[CHILD_TO_CHILD_COMMUNICATION_PORT][0]);

                    char system_command[SYSTEM_COMMAND_LENGTH];
                    snprintf(system_command, sizeof(system_command), "./script.sh %c << \"%s\" > temp.txt", _Character, stats_content);
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
                    perror(FORK_OPERATION_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            printf("%s is of unknown type.\n", dir_entry->d_name);
            exit(EXIT_FAILURE);
        }

        child_pids[child_count++] = pid;
        close(stats_fd);
    }

    __wait_all_processes(child_pids, child_count, pipe_fds);
}


/**
 * Waits for the termination of all child processes and handles their exit statuses.
 *
 * @param _ChildPids An array containing the PIDs of the child processes.
 * @param _ChildCount The count of child processes to wait for and handle.
 * @param _PipeFds An array of pipe file descriptors for communication with child processes.
 * 
 */
void __wait_all_processes(pid_t _ChildPids[], uint32_t _ChildCount, int _PipeFds[][2]) {
    int status = 0;
    int lines_written = 0;
    for (uint32_t index = 0; index < _ChildCount; ++index) {
        if (waitpid(_ChildPids[index], &status, 0) == -1) {
            perror(WAIT_ERROR);
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", _ChildPids[index], WEXITSTATUS(status));

            close(_PipeFds[index][1]);
            read(_PipeFds[index][0], &lines_written, sizeof(lines_written));
            printf("Child %d: Lines written = %d\n", index + 1, lines_written);
            close(_PipeFds[index][0]);
        }
    }
}


/**
 * Constructs statistics related to a directory entry's attributes.
 *
 * @param _FileStat The 'struct stat' structure containing file status information.
 * @param _EntryFileName The name of the directory entry.
 * @return A dynamically allocated string containing directory attribute statistics.
 * 
 */
char *__construct_directory_statistics(struct stat _FileStat, const char *_EntryFileName) {
    char* statistics = (char*)malloc(STATISTICS_FILE_LENGTH * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    snprintf(statistics, STATISTICS_FILE_LENGTH,
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
 * Constructs statistics related to a regular file's attributes.
 *
 * @param _FileStat The 'struct stat' structure containing file status information.
 * @param _EntryFileName The name of the regular file.
 * @return A dynamically allocated string containing regular file attribute statistics.
 *
 */
char *__construct_regular_file_statistics(struct stat _FileStat, const char *_EntryFileName) {
    char* statistics = (char*)malloc(STATISTICS_FILE_LENGTH * sizeof(char));
    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    char modification_time[20];
    strftime(modification_time, sizeof(modification_time), "%d.%m.%Y", localtime(&_FileStat.st_mtime));

    snprintf(statistics, STATISTICS_FILE_LENGTH,
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
 * Constructs statistics related to a BMP image file's attributes.
 *
 * @param _FileStat The 'struct stat' structure containing file status information.
 * @param _EntryFileName The name of the BMP image file.
 * @param _FullDirectoryPath The complete path to the BMP image file.
 * @return A dynamically allocated string containing BMP image file attribute statistics.
 *
 */
char *__construct_bmp_image_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
    char* statistics = (char*)malloc(STATISTICS_FILE_LENGTH * sizeof(char));

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

    snprintf(statistics, STATISTICS_FILE_LENGTH,
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
 * Constructs statistics related to a symbolic link's attributes.
 *
 * @param _FileStat The 'struct stat' structure containing file status information of the symbolic link.
 * @param _EntryFileName The name of the symbolic link.
 * @param _FullDirectoryPath The complete path to the symbolic link.
 * @return A dynamically allocated string containing symbolic link attribute statistics.
 *
 */
char *__construct_symbolic_link_statistics(struct stat _FileStat, const char *_EntryFileName, const char *_FullDirectoryPath) {
    char* statistics = (char*)malloc(STATISTICS_FILE_LENGTH * sizeof(char));

    if (statistics == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        exit(EXIT_FAILURE);
    }

    struct stat target_stat_file;
    if (stat(_FullDirectoryPath, &target_stat_file) == -1) {
        perror(CANT_READ_FROM_FILE);
        exit(EXIT_FAILURE);
    }

    snprintf(statistics, STATISTICS_FILE_LENGTH,
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
 * Writes statistics into a statistics file and communicates line count through pipes.
 * 
 * @param _StatsFd The file descriptor for the statistics file.
 * @param _Statistics The string containing statistics information to be written into the file.
 * @param _PipeFds An array of pipe file descriptors used for inter-process communication.
 * @param _ChildCount The index of the relevant child process for communication.
 * 
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
    if (write(_PipeFds[_ChildCount][1], &lines_written, sizeof(lines_written)) == -1) {
        perror(PIPE_ERROR);
        exit(EXIT_FAILURE);
    };
    close(_PipeFds[_ChildCount][1]);

    close(_PipeFds[CHILD_TO_CHILD_COMMUNICATION_PORT][0]);
    if (write(_PipeFds[CHILD_TO_CHILD_COMMUNICATION_PORT][1], _Statistics, sizeof(_Statistics) * strlen(_Statistics)) == -1) {
        perror(PIPE_ERROR);
        exit(EXIT_FAILURE);
    };
    close(_PipeFds[CHILD_TO_CHILD_COMMUNICATION_PORT][1]);
}
