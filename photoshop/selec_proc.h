#ifndef SELEC_PROC_H
#define SELEC_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Función auxiliar para calcular padding
static inline int calcular_padding(int ancho) {
    return (4 - (ancho * 3) % 4) % 4;
}

void manejar_encabezado(FILE *in, FILE *out, int *ancho, int *alto, int *offset) {
    unsigned char fileHeader[14];
    fread(fileHeader, 1, 14, in);
    *offset = *(int*)&fileHeader[10];
    
    unsigned char* fullHeader = (unsigned char*)malloc(*offset);
    memcpy(fullHeader, fileHeader, 14);
    fread(fullHeader + 14, 1, *offset - 14, in);
    fwrite(fullHeader, 1, *offset, out);
    
    *ancho = *(int*)&fullHeader[18];
    *alto = *(int*)&fullHeader[22];
    free(fullHeader);
}

// 1. Inversión vertical en escala de grises
extern void inv_img(const char* mask, const char* path) {
    FILE *image = fopen(path, "rb");
    if (!image) return;
    
    char add_char[80] = "./img/";
    strcat(add_char, mask); strcat(add_char, ".bmp");
    FILE *outputImage = fopen(add_char, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char** input_rows = (unsigned char**)malloc(alto * sizeof(unsigned char*));
    unsigned char** output_rows = (unsigned char**)malloc(alto * sizeof(unsigned char*));

    for (int i = 0; i < alto; i++) {
        input_rows[i] = (unsigned char*)malloc(row_padded);
        output_rows[i] = (unsigned char*)malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        unsigned char *src = input_rows[alto - 1 - y];
        unsigned char *dst = output_rows[y];
        for (int x = 0; x < ancho; x++) {
            unsigned char b = src[x * 3];
            unsigned char g = src[x * 3 + 1];
            unsigned char r = src[x * 3 + 2];
            unsigned char gray = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
            dst[x * 3] = gray;
            dst[x * 3 + 1] = gray;
            dst[x * 3 + 2] = gray;
        }
        for (int p = ancho * 3; p < row_padded; p++) dst[p] = 0;
    }

    for (int i = 0; i < alto; i++) fwrite(output_rows[i], 1, row_padded, outputImage);

    for (int i = 0; i < alto; i++) {
        free(input_rows[i]);
        free(output_rows[i]);
    }
    free(input_rows);
    free(output_rows);
    fclose(image);
    fclose(outputImage);
}

// 2. Inversión vertical a color
extern void inv_img_color(const char* mask, const char* path) {
    FILE *image = fopen(path, "rb");
    if (!image) return;
    char add_char[80] = "./img/";
    strcat(add_char, mask); strcat(add_char, ".bmp");
    FILE *outputImage = fopen(add_char, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char **input_rows = (unsigned char**)malloc(alto * sizeof(unsigned char*));
    unsigned char **output_rows = (unsigned char**)malloc(alto * sizeof(unsigned char*));

    for (int i = 0; i < alto; i++) {
        input_rows[i] = (unsigned char*)malloc(row_padded);
        output_rows[i] = (unsigned char*)malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        memcpy(output_rows[y], input_rows[alto - 1 - y], row_padded);
    }

    for (int i = 0; i < alto; i++) fwrite(output_rows[i], 1, row_padded, outputImage);

    for (int i = 0; i < alto; i++) {
        free(input_rows[i]);
        free(output_rows[i]);
    }
    free(input_rows);
    free(output_rows);
    fclose(image);
    fclose(outputImage);
}

// 3. Desenfoque a color (Optimizado para usar menos memoria)
extern void desenfoque_color(const char* input_path, const char* name_output, int kernel_size) {
    FILE *image = fopen(input_path, "rb");
    if (!image) return;
    char output_path[100] = "./img/";
    strcat(output_path, name_output); strcat(output_path, ".bmp");
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);
    
    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char** input_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** temp_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** output_rows = malloc(alto * sizeof(unsigned char*));

    for (int i = 0; i < alto; i++) {
        input_rows[i] = malloc(row_padded);
        temp_rows[i] = malloc(row_padded);
        output_rows[i] = malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    int k = kernel_size / 2;
    
    // Primera pasada: desenfoque horizontal (sin modificar input_rows in-place)
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) {
                    sB += input_rows[y][nx*3]; sG += input_rows[y][nx*3+1]; sR += input_rows[y][nx*3+2]; count++;
                }
            }
            temp_rows[y][x*3] = sB / count;
            temp_rows[y][x*3+1] = sG / count;
            temp_rows[y][x*3+2] = sR / count;
        }
        for (int p = ancho * 3; p < row_padded; p++) temp_rows[y][p] = input_rows[y][p];
    }

    // Segunda pasada: desenfoque vertical
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) {
                    sB += temp_rows[ny][x*3]; sG += temp_rows[ny][x*3+1]; sR += temp_rows[ny][x*3+2]; count++;
                }
            }
            output_rows[y][x*3] = sB/count; output_rows[y][x*3+1] = sG/count; output_rows[y][x*3+2] = sR/count;
        }
        for (int p = ancho * 3; p < row_padded; p++) output_rows[y][p] = temp_rows[y][p];
    }

    for (int i = 0; i < alto; i++) fwrite(output_rows[i], 1, row_padded, outputImage);

    for(int i=0; i<alto; i++) { free(input_rows[i]); free(temp_rows[i]); free(output_rows[i]); }
    free(input_rows); free(temp_rows); free(output_rows);
    fclose(image); fclose(outputImage);
}
#endif
