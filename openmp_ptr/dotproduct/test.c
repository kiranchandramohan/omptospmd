#include "test.h"
#define SIZE 2048

int dot_product(int* __restrict__ A, int* __restrict__ B)
{
	int i ;
	int sum = 0 ;
	
        #pragma omp parallel for
	for (i = 0 ; i < SIZE*SIZE ; i++)
	{
		A[i] = B[i] = i ;
	}

	//Changes required for reduction
        #pragma omp parallel for reduction(+:sum)
	for (i = 0 ; i < SIZE*SIZE ; i++)
	{
		sum += A[i] * B[i] ;
	}

	return sum ;
}

int main()
{
	int A[SIZE*SIZE] ;
	int B[SIZE*SIZE] ;
	int dotp ;
	int i ;
	for(i=0 ; i<NUM_ITER ; i++)
	{
		dotp = dot_product(A, B) ;
	}

	return 0 ;
}

void cleanup()
{
}
