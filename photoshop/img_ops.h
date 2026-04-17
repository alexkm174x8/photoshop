#ifndef IMG_OPS_H
#define IMG_OPS_H

int bmp_black_white(const char *input_path, const char *output_path);
int bmp_grayscale(const char *input_path, const char *output_path);
int bmp_flip_horizontal(const char *input_path, const char *output_path);
int bmp_flip_vertical(const char *input_path, const char *output_path);
int bmp_blur(const char *input_path, const char *output_path, int kernel_size);
int bmp_blur_grayscale(const char *input_path, const char *output_path, int kernel_size);

int inv_img(const char *output_prefix, const char *input_path);
int inv_img_grey_horizontal(const char *output_prefix, const char *input_path);
int inv_img_color(const char *output_prefix, const char *input_path);
int inv_img_bn_vertical(const char *output_prefix, const char *input_path);
int desenfoque(const char *input_path, const char *output_prefix, int kernel_size);
int desenfoque_gris(const char *input_path, const char *output_prefix, int kernel_size);
int inv_img_color_horizontal(const char *output_prefix, const char *input_path);

#endif
