#include "img_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char header[54];
    unsigned char *preamble;
    int pixel_offset;
    int width;
    int height;
    int row_size;
    int data_size;
    unsigned char *data;
} Bmp24;

static void bmp_free(Bmp24 *bmp) {
    free(bmp->preamble);
    free(bmp->data);
    bmp->preamble = NULL;
    bmp->data = NULL;
}

static int bmp_read_24(const char *path, Bmp24 *bmp) {
    FILE *f = fopen(path, "rb");
    int bits_per_pixel;
    int compression;

    memset(bmp, 0, sizeof(Bmp24));
    if (f == NULL) {
        return 0;
    }

    if (fread(bmp->header, 1, 54, f) != 54) {
        fclose(f);
        return 0;
    }

    if (bmp->header[0] != 'B' || bmp->header[1] != 'M') {
        fclose(f);
        return 0;
    }

    bmp->width = *(int *)&bmp->header[18];
    bmp->height = *(int *)&bmp->header[22];
    bmp->pixel_offset = *(int *)&bmp->header[10];
    bits_per_pixel = *(short *)&bmp->header[28];
    compression = *(int *)&bmp->header[30];

    if (bmp->width <= 0 || bmp->height == 0 || bits_per_pixel != 24 || compression != 0 || bmp->pixel_offset < 54) {
        fclose(f);
        return 0;
    }

    if (bmp->height < 0) {
        bmp->height = -bmp->height;
    }

    bmp->row_size = ((bmp->width * 3 + 3) / 4) * 4;
    bmp->data_size = bmp->row_size * bmp->height;

    bmp->preamble = (unsigned char *)malloc((size_t)bmp->pixel_offset);
    if (bmp->preamble == NULL) {
        fclose(f);
        return 0;
    }

    fseek(f, 0, SEEK_SET);
    if (fread(bmp->preamble, 1, (size_t)bmp->pixel_offset, f) != (size_t)bmp->pixel_offset) {
        fclose(f);
        bmp_free(bmp);
        return 0;
    }

    bmp->data = (unsigned char *)malloc((size_t)bmp->data_size);
    if (bmp->data == NULL) {
        fclose(f);
        bmp_free(bmp);
        return 0;
    }

    if (fread(bmp->data, 1, (size_t)bmp->data_size, f) != (size_t)bmp->data_size) {
        fclose(f);
        bmp_free(bmp);
        return 0;
    }

    fclose(f);
    return 1;
}

static int bmp_write_24(const char *path, const Bmp24 *bmp, const unsigned char *data) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return 0;
    }

    if (fwrite(bmp->preamble, 1, (size_t)bmp->pixel_offset, f) != (size_t)bmp->pixel_offset) {
        fclose(f);
        return 0;
    }

    if (fwrite(data, 1, (size_t)bmp->data_size, f) != (size_t)bmp->data_size) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

static unsigned char to_gray(unsigned char b, unsigned char g, unsigned char r) {
    return (unsigned char)(0.114f * b + 0.587f * g + 0.299f * r);
}

static void make_output_name(const char *prefix, char *out, size_t out_size) {
    size_t len = strlen(prefix);
    if (len >= 4 && strcmp(prefix + len - 4, ".bmp") == 0) {
        snprintf(out, out_size, "%s", prefix);
    } else {
        snprintf(out, out_size, "%s.bmp", prefix);
    }
}

static void invert_colors_in_place(Bmp24 *bmp) {
    int y;
    int x;
    for (y = 0; y < bmp->height; y++) {
        for (x = 0; x < bmp->width; x++) {
            int idx = y * bmp->row_size + x * 3;
            bmp->data[idx] = (unsigned char)(255 - bmp->data[idx]);
            bmp->data[idx + 1] = (unsigned char)(255 - bmp->data[idx + 1]);
            bmp->data[idx + 2] = (unsigned char)(255 - bmp->data[idx + 2]);
        }
    }
}

