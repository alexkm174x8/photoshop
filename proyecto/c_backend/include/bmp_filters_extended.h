#ifndef BMP_FILTERS_EXTENDED_H
#define BMP_FILTERS_EXTENDED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void manejar_encabezado_1(FILE *in, FILE *out, int *ancho, int *alto, int *offset) {
    unsigned char fileHeader[14];
    fread(fileHeader, 1, 14, in);
    *offset = *(int *)&fileHeader[10];

    unsigned char *fullHeader = (unsigned char *)malloc(*offset);
    memcpy(fullHeader, fileHeader, 14);
    fread(fullHeader + 14, 1, *offset - 14, in);
    fwrite(fullHeader, 1, *offset, out);

    *ancho = *(int *)&fullHeader[18];
    *alto = *(int *)&fullHeader[22];
    free(fullHeader);
}

static void gray_img(const char *output_path, const char *input_path) {
    FILE *image = fopen(input_path, "rb");
    if (!image) {
        return;
    }

    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            unsigned char b = fgetc(image);
            unsigned char g = fgetc(image);
            unsigned char r = fgetc(image);
            unsigned char gray = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
            fputc(gray, outputImage);
            fputc(gray, outputImage);
            fputc(gray, outputImage);
        }
        for (int p = 0; p < padding; p++) {
            fgetc(image);
            fputc(0, outputImage);
        }
    }

    fclose(image);
    fclose(outputImage);
}

static void inv_img_grey_horizontal(const char *output_path, const char *input_path) {
    FILE *image = fopen(input_path, "rb");
    if (!image) {
        return;
    }

    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    unsigned char *pixels = (unsigned char *)malloc(ancho * alto);

    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            unsigned char b = fgetc(image);
            unsigned char g = fgetc(image);
            unsigned char r = fgetc(image);
            pixels[i * ancho + j] = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
        }
        for (int p = 0; p < padding; p++) {
            fgetc(image);
        }
    }

    for (int i = 0; i < alto; i++) {
        for (int j = ancho - 1; j >= 0; j--) {
            unsigned char pxl = pixels[i * ancho + j];
            fputc(pxl, outputImage);
            fputc(pxl, outputImage);
            fputc(pxl, outputImage);
        }
        for (int p = 0; p < padding; p++) {
            fputc(0, outputImage);
        }
    }

    free(pixels);
    fclose(image);
    fclose(outputImage);
}

static void inv_img_color_horizontal(const char *output_path, const char *input_path) {
    FILE *image = fopen(input_path, "rb");
    if (!image) {
        return;
    }

    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    unsigned char *b = (unsigned char *)malloc(ancho * alto);
    unsigned char *g = (unsigned char *)malloc(ancho * alto);
    unsigned char *r = (unsigned char *)malloc(ancho * alto);

    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            b[i * ancho + j] = fgetc(image);
            g[i * ancho + j] = fgetc(image);
            r[i * ancho + j] = fgetc(image);
        }
        for (int p = 0; p < padding; p++) {
            fgetc(image);
        }
    }

    for (int i = 0; i < alto; i++) {
        for (int j = ancho - 1; j >= 0; j--) {
            int idx = i * ancho + j;
            fputc(b[idx], outputImage);
            fputc(g[idx], outputImage);
            fputc(r[idx], outputImage);
        }
        for (int p = 0; p < padding; p++) {
            fputc(0, outputImage);
        }
    }

    free(b);
    free(g);
    free(r);
    fclose(image);
    fclose(outputImage);
}

static void desenfoque(const char *input_path, const char *output_path, int kernel_size) {
    FILE *image = fopen(input_path, "rb");
    if (!image) {
        return;
    }

    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) {
        fclose(image);
        return;
    }

    int ancho, alto, offset;
    manejar_encabezado_1(image, outputImage, &ancho, &alto, &offset);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char **in_rows = (unsigned char **)malloc(alto * sizeof(unsigned char *));
    unsigned char **out_rows = (unsigned char **)malloc(alto * sizeof(unsigned char *));
    unsigned char **tmp_rows = (unsigned char **)malloc(alto * sizeof(unsigned char *));

    for (int i = 0; i < alto; i++) {
        in_rows[i] = (unsigned char *)malloc(row_padded);
        out_rows[i] = (unsigned char *)malloc(row_padded);
        tmp_rows[i] = (unsigned char *)malloc(row_padded);
        fread(in_rows[i], 1, row_padded, image);

        for (int x = 0; x < ancho; x++) {
            unsigned char b = in_rows[i][x * 3];
            unsigned char g = in_rows[i][x * 3 + 1];
            unsigned char r = in_rows[i][x * 3 + 2];
            unsigned char gray = (unsigned char)(0.21 * r + 0.72 * g + 0.07 * b);
            in_rows[i][x * 3] = gray;
            in_rows[i][x * 3 + 1] = gray;
            in_rows[i][x * 3 + 2] = gray;
        }
    }

    int k = kernel_size / 2;
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sum = 0;
            int count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) {
                    sum += in_rows[y][nx * 3];
                    count++;
                }
            }
            unsigned char blurred = (unsigned char)(sum / count);
            tmp_rows[y][x * 3] = blurred;
            tmp_rows[y][x * 3 + 1] = blurred;
            tmp_rows[y][x * 3 + 2] = blurred;
        }
    }

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sum = 0;
            int count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) {
                    sum += tmp_rows[ny][x * 3];
                    count++;
                }
            }
            unsigned char blurred = (unsigned char)(sum / count);
            out_rows[y][x * 3] = blurred;
            out_rows[y][x * 3 + 1] = blurred;
            out_rows[y][x * 3 + 2] = blurred;
        }
        fwrite(out_rows[y], 1, row_padded, outputImage);
    }

    for (int i = 0; i < alto; i++) {
        free(in_rows[i]);
        free(tmp_rows[i]);
        free(out_rows[i]);
    }

    free(in_rows);
    free(tmp_rows);
    free(out_rows);
    fclose(image);
    fclose(outputImage);
}

#endif
