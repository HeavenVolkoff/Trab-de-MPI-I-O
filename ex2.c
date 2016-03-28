#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define NUMBERS_PER_PROCESS 10

int main(int argc, char* argv[]){
    int size, rank, *buf, counter, base;
    size_t extension;
    MPI_File fh;
    MPI_Status status;
    MPI_Datatype contiguous, fileType;
    MPI_Init(&argc, &argv);

    buf = (int*) malloc(NUMBERS_PER_PROCESS * sizeof(int));

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    base = 0;
    extension = (size - 1) * 2 * sizeof(int);
    MPI_Type_contiguous(2, MPI_INT, &contiguous);
    MPI_Type_create_resized(contiguous, base, extension, &fileType);
    MPI_Type_commit(&fileType);

    //=========== WRITE ===========
    if (MPI_File_open(MPI_COMM_WORLD, "file.data", MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL, &fh)) {
        //Failed opening file
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    for(counter = 0; counter < 10; counter++){
        buf[counter] = counter + (rank * NUMBERS_PER_PROCESS);
    }

    if(MPI_File_set_view(fh, (MPI_Offset) (rank * NUMBERS_PER_PROCESS * sizeof(int)), MPI_INT, MPI_INT, "native", MPI_INFO_NULL)){
        //Failed Setting file view file
        MPI_Abort(MPI_COMM_WORLD, -2);
    }

    if(MPI_File_write(fh, buf, NUMBERS_PER_PROCESS, MPI_INT, &status)){
        //Failed writing to file
        MPI_Abort(MPI_COMM_WORLD, -3);
    }

    MPI_File_close(&fh);

    //=========== READ ===========
    if (MPI_File_open(MPI_COMM_WORLD, "file.data", MPI_MODE_RDWR, MPI_INFO_NULL, &fh)){
        //Failed opening file
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if(MPI_File_set_view(fh, (MPI_Offset) (rank * 2 * sizeof(int)), MPI_INT, fileType, "native", MPI_INFO_NULL)){
        //Failed Setting file view file
        MPI_Abort(MPI_COMM_WORLD, -2);
    }

    if(MPI_File_read_ordered(fh, buf, NUMBERS_PER_PROCESS, MPI_INT, &status)){
        //Failed reading file
        MPI_Abort(MPI_COMM_WORLD, -4);
    }

    //=========== WRITE ===========
    if(MPI_File_write(fh, buf, NUMBERS_PER_PROCESS, MPI_INT, &status)){
        //Failed writing to file
        MPI_Abort(MPI_COMM_WORLD, -3);
    }

    free(buf);
    MPI_Type_free(&contiguous);
    MPI_Type_free(&fileType);
    MPI_File_close(&fh);
    MPI_Finalize();

    return 0;
}