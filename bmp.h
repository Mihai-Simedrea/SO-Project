// > sa nu uit de free la memorie ms

#ifndef _BMP_H_  // prevent recursive inclusion
#define _BMP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "constants.h"

#define BMP_HEADER_SIZE 54
#define DIB_HEADER_SIZE 40

#pragma pack(push, 1)
typedef struct {
    uint16_t type;              // Magic identifier: 0x4d42
    uint32_t size;              // File size in bytes
    uint16_t reserved1;         // Not used
    uint16_t reserved2;         // Not used
    uint32_t offset;            // Offset to image data in bytes from beginning of file
    uint32_t dib_header_size;   // DIB Header size in bytes
    int32_t  width_px;          // Width of the image
    int32_t  height_px;         // Height of image
    uint16_t num_planes;        // Number of color planes
    uint16_t bits_per_pixel;    // Bits per pixel
    uint32_t compression;       // Compression type
    uint32_t image_size_bytes;  // Image size in bytes
    int32_t  x_resolution_ppm;  // Pixels per meter
    int32_t  y_resolution_ppm;  // Pixels per meter
    uint32_t num_colors;        // Number of colors
    uint32_t important_colors;  // Important colors
} BMPHeader;

typedef struct {
    BMPHeader header;
    uint8_t *data;
} BMPImage;
#pragma pack(pop)

int32_t read_bmp_height(int _Fd);
int32_t read_bmp_width(int _Fd);
void __print_bmp_header(BMPHeader header);
BMPHeader __read_bmp_header(int _Fd);  // > I'm really curious if I can set this bad boy to 'private' somehow. ca nu prea e okay - pot sa accesez tot header-ul din main
void __convert_to_grayscale(const char *_FullDirectoryPath);



/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
int32_t read_bmp_height(int _Fd) {
    BMPHeader header;
    header = __read_bmp_header(_Fd);
    return header.height_px;
}


/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
int32_t read_bmp_width(int _Fd) {
    BMPHeader header;
    header = __read_bmp_header(_Fd);
    return header.width_px;
}


/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
BMPHeader __read_bmp_header(int _Fd) {
    lseek(_Fd, 0, SEEK_SET);
    BMPHeader header;

    if (read(_Fd, &header, sizeof(header)) == -1) {
        perror(CANT_READ_FROM_FILE);
        close(_Fd);
        exit(EXIT_FAILURE);
    }
    return header;
}


/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __print_bmp_header(BMPHeader header) {
    printf("Type: 0x%X\n", header.type);
    printf("Size: %u bytes\n", header.size);
    printf("Reserved1: %u\n", header.reserved1);
    printf("Reserved2: %u\n", header.reserved2);
    printf("Offset: %u bytes\n", header.offset);
    printf("DIB Header Size: %u bytes\n", header.dib_header_size);
    printf("Width: %d pixels\n", header.width_px);
    printf("Height: %d pixels\n", header.height_px);
    printf("Number of Planes: %u\n", header.num_planes);
    printf("Bits per Pixel: %u\n", header.bits_per_pixel);
    printf("Compression: %u\n", header.compression);
    printf("Image Size: %u bytes\n", header.image_size_bytes);
    printf("X Resolution: %d pixels per meter\n", header.x_resolution_ppm);
    printf("Y Resolution: %d pixels per meter\n", header.y_resolution_ppm);
    printf("Number of Colors: %u\n", header.num_colors);
    printf("Important Colors: %u\n", header.important_colors);
}


/**
 * <placeholder>.
 *
 * @param <placeholder>.
 * @return <placeholder>.
 */
void __convert_to_grayscale(const char *_FullDirectoryPath) {
    int file_descriptor = open(_FullDirectoryPath, O_RDWR);
    if (file_descriptor == -1) {
        perror(OPEN_FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    BMPImage image;

    int height = read_bmp_height(file_descriptor);
    int width = read_bmp_width(file_descriptor);

    size_t image_size = height * width * 3;

    image.data = (uint8_t*)malloc(image_size);
    if (image.data == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    lseek(file_descriptor, 54, SEEK_SET);

    if (read(file_descriptor, image.data, image_size) == -1) {
        perror(CANT_READ_FROM_FILE);
        free(image.data);
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    for (uint32_t index = 0; index < image_size; index += 3) {
        uint8_t gray_value = (uint8_t)(0.299 * image.data[index] + 0.587 * image.data[index + 1] + 0.114 * image.data[index + 2]);
        image.data[index] = image.data[index + 1] = image.data[index + 2] = gray_value;
    }

    lseek(file_descriptor, 54, SEEK_SET);

    if (write(file_descriptor, image.data, image_size) == -1) {
        perror(CANT_WRITE_TO_FILE);
        free(image.data);
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    free(image.data);
    close(file_descriptor);
}
#endif  /* bmp.h */
