#include "barrier.h"
#include "test.h"
#define N 512

inline int mod(int n)
{
	if(n<0)
		return -n ;
	else
		return n ;
}

void floyd_warshall(int* path)
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
