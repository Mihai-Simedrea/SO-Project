#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdint.h>


struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

char *get_permissions(mode_t mode) {
    static char permissions[10];
    snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c",
        (mode & S_IRUSR) ? 'R' : '-',
        (mode & S_IWUSR) ? 'W' : '-',
        (mode & S_IXUSR) ? 'X' : '-',
        (mode & S_IRGRP) ? 'R' : '-',
        (mode & S_IWGRP) ? 'W' : '-',
        (mode & S_IXGRP) ? 'X' : '-',
        (mode & S_IROTH) ? 'R' : '-',
        (mode & S_IWOTH) ? 'W' : '-',
        (mode & S_IXOTH) ? 'X' : '-');
    return permissions;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fisier_intrare>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului");
        exit(EXIT_FAILURE);
    }

    lseek(fd, 54, SEEK_SET);
    struct BITMAPINFOHEADER bmp_header;

    if (read(fd, &bmp_header, sizeof(bmp_header)) == -1) {
        perror("Eroare la citirea header-ului de imagine");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int32_t height = abs(bmp_header.biHeight);
    int32_t width = bmp_header.biWidth;

    struct stat file_info;
    if (fstat(fd, &file_info) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisier");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char modification_time[20];
    strftime(modification_time, sizeof(modification_time), "%d.%m.%Y", localtime(&file_info.st_mtime));

    char statistics[400];
    snprintf(statistics, sizeof(statistics),
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
        argv[1],
        height,
        width,
        file_info.st_size,
        file_info.st_uid,
        modification_time,
        file_info.st_nlink,
        get_permissions(file_info.st_mode),
        get_permissions(file_info.st_mode >> 3),
        get_permissions(file_info.st_mode >> 6));

    int stats_fd = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stats_fd == -1) {
        perror("Eroare la deschiderea fisierului de statistici");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (write(stats_fd, statistics, strlen(statistics)) == -1) {
        perror("Eroare la scrierea in fisierul de statistici");
        close(fd);
        close(stats_fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    close(stats_fd);

    return 0;
}