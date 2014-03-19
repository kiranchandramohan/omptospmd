#include "test.h"
#define SIZE 1024   // Size by SIZE matrices
int a_matrix[SIZE*SIZE] ;
int b_matrix[SIZE*SIZE] ;
int c_matrix[SIZE*SIZE] ;

void multiply(int* __restrict__ A, int* __restrict__ B, int* __restrict__ C)
{
	int i,j,k;
	#pragma omp parallel for
	for (i = 0 ; i < SIZE ; i++)
	{   
		for (j = 0; j < SIZE; j++)
		{   
			C[i*SIZE+j] = 0 ;
			for ( k = 0; k < SIZE; k++)
				C[i*SIZE+j] += A[i*SIZE+k] * B[k*SIZE+j] ;
		}   
	}
}

int main()
{
	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		multiply(a_matrix, b_matrix, c_matrix) ;
	}

	return 0 ;
}

void cleanup()
{
}
