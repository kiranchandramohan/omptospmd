#define SIZE 1024   // Size by SIZE matrices

void multiply(int* __restrict__ A, int* __restrict__ B, int* __restrict__ C)
{
	int i,j,k;
	#pragma omp parallel for
	for (i = 0 ; i < SIZE ; i++)
	{   
		for (j = 0; j < SIZE; j++)
		{   
			C[i*SIZE+j] = 0;
			for ( k = 0; k < SIZE; k++)
				C[i*SIZE+j] += A[i*SIZE+k]*B[k*SIZE+j];
		}   
	}
}
