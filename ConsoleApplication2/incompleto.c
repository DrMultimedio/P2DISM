#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <Windows.h>   //Para LARGE_INTEGER
#include "imagenBMP.h"

 
// Retorna (a - b) en segundos
double performancecounter_diff(LARGE_INTEGER *a, LARGE_INTEGER *b)
{
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return (double)(a->QuadPart - b->QuadPart) / (double)freq.QuadPart;
}



void equalizar(ImagenBMP *img) {

    int p, vp, m = 1;
    int max = 0, min = 255;
        
    for(p = 0; p < img->tamanyo; m++ ) {                
        vp = img->datos[p];   
        if ( vp < min ) 
            min = vp;	//Hallar minimo valor de gris
        else
        if ( vp > max )
            max = vp;	//Hallar maximo valor de gris
        
        //Si hemos llegado al final de la fila,
        //saltamos al siguiente byte de datos
        if ( m % img->ancho == 0 ) 
            p += img->padding + 1;	//Los bits de padding son "de relleno"
        else
            p++;        
    }
    
    //Hacemos la ecualizacion
    //Aqui no tenemos en cuenta los bytes de padding ya que no importa
    //el valor que contengan, asi que los modificaremos tambien
    for(p = 0; p < img->tamanyo; p++)
        img->datos[p] = 255 * (img->datos[p] - min) / (max - min);

}


void equalizarMMX(ImagenBMP *img) {

    int p, vp, m = 1, j;
    int max = 0, min = 255;
	int ctecorreccion;
	short vctecorreccion[4];	// 16 x [4] bits
	unsigned char vmin[8];		//  8 x [8] bits
	double *puntero;
        
    for(p = 0; p < img->tamanyo; m++ ) {                
        vp = img->datos[p];   
        if ( vp < min ) 
            min = vp;
        else
        if ( vp > max )
            max = vp;
        
        //Si hemos llegado al final de la fila
        //saltamos al siguiente byte de datos
        if ( m % img->ancho == 0 ) 
            p += img->padding + 1;
        else
            p++;        
    }
    
    //Hacemos la ecualizacion
    //Aqui no tenemos en cuenta los bytes de padding ya que no importa
    //el valor que contengan, asi que los modificaremos tambien
    //************* INCLUIR AQUI EL CODIGO MMX !!!!!!!!!!!!!!!!!!!!!!!  

	ctecorreccion = 255 / (max - min);
	for (j = 0; j < 8; j++)
		vmin[j] = min;
	for (j = 0; j < 4; j++) {
		vctecorreccion[j] = ctecorreccion;
	}
	for (p = 0; p < img->tamanyo; p += 8) {
		puntero = (double*)&img->datos[p];
		_asm {
			movq mm1, puntoA //almacena en el registro mm1 4 pixeles de A
			movq mm2, puntoB //almacena en el registro mm2 4 pixeles de la imagen B
			movq mm3, vFade // almacena en el registro mm3 los 4 valores de fade
			pxor mm0, mm0 // inicializamos el registro mm0 a 0 para evitar problemas

						  //tenemos ue desempaquetar mm1 y mm2 para quer ocupen lo mismo que mm3
						  punpcklbw mm1, mm0  //desempaqueta la imagen A, pasa de 8 a 16 bits
						  punpcklbw mm2, mm0 //desempaqueta la imagen B

						  psubusw mm1, mm2 //resta la imagen A con B, se guarda en MM1

						  pmullw mm1, mm3 //muliplica los pixeles de la resta anterior por el fade, se almacena en mm1
						  parlw mm1, 8 // desplazamiento a la derecha de 8 bits para seleccionar la parte alta del 

						  paddsw mm1, mm2 // suma la imagen a la imagen b desempaquetada 

						  packuswb mm1, mm0 //desempaqueta los bits a pixeles. Seran lso pixeles resultantes
						  movq puntoResultado, mm1 //copia los pixeles de mm1 y los almacena en la variable punto resultado
		}
		memcpy(resultado, &puntoResultado, 4); //copia en memoria los valores que hay en puntoResultado

	}
	_asm
	{
		emms
	}

}


int main(int argc, char **argv) {

    int i=0;
    struct stat buf;  
	char *dSalida;
	ImagenBMP img;
    
	LARGE_INTEGER t_ini, t_fin;		//Para contar tiempo de proceso
    double secs;

    if (argc < 2) {
        fprintf(stderr,"Uso incorrecto de los parametros.\n");
        exit(1);
    }
    
    dSalida = (unsigned char*)calloc( 8 + strlen(argv[1]), 1);
    sprintf(dSalida, "salida_%s", argv[1]);    
    
    if (stat(argv[1], &buf) == 0) {
        
            fprintf(stderr, "Procesando imagen: %s ... ", argv[1]);
            leerBMP(&img, argv[1]);            
            
			//-------------------- ALTO NIVEL ----------------------------
			QueryPerformanceCounter(&t_ini);
			for (i=0; i<100; i++) //100 pruebas para poder obtener tiempos medibles
				equalizar(&img);
			QueryPerformanceCounter(&t_fin);
			secs = performancecounter_diff(&t_fin, &t_ini);
			                    
            fprintf(stderr, "FIN. CORRECTO. TIEMPO = %f\n",secs);
            escribirBMP(&img, dSalida);
            
			//-------------------- MMX ----------------------------
			
			QueryPerformanceCounter(&t_ini);
			for (i=0; i<100; i++) //100 pruebas para poder obtener tiempos medibles
				equalizarMMX(&img);
			QueryPerformanceCounter(&t_fin);
			secs = performancecounter_diff(&t_fin, &t_ini);
			                    
            fprintf(stderr, "FIN MMX. CORRECTO. TIEMPO = %f\n",secs);
            sprintf(dSalida, "salidammx_%s", argv[1]);    
			escribirBMP(&img, dSalida);
            exit(0);
    }
    else {
        fprintf(stderr, "No existe el fichero o directorio indicado\n");
        exit(1);        
    }
    
        
    return 0;
}