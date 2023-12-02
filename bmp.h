#ifndef _BMP_H_  // prevent recursive inclusion
#define _BMP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define BMP_HEADER_SIZE 54
#define DIB_HEADER_SIZE 40

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
    uint8_t* data;
} BMPImage;


BMPHeader read_bmp_header(int _Fd);  // > I'm really curious if I can set this bad boy to 'private' somehow. ca nu prea e okay - pot sa accesez tot header-ul din main
int32_t read_bmp_height(int _Fd);
int32_t read_bmp_width(int _Fd);
void print_bmp_header(BMPHeader header);




/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
BMPHeader read_bmp_header(int _Fd) {
    BMPHeader header;

    if (read(_Fd, &header, sizeof(header)) == -1) {
        perror("Eroare la citirea header-ului de imagine");  // > asta trb modificata, dar imi e lene acum
        close(_Fd);
        exit(EXIT_FAILURE);
    }
    print_bmp_header(header);
    return header;
}


/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
int32_t read_bmp_height(int _Fd) {
    BMPHeader header;
    header = read_bmp_header(_Fd);
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
    header = read_bmp_header(_Fd);
    return header.width_px;
}


/**
 * <placeholder>.
 * 
 * @param <placeholder>.
 * @return <placeholder>.
 */
void print_bmp_header(BMPHeader header) {
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

#endif  /* bmp.h */
