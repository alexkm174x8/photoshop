#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "img_ops.h"   

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

    // Desactiva el ajuste dinámico para respetar exactamente los hilos solicitados.
    omp_set_dynamic(0);

    // un for para cada configuración de threads 
    for (int config_idx = 0; config_idx < num_configs; config_idx++) {
        int num_threads = thread_configs[config_idx];
        int threads_used = 0;
        printf("Configuracion solicitada: %d threads\n", num_threads);

        #pragma omp parallel num_threads(num_threads)
        {
            #pragma omp single
            {
                threads_used = omp_get_num_threads();
            }
        }
        
        double start_time = omp_get_wtime();
        
        // Data parallelism: cada thread procesa imágenes distintas del arreglo.
        #pragma omp parallel for num_threads(num_threads) schedule(static)
        for (int img_idx = 0; img_idx < num_images; img_idx++) {
            char img_name[50];
            const char *filename = strrchr(images[img_idx], '/') + 1;
            strcpy(img_name, filename);
            char *dot = strrchr(img_name, '.');
            if (dot) *dot = '\0';
            
            // se crea un nuevo nombre para cada imagen que incluya el número de threads usados
            char img_name_with_threads[80];
            snprintf(img_name_with_threads, sizeof(img_name_with_threads), "%s_t%d", img_name, num_threads);

            char output[100];
            snprintf(output, sizeof(output), "results/%s_inv", img_name_with_threads);
            inv_img(output, images[img_idx]);

            snprintf(output, sizeof(output), "results/%s_espejo_gris", img_name_with_threads);
            inv_img_grey_horizontal(output, images[img_idx]);

            snprintf(output, sizeof(output), "results/%s_inv_color", img_name_with_threads);
            inv_img_color(output, images[img_idx]);

            snprintf(output, sizeof(output), "results/%s_inv_bn_vertical", img_name_with_threads);
            inv_img_bn_vertical(output, images[img_idx]);

            snprintf(output, sizeof(output), "results/%s_desenfoque", img_name_with_threads);
            desenfoque(images[img_idx], output, 27);

            snprintf(output, sizeof(output), "results/%s_desenfoque_bn", img_name_with_threads);
            desenfoque_gris(images[img_idx], output, 27);

            snprintf(output, sizeof(output), "results/%s_espejo_color", img_name_with_threads);
            inv_img_color_horizontal(output, images[img_idx]);
        }
        
        double end_time = omp_get_wtime();
        double elapsed_time = end_time - start_time;
        printf("Threads realmente usados: %d\n", threads_used);
        printf("Tiempo: %.4f segundos\n\n", elapsed_time);
    }
    
    return 0;
}