int bmp_black_white(const char *input_path, const char *output_path) {
    Bmp24 bmp;
    int y;
    int x;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int idx = y * bmp.row_size + x * 3;
            unsigned char gray = to_gray(bmp.data[idx], bmp.data[idx + 1], bmp.data[idx + 2]);
            unsigned char bw = (gray >= 128) ? 255 : 0;
            bmp.data[idx] = bw;
            bmp.data[idx + 1] = bw;
            bmp.data[idx + 2] = bw;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int bmp_grayscale(const char *input_path, const char *output_path) {
    Bmp24 bmp;
    int y;
    int x;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int idx = y * bmp.row_size + x * 3;
            unsigned char gray = to_gray(bmp.data[idx], bmp.data[idx + 1], bmp.data[idx + 2]);
            unsigned char bw = (gray >= 128) ? 255 : 0;
            bmp.data[idx] = bw;
            bmp.data[idx + 1] = bw;
            bmp.data[idx + 2] = bw;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int bmp_flip_horizontal(const char *input_path, const char *output_path) {
    Bmp24 bmp;
    int y;
    int x;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width / 2; x++) {
            int left = y * bmp.row_size + x * 3;
            int right = y * bmp.row_size + (bmp.width - 1 - x) * 3;
            unsigned char tb = bmp.data[left];
            unsigned char tg = bmp.data[left + 1];
            unsigned char tr = bmp.data[left + 2];

            bmp.data[left] = bmp.data[right];
            bmp.data[left + 1] = bmp.data[right + 1];
            bmp.data[left + 2] = bmp.data[right + 2];

            bmp.data[right] = tb;
            bmp.data[right + 1] = tg;
            bmp.data[right + 2] = tr;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int bmp_flip_vertical(const char *input_path, const char *output_path) {
    Bmp24 bmp;
    unsigned char *tmp_row;
    int y;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    tmp_row = (unsigned char *)malloc((size_t)bmp.row_size);
    if (tmp_row == NULL) {
        bmp_free(&bmp);
        return 0;
    }

    for (y = 0; y < bmp.height / 2; y++) {
        int top = y * bmp.row_size;
        int bottom = (bmp.height - 1 - y) * bmp.row_size;
        memcpy(tmp_row, bmp.data + top, (size_t)bmp.row_size);
        memcpy(bmp.data + top, bmp.data + bottom, (size_t)bmp.row_size);
        memcpy(bmp.data + bottom, tmp_row, (size_t)bmp.row_size);
    }

    free(tmp_row);
    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int bmp_blur(const char *input_path, const char *output_path, int kernel_size) {
    Bmp24 bmp;
    unsigned char *out_data;
    int radius;
    int y;
    int x;

    if (kernel_size < 1) {
        kernel_size = 1;
    }
    if (kernel_size % 2 == 0) {
        kernel_size += 1;
    }
    radius = kernel_size / 2;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    out_data = (unsigned char *)malloc((size_t)bmp.data_size);
    if (out_data == NULL) {
        bmp_free(&bmp);
        return 0;
    }
    memcpy(out_data, bmp.data, (size_t)bmp.data_size);

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int ky;
            int kx;
            int count = 0;
            int sum_b = 0;
            int sum_g = 0;
            int sum_r = 0;
            int out_idx = y * bmp.row_size + x * 3;

            for (ky = -radius; ky <= radius; ky++) {
                int ny = y + ky;
                if (ny < 0 || ny >= bmp.height) {
                    continue;
                }
                for (kx = -radius; kx <= radius; kx++) {
                    int nx = x + kx;
                    int in_idx;
                    if (nx < 0 || nx >= bmp.width) {
                        continue;
                    }
                    in_idx = ny * bmp.row_size + nx * 3;
                    sum_b += bmp.data[in_idx];
                    sum_g += bmp.data[in_idx + 1];
                    sum_r += bmp.data[in_idx + 2];
                    count++;
                }
            }

            if (count > 0) {
                out_data[out_idx] = (unsigned char)(sum_b / count);
                out_data[out_idx + 1] = (unsigned char)(sum_g / count);
                out_data[out_idx + 2] = (unsigned char)(sum_r / count);
            }
        }
    }

    x = bmp_write_24(output_path, &bmp, out_data);
    free(out_data);
    bmp_free(&bmp);
    return x;
}

int bmp_blur_grayscale(const char *input_path, const char *output_path, int kernel_size) {
    Bmp24 bmp;
    unsigned char *out_data;
    int radius;
    int y;
    int x;

    if (kernel_size < 1) {
        kernel_size = 1;
    }
    if (kernel_size % 2 == 0) {
        kernel_size += 1;
    }
    radius = kernel_size / 2;

    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    /* Convert first to grayscale, then blur grayscale values. */
    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int idx = y * bmp.row_size + x * 3;
            unsigned char gray = to_gray(bmp.data[idx], bmp.data[idx + 1], bmp.data[idx + 2]);
            bmp.data[idx] = gray;
            bmp.data[idx + 1] = gray;
            bmp.data[idx + 2] = gray;
        }
    }

    out_data = (unsigned char *)malloc((size_t)bmp.data_size);
    if (out_data == NULL) {
        bmp_free(&bmp);
        return 0;
    }
    memcpy(out_data, bmp.data, (size_t)bmp.data_size);

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int ky;
            int kx;
            int count = 0;
            int sum_gray = 0;
            int out_idx = y * bmp.row_size + x * 3;

            for (ky = -radius; ky <= radius; ky++) {
                int ny = y + ky;
                if (ny < 0 || ny >= bmp.height) {
                    continue;
                }
                for (kx = -radius; kx <= radius; kx++) {
                    int nx = x + kx;
                    int in_idx;
                    if (nx < 0 || nx >= bmp.width) {
                        continue;
                    }
                    in_idx = ny * bmp.row_size + nx * 3;
                    sum_gray += bmp.data[in_idx];
                    count++;
                }
            }

            if (count > 0) {
                unsigned char gray = (unsigned char)(sum_gray / count);
                out_data[out_idx] = gray;
                out_data[out_idx + 1] = gray;
                out_data[out_idx + 2] = gray;
            }
        }
    }

    x = bmp_write_24(output_path, &bmp, out_data);
    free(out_data);
    bmp_free(&bmp);
    return x;
}

