#include "test.h"
#define N 512

inline int mod(int n)
{
	if(n<0)
		return -n ;
	else
		return n ;
}

void floyd_warshall(int* __restrict__ path)
{
	int i, j, k;

	#pragma omp parallel for
	for(i=0 ; i<N ; i++) {
		for(j=0 ; j<N ; j++) {
			path[i*N+j] = mod(i-j) ;
		}
	}

	for (k = 0; k < N; k++) {
		#pragma omp parallel for
		for(i = 0; i < N; i++) {
			for (j = 0; j < N; j++) {
				path[i*N+j] = path[i*N+j] < path[i*N+k] + path[k*N+j] ? 
					path[i*N+j] : path[i*N+k] + path[k*N+j] ;
			}
		}
	}
}

int main()
{
	int path[N*N] ;
	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		floyd_warshall(path) ;
	}

	return 0 ;
}

void cleanup()
{
}
