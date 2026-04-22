#ifndef SELEC_PROC_1_H
#define SELEC_PROC_1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

void manejar_encabezado_1(FILE *in, FILE *out, int *ancho, int *alto, int *offset) {
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

extern void gray_img(const char* mask, const char* path) {
    FILE *image = fopen(path, "rb");
    char add_char[80] = "./img/";
    strcat(add_char, mask); strcat(add_char, ".bmp");
    FILE *outputImage = fopen(add_char, "wb");

    int ancho, alto, offset;
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            unsigned char b = fgetc(image);
            unsigned char g = fgetc(image);
            unsigned char r = fgetc(image);
            unsigned char gray = 0.21 * r + 0.72 * g + 0.07 * b;
            fputc(gray, outputImage); fputc(gray, outputImage); fputc(gray, outputImage);
        }
        for (int p = 0; p < padding; p++) { fgetc(image); fputc(0, outputImage); }
    }
    fclose(image); fclose(outputImage);
}

extern void inv_img_grey_horizontal(const char* mask, const char* path) {
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
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char** input_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** output_rows = malloc(alto * sizeof(unsigned char*));

    for(int i = 0; i < alto; i++) {
        input_rows[i] = malloc(row_padded);
        output_rows[i] = malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    #pragma omp parallel for schedule(static)
    for(int y = 0; y < alto; y++) {
        for(int x = 0; x < ancho; x++) {
            int mirrored_x = ancho - 1 - x;
            unsigned char b = input_rows[y][mirrored_x * 3];
            unsigned char g = input_rows[y][mirrored_x * 3 + 1];
            unsigned char r = input_rows[y][mirrored_x * 3 + 2];
            unsigned char gray = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
            output_rows[y][x * 3] = gray;
            output_rows[y][x * 3 + 1] = gray;
            output_rows[y][x * 3 + 2] = gray;
        }
        for (int p = ancho * 3; p < row_padded; p++) output_rows[y][p] = 0;
    }

    for (int i = 0; i < alto; i++) fwrite(output_rows[i], 1, row_padded, outputImage);

    for(int i = 0; i < alto; i++) {
        free(input_rows[i]);
        free(output_rows[i]);
    }
    free(input_rows);
    free(output_rows);
    fclose(image);
    fclose(outputImage);
}

extern void inv_img_color_horizontal(const char* mask, const char* path) {
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
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char **input_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char **output_rows = malloc(alto * sizeof(unsigned char*));

    for(int i = 0; i < alto; i++) {
        input_rows[i] = malloc(row_padded);
        output_rows[i] = malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    #pragma omp parallel for schedule(static)
    for(int y = 0; y < alto; y++) {
        for(int x = 0; x < ancho; x++) {
            int mirrored_x = ancho - 1 - x;
            output_rows[y][x * 3] = input_rows[y][mirrored_x * 3];
            output_rows[y][x * 3 + 1] = input_rows[y][mirrored_x * 3 + 1];
            output_rows[y][x * 3 + 2] = input_rows[y][mirrored_x * 3 + 2];
        }
        for(int p = ancho * 3; p < row_padded; p++) output_rows[y][p] = input_rows[y][p];
    }

    for(int i = 0; i < alto; i++) fwrite(output_rows[i], 1, row_padded, outputImage);

    for(int i = 0; i < alto; i++) {
        free(input_rows[i]);
        free(output_rows[i]);
    }
    free(input_rows);
    free(output_rows);
    fclose(image);
    fclose(outputImage);
}

extern void desenfoque(const char* input_path, const char* name_output, int kernel_size) {
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
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char** in_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** out_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** tmp_rows = malloc(alto * sizeof(unsigned char*));

    for (int i = 0; i < alto; i++) {
        in_rows[i] = malloc(row_padded);
        out_rows[i] = malloc(row_padded);
        tmp_rows[i] = malloc(row_padded);
        fread(in_rows[i], 1, row_padded, image);
    }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            unsigned char b = in_rows[y][x*3];
            unsigned char g = in_rows[y][x*3+1];
            unsigned char r = in_rows[y][x*3+2];
            unsigned char gray = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
            in_rows[y][x*3] = gray;
            in_rows[y][x*3+1] = gray;
            in_rows[y][x*3+2] = gray;
        }
    }

    int k = kernel_size / 2;
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sum = 0, count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) { sum += in_rows[y][nx*3]; count++; }
            }
            tmp_rows[y][x*3] = tmp_rows[y][x*3+1] = tmp_rows[y][x*3+2] = sum/count;
        }
        for (int p = ancho * 3; p < row_padded; p++) tmp_rows[y][p] = in_rows[y][p];
    }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sum = 0, count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) { sum += tmp_rows[ny][x*3]; count++; }
            }
            out_rows[y][x*3] = out_rows[y][x*3+1] = out_rows[y][x*3+2] = sum/count;
        }
        for (int p = ancho * 3; p < row_padded; p++) out_rows[y][p] = tmp_rows[y][p];
    }

    for (int i = 0; i < alto; i++) fwrite(out_rows[i], 1, row_padded, outputImage);

    for(int i=0; i<alto; i++) { free(in_rows[i]); free(tmp_rows[i]); free(out_rows[i]); }
    free(in_rows); free(tmp_rows); free(out_rows);
    fclose(image); fclose(outputImage);
}

#endif