int inv_img(const char *output_prefix, const char *input_path) {
    char output_path[512];
    make_output_name(output_prefix, output_path, sizeof(output_path));
    return bmp_black_white(input_path, output_path);
}

int inv_img_grey_horizontal(const char *output_prefix, const char *input_path) {
    Bmp24 bmp;
    int y;
    int x;
    char output_path[512];

    make_output_name(output_prefix, output_path, sizeof(output_path));
    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int idx = y * bmp.row_size + x * 3;
            unsigned char gray = to_gray(bmp.data[idx], bmp.data[idx + 1], bmp.data[idx + 2]);
            bmp.data[idx] = gray;
            bmp.data[idx + 1] = gray;
            bmp.data[idx + 2] = gray;
        }
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width / 2; x++) {
            int left = y * bmp.row_size + x * 3;
            int right = y * bmp.row_size + (bmp.width - 1 - x) * 3;
            unsigned char tb = bmp.data[left];
            unsigned char tg = bmp.data[left + 1];
            unsigned char tr = bmp.data[left + 2];
            bmp.data[left] = bmp.data[right];
            bmp.data[left + 1] = bmp.data[right + 1];
            bmp.data[left + 2] = bmp.data[right + 2];
            bmp.data[right] = tb;
            bmp.data[right + 1] = tg;
            bmp.data[right + 2] = tr;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int inv_img_color(const char *output_prefix, const char *input_path) {
    char output_path[512];
    make_output_name(output_prefix, output_path, sizeof(output_path));
    return bmp_flip_vertical(input_path, output_path);
}

int inv_img_bn_vertical(const char *output_prefix, const char *input_path) {
    Bmp24 bmp;
    int y;
    int x;
    char output_path[512];

    make_output_name(output_prefix, output_path, sizeof(output_path));
    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width; x++) {
            int idx = y * bmp.row_size + x * 3;
            unsigned char gray = to_gray(bmp.data[idx], bmp.data[idx + 1], bmp.data[idx + 2]);
            unsigned char bw = (gray >= 128) ? 255 : 0;
            bmp.data[idx] = bw;
            bmp.data[idx + 1] = bw;
            bmp.data[idx + 2] = bw;
        }
    }

    for (y = 0; y < bmp.height / 2; y++) {
        int top = y * bmp.row_size;
        int bottom = (bmp.height - 1 - y) * bmp.row_size;
        int i;
        for (i = 0; i < bmp.row_size; i++) {
            unsigned char tmp = bmp.data[top + i];
            bmp.data[top + i] = bmp.data[bottom + i];
            bmp.data[bottom + i] = tmp;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}

int desenfoque(const char *input_path, const char *output_prefix, int kernel_size) {
    char output_path[512];
    make_output_name(output_prefix, output_path, sizeof(output_path));
    return bmp_blur(input_path, output_path, kernel_size);
}

int desenfoque_gris(const char *input_path, const char *output_prefix, int kernel_size) {
    char output_path[512];
    make_output_name(output_prefix, output_path, sizeof(output_path));
    return bmp_blur_grayscale(input_path, output_path, kernel_size);
}

int inv_img_color_horizontal(const char *output_prefix, const char *input_path) {
    Bmp24 bmp;
    int y;
    int x;
    char output_path[512];

    make_output_name(output_prefix, output_path, sizeof(output_path));
    if (!bmp_read_24(input_path, &bmp)) {
        return 0;
    }

    for (y = 0; y < bmp.height; y++) {
        for (x = 0; x < bmp.width / 2; x++) {
            int left = y * bmp.row_size + x * 3;
            int right = y * bmp.row_size + (bmp.width - 1 - x) * 3;
            unsigned char tb = bmp.data[left];
            unsigned char tg = bmp.data[left + 1];
            unsigned char tr = bmp.data[left + 2];
            bmp.data[left] = bmp.data[right];
            bmp.data[left + 1] = bmp.data[right + 1];
            bmp.data[left + 2] = bmp.data[right + 2];
            bmp.data[right] = tb;
            bmp.data[right + 1] = tg;
            bmp.data[right + 2] = tr;
        }
    }

    y = bmp_write_24(output_path, &bmp, bmp.data);
    bmp_free(&bmp);
    return y;
}
