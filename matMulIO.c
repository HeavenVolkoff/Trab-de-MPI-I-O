#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define INT 1
#define FLOAT 2

int min(int a, int b){
	if(a < b){
		return a;
	}
	return b;
}

int main(int argc, char** argv){
	int rank, nProc, i, j, k, file_type_size, size_int, offset;
	int amount = 0, verbose = 0;
	int params[2];
	int* m1BufInt, *m2BufInt, *resBufInt;
	float* m1BufFloat, *m2BufFloat, *resBufFloat;
	MPI_File saveFile;
	MPI_File resultsFile;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);

	if(argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'v'){
		verbose = 1;
	}
	else{
		if(rank == 0){
			printf("If you wish to see the program's log, pass \'-v\' as the last parameter\n");
		}
	}

	MPI_File_open(MPI_COMM_WORLD, "matrizes.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &saveFile);

	if(rank == 0){
		MPI_File_read(saveFile, params, 2, MPI_INT, MPI_STATUS_IGNORE);
		if(verbose){
			printf("Read quantity = %d\n", params[0]);
			printf("Read type = %d\n", params[1]);
			printf("\n\n\n");
		}
	}

	MPI_Bcast(params, 2, MPI_INT, 0, MPI_COMM_WORLD);

	if(verbose){
		printf("recebi %d e %d - from process %d\n", params[0], params[1], rank);
	}
	
	amount = params[0] / nProc;
	if(params[0] % nProc != 0 && params[0] % nProc > rank){
		amount++;
	}

	offset = (params[0] / nProc) * rank; //Cada processo pega pelo menos params[0] / nProc linhas da primeira matriz.
	if(params[0] % nProc != 0){ //Ajusta o offset caso a divisão não seja exata.
		offset += min(rank, params[0] % nProc);
	}
	offset *= params[0];//Multiplicamos a quantidade de linhas (que já está guardada em offset) pelo número de elementos em cada linha.

	if(params[1] == INT){
		offset += 2; //Leva em conta os dois primeiros números (quantidade de pares e o tipo).
		m1BufInt = (int*) malloc((amount * params[0]) * sizeof(int));
		m2BufInt = (int*) malloc((params[0] * params[0]) * sizeof(int));
		resBufInt = (int*) malloc((amount * params[0]) * sizeof(int));
		for(i = 0; i < amount * params[0]; i++){
			resBufInt[i] = 0;
		}
		MPI_Type_size(MPI_INT, &file_type_size);
		MPI_File_seek(saveFile, offset * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, m1BufInt, amount * params[0], MPI_INT, MPI_STATUS_IGNORE);
		if(verbose){
			for(i = 0; i < amount; i++){
				for(j = 0; j < params[0]; j++){
					printf("Read %d - from process %d\n", m1BufInt[i * params[0] + j], rank);	
				}
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);
		MPI_File_seek(saveFile, (2 + params[0] * params[0]) * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, m2BufInt, params[0] * params[0], MPI_INT, MPI_STATUS_IGNORE);
		MPI_File_close(&saveFile);
		if(verbose){
			if(rank == 0){
				printf("\nMatrix 2:\n");
				for(i = 0; i < params[0]; i++){
					for(j = 0; j < params[0]; j++){
						printf(" %4d", m2BufInt[i * params[0] + j]);	
					}
					printf(" - from process %d\n", rank);
				}
				// for(i = 0; i < params[0] * params[0]; i++){
				// 	printf("M2 => %d - from process %d\n", m2BufInt[i], rank);
				// }
			}
		}
		//Multiplica parte das matrizes
		for(i = 0; i < amount; i++){
			for(j = 0; j < params[0]; j++){
				for(k = 0; k < params[0]; k++){
					resBufInt[i * params[0] + j] += m1BufInt[i * params[0] + k] * m2BufInt[k * params[0] + j];
				}
			}
		}
		if(verbose){
			for(k = 0; k < nProc; k++){
				if(rank == k){
					printf("\nPartial Result Matrix:\n");
					for(i = 0; i < amount; i++){
						for(j = 0; j < params[0]; j++){
							printf(" %4d", resBufInt[i * params[0] + j]);	
						}
						printf(" - from process %d\n", rank);
					}
					// for(i = 0; i < params[0] * params[0]; i++){
					// 	printf("M2 => %d - from process %d\n", m2BufInt[i], rank);
					// }
				}
				MPI_Barrier(MPI_COMM_WORLD);
			}
		}

		MPI_File_open(MPI_COMM_WORLD, "matrizes.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &saveFile);
		offset = (2 * params[1] * params[1] / nProc) * rank; //Geramos pelo menos 2 * (params[1] * params[1] / nProc) números por processo.
		if((2 * params[1] * params[1]) % nProc != 0){ //Ajusta o offset caso a divisão não seja exata.
			offset += min(rank, (2 * params[1] * params[1]) % nProc);
		}
		
		MPI_File_seek(saveFile, offset * file_type_size, MPI_SEEK_SET);
		//Escreve os números do buffer no arquivo.
		MPI_File_write(saveFile, bufferInt, amount, MPI_INT, MPI_STATUS_IGNORE);
	}
	else{
		m1BufFloat = (float*) malloc((amount * params[0]) * sizeof(float));
		m2BufFloat = (float*) malloc((params[0] * params[0]) * sizeof(float));
		MPI_Type_size(MPI_INT, &size_int);
		MPI_Type_size(MPI_FLOAT, &file_type_size);
		MPI_File_seek(saveFile, (2 * size_int) + offset * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, m1BufFloat, amount * params[0], MPI_FLOAT, MPI_STATUS_IGNORE);
		if(verbose){
			for(i = 0; i < amount; i++){
				for(j = 0; j < params[0]; j++){
					printf("Read %f - from process %d\n", m1BufFloat[i * params[0] + j], rank);	
				}
			}
		}


		MPI_File_seek(saveFile, 2 * size_int + params[0] * params[0] * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, m2BufFloat, params[0] * params[0], MPI_FLOAT, MPI_STATUS_IGNORE);
		if(verbose){
			if(rank == 0){
				printf("\nMatrix 2:\n");
				for(i = 0; i < params[0]; i++){
					for(j = 0; j < params[0]; j++){
						printf(" %4.4f", m2BufFloat[i * params[0] + j]);	
					}
					printf(" - from process %d\n", rank);
				}
				// for(i = 0; i < params[0] * params[0]; i++){
				// 	printf("M2 => %d - from process %d\n", m2BufFloat[i], rank);
				// }
			}
		}
	}

	return 0;
}