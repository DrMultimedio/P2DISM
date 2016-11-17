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



void equalizar(ImagenBMP *img1, ImagenBMP *img2, int f) {
	int i;
	float v = (float)f / 255;
	for (i = 0; i < img1->tamanyo; i++) {
		img1->datos[i] = (img1->datos[i] - img2->datos[i])*v + img2->datos[i];
	}
};

void equalizarMMX(ImagenBMP *img1, ImagenBMP *img2, ImagenBMP *img3, int f) {
	int i;
	double *puntero1;
	short fade[4];
	unsigned char valor1[4];
	unsigned char valor2[4];
	unsigned char valor3[4];
	for (int j = 0; j < 4; j++) {
		fade[j] = f;
	}
	for (i = 0; i <img1->tamanyo; i += 4) //De 4 en 4, ya que leemos 4 pixels cada vez
	{

		for (int k = 0; k < 4; k++) {
			valor1[k] = img1->datos[i + k];
			valor2[k] = img2->datos[i + k];

		}
		_asm {

			movq mm1, valor1 //almacena en el registro mm1 4 pixeles de A
			pxor mm2, mm2 // inicializamos el registro mm2 a 0 para evitar problemas
			movq mm3, valor2 //almacena en el registro mm3 4 pixeles de la imagen B
			movq mm0, fade // almacena en el registro mm0 los 4 valores de fade

			punpcklbw mm3, mm2 //desempaquetamos B 
			punpcklbw mm1, mm2 //desempaquetamos A 

			psubusw mm1, mm3 //resta la imagen A con B, se guarda en MM1

			pmullw mm1, mm0 //muliplica los pixeles de la resta anterior por el fade, se almacena en mm1
			psrlw mm1, 8  // desplazamiento a la derecha de 8 bits para seleccionar la parte alta 
			paddusw mm1, mm3  // suma la imagen a la imagen b desempaquetada 

			packuswb mm1, mm2  //desempaqueta los bits a pixeles. Seran lso pixeles resultantes
			movq puntero1, mm1 //copia los pixeles de mm1 y los almacena en la variable puntero1
		}

		
		memcpy(valor3, &puntero1, 4); //copia en memoria los valores que hay en puntoResultado
		for (int k = 0; k < 4; k++) {

			img3->datos[i + k] = valor3[k];

		}
	} //end of for
	_asm

	{
		emms //Finalizar utilización de registros MMX	
	}
}; //end of equalizarmmx

int main(int argc, char **argv) {
	short fade = 0; //el tercer parametro, el fade
	int i = 0;
	struct stat buf;
	ImagenBMP img3;
	ImagenBMP img1;
	ImagenBMP img2;
	int resultado;
	LARGE_INTEGER t_ini, t_fin;		//Para contar tiempo de proceso
	char *img_Salida;

	double secs;
	if (argc < 4) {
		fprintf(stderr, "Uso incorrecto de los parametros.\n");
		exit(1);
	}
	//declaramos las dos imagenes a las que vamos a hacer equalizar
	img_Salida = (char*)calloc(8 + strlen(argv[1]), 1);
	sprintf(img_Salida, "salida_%s", argv[1]);
	if ((stat(argv[1], &buf) == 0) && (stat(argv[2], &buf) == 0)) {
		fprintf(stderr, "Procesando imagenenes: %s y %s... ", argv[1], argv[2]);
		leerBMP(&img1, argv[1]);
		leerBMP(&img2, argv[2]);

		fade = atoi(argv[3]); //el factor de equalizar
		img3 = img1;
		//-------------------- ALTO NIVEL ----------------------------
		QueryPerformanceCounter(&t_ini);
		//for (i=0; i<100; i++) //100 pruebas para poder obtener tiempos medibles
		equalizar(&img1, &img2, fade);
		QueryPerformanceCounter(&t_fin);
		secs = performancecounter_diff(&t_fin, &t_ini);

		fprintf(stderr, "FIN. CORRECTO. TIEMPO = %f\n", secs);
		escribirBMP(&img1, img_Salida);
		//-------------------- MMX ----------------------------
		QueryPerformanceCounter(&t_ini);
		equalizarMMX(&img1, &img2, &img3, fade);
		QueryPerformanceCounter(&t_fin);
		secs = performancecounter_diff(&t_fin, &t_ini);

		fprintf(stderr, "FIN MMX. CORRECTO. TIEMPO = %f\n", secs);
		sprintf(img_Salida, "salidammx_%s", argv[1]);
		escribirBMP(&img3, img_Salida);
		exit(0);
	}
	else {
		fprintf(stderr, "No existe el fichero o directorio indicado\n");
		exit(1);
	}
	return 0;
} //end of main
