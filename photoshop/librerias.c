#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "img_ops.h"   

#define NUM_THREADS 6

int main() {
    FILE *fptr;
    char data[80] = "results/arc1.txt";
    system("mkdir -p results");
    fptr = fopen(data, "w");
    if (fptr == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    fprintf(fptr, "Ejemplo escribir en un archivo");
    fclose(fptr);

    // se crea un array para guardar las rutas de las imágenes a procesar
    const char *images[] = {
        "./img/miri.bmp",
        "./img/kong.bmp",
        "./img/fany.bmp",
        "./img/sofi.bmp"
    };
    int num_images = 4;

    // Array de treads a probar
    int thread_configs[] = {6, 12, 18};
    int num_configs = 3;

    // un for para cada configuración de threads 
    for (int config_idx = 0; config_idx < num_configs; config_idx++) {
        int num_threads = thread_configs[config_idx];
        printf("%d threads:\n", num_threads);
        
        clock_t start_time = clock();
        
        // otro for para procesar cada imagen con esa configuración de threads
        for (int img_idx = 0; img_idx < num_images; img_idx++) {
            char img_name[50];
            const char *filename = strrchr(images[img_idx], '/') + 1;
            strcpy(img_name, filename);
            char *dot = strrchr(img_name, '.');
            if (dot) *dot = '\0';
            
            // se crea un nuevo nombre para cada imagen que incluya el número de threads usados
            char img_name_with_threads[80];
            sprintf(img_name_with_threads, "%s_t%d", img_name, num_threads);
            
            #pragma omp parallel num_threads(num_threads)
            {
                #pragma omp sections
                {
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_inv", img_name_with_threads);
                        inv_img(output, images[img_idx]);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_espejo_gris", img_name_with_threads);
                        inv_img_grey_horizontal(output, images[img_idx]);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_inv_color", img_name_with_threads);
                        inv_img_color(output, images[img_idx]);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_inv_bn_vertical", img_name_with_threads);
                        inv_img_bn_vertical(output, images[img_idx]);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_desenfoque", img_name_with_threads);
                        desenfoque(images[img_idx], output, 27);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_desenfoque_bn", img_name_with_threads);
                        desenfoque_gris(images[img_idx], output, 27);
                    }
                    #pragma omp section
                    {
                        char output[100];
                        sprintf(output, "results/%s_espejo_color", img_name_with_threads);
                        inv_img_color_horizontal(output, images[img_idx]);
                    }
                }
            }
        }
        
        clock_t end_time = clock();
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("%.2f segundos\n", elapsed_time);
    }
    
    return 0;
}