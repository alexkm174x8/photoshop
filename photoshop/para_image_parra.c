#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"

#define NUM_THREADS 18


int main(){
    clock_t inicio = clock();
    omp_set_num_threads(NUM_THREADS);
    
    // Archivo de registro inicial (tomado de tu código original)
    FILE *fptr;
    char data[80] = "arc1.txt";
    fptr = fopen(data, "w");
    if (fptr != NULL){
        fprintf(fptr, "Ejemplo escribir\n");
        fprintf(fptr, "Estefania Antonio Villaseca\n");
        fclose(fptr);
    } else {
        printf("Error al abrir arc1.txt\n");
    }

    // 1. Definimos las rutas de las 4 imágenes de entrada
    const char *imagenes_entrada[4] = {
        "./img/prueba1.bmp",
        "./img/prueba2.bmp",
        "./img/prueba3.bmp",
        "./img/prueba4.bmp"
    };

    // 2. Definimos un prefijo para cada imagen para que los archivos de salida no se sobrescriban
    const char *prefijos[4] = {
        "img1",
        "img2",
        "img3",
        "img4"
    };

    // 3. Ciclo principal que procesa una imagen a la vez
    for (int i = 0; i < 4; i++) {

        // Cadenas para almacenar los nombres de salida dinámicos
        char out_inv[50], out_espejo[50], out_inv_color[50];
        char out_espejo_color[50], out_desenfoque[50], out_desenfoque_color[50];

        // Construimos los nombres de salida concatenando el prefijo
        sprintf(out_inv, "%s_inv_1", prefijos[i]);
        sprintf(out_espejo, "%s_espejo", prefijos[i]);
        sprintf(out_inv_color, "%s_inv_color_1", prefijos[i]);
        sprintf(out_espejo_color, "%s_espejo_color", prefijos[i]);
        sprintf(out_desenfoque, "%s_desenfoque", prefijos[i]);
        sprintf(out_desenfoque_color, "%s_desenfoque_color", prefijos[i]);

        // 4. Las transformaciones DE ESTA IMAGEN se hacen en paralelo
        #pragma omp parallel
        {
            #pragma omp sections
            {
                #pragma omp section
                inv_img(out_inv, imagenes_entrada[i]); 
                
                #pragma omp section
                inv_img_grey_horizontal(out_espejo, imagenes_entrada[i]); 
                
                #pragma omp section
                inv_img_color(out_inv_color, imagenes_entrada[i]); 

                #pragma omp section
                inv_img_color_horizontal(out_espejo_color, imagenes_entrada[i]); 
                
                #pragma omp section
                desenfoque(imagenes_entrada[i], out_desenfoque, 27); 

                #pragma omp section
                desenfoque_color(imagenes_entrada[i], out_desenfoque_color, 27);
            }
        }
    }
    
    clock_t fin = clock();
    double tiempo_total = (double)(fin - inicio) / CLOCKS_PER_SEC;
    printf("Tiempo total de ejecución: %.2f segundos\n", tiempo_total);
    
    return 0;
}
