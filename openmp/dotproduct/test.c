#define NUM_ITER 100
#define SIZE 2048

int dot_product(int* A, int* B)
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

void compute()
{
	int A[SIZE*SIZE] ;
	int B[SIZE*SIZE] ;
	int dotp ;
	int i ;
	for(i=0 ; i<NUM_ITER ; i++)
	{
		dotp = dot_product(A, B) ;
	}
}
