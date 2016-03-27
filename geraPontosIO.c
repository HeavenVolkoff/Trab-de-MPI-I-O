#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

#define INT 1
#define FLOAT 2
#define ERROR 3

#define ANGULAR_COEFICIENT 2
#define LINEAR_COEFICIENT 5

int main(int argc, char** argv){
	int params[2];
	int rank, nProc, i;
	int amount = 0, selected_type;
	int offset;
	MPI_File saveFile;
	size_t type_size;
	int* bufferInt;
	float* bufferFloat;

	int* readBufInt;
	float* readBufFloat;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);

	//Verifica se os parâmetros foram passados corretamente ao programa.
	if(argc < 3){
		if(rank == 0){
			printf("Format Error:\nPlease run the program in the following format:\n\t%s <type of the pairs> <number of pairs to be generated>\n", argv[0]);
		}
		MPI_Finalize();
		exit(-1);
	}

	//Verifica se o programa deve gerar inteiros ou floats e quantos números devem ser gerados.
	if(rank == 0){
		if((argv[1][0] == 'I' || argv[1][0] == 'i') && (argv[1][1] == 'N' || argv[1][1] == 'n') && (argv[1][2] == 'T' || argv[1][2] == 't')){
			params[0] = INT;
			params[1] = atoi(argv[2]);
			printf("\nThis program will generate %d integers and store them in pontos.txt\n\n", params[1]);
		}
		else if((argv[1][0] == 'F' || argv[1][0] == 'f') && (argv[1][1] == 'L' || argv[1][1] == 'l') && (argv[1][2] == 'O' || argv[1][2] == 'o') && (argv[1][3] == 'A' || argv[1][3] == 'a') && (argv[1][4] == 'T' || argv[1][4] == 't')){
			params[0] = FLOAT;
			params[1] = atoi(argv[2]);
			printf("\nThis program will generate %d floating point numbers and store them in pontos.txt\n\n", params[1]);
		}
		else{
			params[0] = ERROR;
		}
	}
	//Transmite o tipo e a quantidade de números para os outros processos.
	MPI_Bcast(params, 2, MPI_INT, 0, MPI_COMM_WORLD);

	//Termina o programa caso o usuário tenha escolhido um tipo inválido.
	if(params[0] == ERROR){
		if(rank == 0){
			printf("\nThe program only works with INT or FLOAT (case insensitive).\nPlease choose one of these and try again.\n\n");	
		}		
		MPI_Finalize();
		exit(-1);
	}

	//Seed do gerador de números aleatórios.
	srand (time(NULL) + rank);

	//Calcula o tamanho certo para alocar em buffer.
	selected_type = (params[0] == INT)? INT : FLOAT;
	type_size = (params[0] == INT)? sizeof(int) : sizeof(float);
	amount += params[1] / nProc;
	if(params[1] % nProc != 0 && params[1] % nProc > rank){
		amount++;
	}

	printf("Farei %d nums - from process %d\n", amount, rank);
	//buffer = malloc(type_size * amount);
	if(params[0] == INT){
		bufferInt = (int*) malloc(2 * type_size * amount);
	}
	else{
		bufferFloat = (float*) malloc(2 * type_size * amount);	
	}

	//Gera números aleatórios.
	if(params[0] == INT){
		for(i = 0; i < 2 * amount; i+=2){
			bufferInt[i] = rand() % (rand() % 1000 + 1);
			bufferInt[i + 1] = ANGULAR_COEFICIENT * bufferInt[i] + LINEAR_COEFICIENT;
			printf("made %d = 2 * %d + 5 - form process %d\n", bufferInt[i + 1], bufferInt[i], rank);
		}
	}
	else{
		for(i = 0; i < amount; i+=2){
			bufferFloat[i] = (rand() % (rand() % 1000 + 1)) + (rand() % (rand() % 1000 + 1))/1000.0;
			bufferFloat[i + 1] = ANGULAR_COEFICIENT * bufferFloat[i] + LINEAR_COEFICIENT;
			printf("made %f = 2 * %f + 5 - form process %d\n", bufferFloat[i + 1], bufferFloat[i], rank);
		}
	}

	//Abre o arquivo que o programa escreverá os números
	MPI_File_open(MPI_COMM_WORLD, "pontos.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &saveFile);
	
	//Cria uma vista dependendo do tipo de número esoclhido pelo usuário.
	if(params[0] == INT){
		if(rank == 0){
			//MPI_File_set_view(saveFile, 0, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
			MPI_File_seek(saveFile, 0, MPI_SEEK_SET);
			//Escreve quantos números serão escritos no arquivo.
			MPI_File_write(saveFile, &(params[1]), 1, MPI_INT, MPI_STATUS_IGNORE);

			//Escreve o tipo desses números.
			MPI_File_write(saveFile, &selected_type, 1, MPI_INT, MPI_STATUS_IGNORE);

			//Escreve os números do buffer no arquivo.
			MPI_File_write(saveFile, bufferInt, 2 * amount, MPI_INT, MPI_STATUS_IGNORE);
		}
		else{
			//offset = params[1] / nProc + ((params[1] % nProc == 0)? 0: ((rank <= params[1] % nProc)? rank : 0));
			offset = 2 * (params[1] / nProc); //Geramos params[1] pares de números.
			if(params[1] % nProc != 0){ //Ajusta o offset caso a divisão não seja exata.
				offset += 2 * rank;
			}
			offset += 2; //Leva em conta os dois primeiros números (quantidade de pares e o tipo).
			printf("offset = %d - from process %d\n", offset, rank);
			//MPI_File_set_view(saveFile, 2 + offset * type_size, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
			MPI_File_seek(saveFile, offset, MPI_SEEK_SET);
			//Escreve os números do buffer no arquivo.
			MPI_File_write(saveFile, bufferInt, 2 * amount, MPI_INT, MPI_STATUS_IGNORE);
		}
	}
	else{
		if(rank == 0){
			//MPI_File_set_view(saveFile, 0, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
			MPI_File_seek(saveFile, 0, MPI_SEEK_SET);
			//Escreve quantos números serão escritos no arquivo.
			MPI_File_write(saveFile, &(params[1]), 1, MPI_INT, MPI_STATUS_IGNORE);

			//Escreve o tipo desses números.
			MPI_File_write(saveFile, &selected_type, 1, MPI_INT, MPI_STATUS_IGNORE);

			//Escreve os números do buffer no arquivo.
			MPI_File_write(saveFile, bufferFloat, 2 * amount, MPI_FLOAT, MPI_STATUS_IGNORE);
		}
		else{
			//offset = params[1] / nProc + ((params[1] % nProc == 0)? 0: ((rank <= params[1] % nProc)? rank : 0));
			offset = 2 * (params[1] / nProc); //Geramos params[1] pares de números.
			if(params[1] % nProc != 0){ //Ajusta o offset caso a divisão não seja exata.
				offset += 2 * rank;
			}
			offset += 2; //Leva em conta os dois primeiros números (quantidade de pares e o tipo).
			printf("offset = %d - from process %d\n", offset, rank);
			//MPI_File_set_view(saveFile, 2 + offset * type_size, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
			MPI_File_seek(saveFile, offset, MPI_SEEK_SET);
			//Escreve os números do buffer no arquivo.
			MPI_File_write(saveFile, bufferFloat, 2 * amount, MPI_FLOAT, MPI_STATUS_IGNORE);
		}
	}
	
	MPI_File_close(&saveFile);

	//=============================================================================
	MPI_File_open(MPI_COMM_WORLD, "pontos.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &saveFile);
	if(rank == 0){
		printf("\n\n\n");
		if(params[0] == INT){
			readBufInt = (int*) malloc((2 * params[1] + 2) * sizeof(int));
			MPI_File_read(saveFile, readBufInt, 2 * params[1] + 2, MPI_INT, MPI_STATUS_IGNORE);
			for(i = 0; i < 2 * params[1] + 2; i++){
				printf("Read %d - from process %d\n", readBufInt[i], rank);
			}
		}
		else{
			readBufFloat = (float*) malloc((2 * params[1] + 2) * sizeof(float));
			MPI_File_read(saveFile, readBufFloat, 2 * params[1] + 2, MPI_FLOAT, MPI_STATUS_IGNORE);
			for(i = 0; i < 2 * params[1] + 2; i++){
				printf("Read %f - from process %d\n", readBufFloat[i], rank);
			}
		}

	}

	MPI_Finalize();

	// free(bufferFloat);
	// free(bufferInt);

	return 0;
}