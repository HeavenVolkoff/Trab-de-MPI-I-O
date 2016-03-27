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
	int rank, nProc, i, file_type_size, offset;
	float mySUMx = 0, mySUMy = 0, mySUMxy = 0, mySUMxx = 0;
	float totalSUMx = 0, totalSUMy = 0, totalSUMxy = 0, totalSUMxx = 0;
	float slope, y_intercept;
	int amount = 0, verbose = 0;
	int params[2];
	int* readBufInt;
	float* readBufFloat;
	MPI_File saveFile;

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

	MPI_File_open(MPI_COMM_WORLD, "pontos.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &saveFile);

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
	
	amount += params[0] / nProc;
	if(params[0] % nProc != 0 && params[0] % nProc > rank){
		amount++;
	}

	offset = 2 * (params[0] / nProc) * rank; //Geramos params[1] pares de números.
	if(params[0] % nProc != 0){ //Ajusta o offset caso a divisão não seja exata.
		offset += 2 * min(rank, params[0] % nProc);
	}
	offset += 2; //Leva em conta os dois primeiros números (quantidade de pares e o tipo).

	if(params[1] == INT){
		readBufInt = (int*) malloc((2 * params[0]) * sizeof(int));
		MPI_Type_size(MPI_INT, &file_type_size);
		MPI_File_seek(saveFile, offset * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, readBufInt, 2 * amount, MPI_INT, MPI_STATUS_IGNORE);
		if(verbose){
			for(i = 0; i < 2 * amount; i++){
				printf("Read %d - from process %d\n", readBufInt[i], rank);
			}
		}
		
		for (i = 0; i < 2 * amount; i+=2) {
	    	mySUMx = mySUMx + readBufInt[i];
	    	mySUMy = mySUMy + readBufInt[i + 1];
	    	mySUMxy = mySUMxy + readBufInt[i] * readBufInt[i + 1];
	    	mySUMxx = mySUMxx + readBufInt[i] * readBufInt[i];
	  	}
	  	if(verbose){
	  		printf("mySUMx = %f; mySUMy = %f; mySUMxy = %f; mySUMxy = %f - from process %d\n", mySUMx, mySUMy, mySUMxy, mySUMxx, rank);
	  	}
	}
	else{
		readBufFloat = (float*) malloc((2 * params[0]) * sizeof(float));
		MPI_Type_size(MPI_INT, &file_type_size);
		MPI_File_seek(saveFile, offset * file_type_size, MPI_SEEK_SET);
		MPI_File_read(saveFile, readBufFloat, 2 * amount, MPI_FLOAT, MPI_STATUS_IGNORE);
		
		if(verbose){
			for(i = 0; i < 2 * amount; i++){
				printf("Read %f - from process %d\n", readBufFloat[i], rank);
			}
		}

		for (i = 0; i < 2 * amount; i+=2) {
	    	mySUMx = mySUMx + readBufFloat[i];
	    	mySUMy = mySUMy + readBufFloat[i + 1];
	    	mySUMxy = mySUMxy + readBufFloat[i] * readBufFloat[i + 1];
	    	mySUMxx = mySUMxx + readBufFloat[i] * readBufFloat[i];
	  	}
	  	if(verbose){
	  		printf("mySUMx = %f; mySUMy = %f; mySUMxy = %f; mySUMxy = %f - from process %d\n", mySUMx, mySUMy, mySUMxy, mySUMxx, rank);
	  	}
	}

	MPI_Reduce(&mySUMx, &totalSUMx,  1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mySUMy, &totalSUMy,  1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mySUMxy, &totalSUMxy,  1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&mySUMxx, &totalSUMxx,  1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

	if(rank == 0){

		if(verbose){
			printf("totalSUMx = %f, totalSUMy = %f, totalSUMxy = %f, totalSUMxx = %f - from process %d\n", totalSUMx, totalSUMy, totalSUMxy, totalSUMxx, rank);
		}

		slope = ( totalSUMx * totalSUMy - params[0] * totalSUMxy ) / ( totalSUMx * totalSUMx - params[0] * totalSUMxx );
	    y_intercept = ( totalSUMy - slope * totalSUMx ) / params[0];

	   	printf("\ny = %f * x + %f\n\n", slope, y_intercept);
	}

	MPI_File_close(&saveFile);

	return 0;
}