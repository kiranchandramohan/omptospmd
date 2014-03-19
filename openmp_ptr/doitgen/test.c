#include "test.h"
#define NQ 64
#define NR 64
#define NP 64

void doitgen(int* __restrict__ A, int* __restrict__ sum, int* __restrict__ C4)
{
	int r, q, p, s ; 

	#pragma omp parallel for
	for (r = 0 ; r < NR ; r++) {
		for (q = 0 ; q < NQ ; q++)
			for (p = 0; p < NP; p++)
				A[r*NQ*NP+q*NP+p] = ((int) r*q + p) / NP ;
	}

	#pragma omp parallel for
	for (r = 0; r < NP; r++) {
		for (q = 0; q < NP; q++)
			C4[r*NP+q] = ((int) r*q) / NP ;
	}

	#pragma omp parallel for
	for (r = 0; r < NR; r++) {
		for (q = 0; q < NQ; q++) {
			for (p = 0; p < NP; p++) {
				sum[r*NQ*NP+q*NP+p] = 0;
				for (s = 0; s < NP; s++)
					sum[r*NQ*NP+q*NP+p] = sum[r*NQ*NP+q*NP+p] + A[r*NQ*NP+q*NP+s] * C4[s*NP+p];
			}   
			for (p = 0; p < NR; p++)
				A[r*NQ*NP+q*NP+p] = sum[r*NQ*NP+q*NP+p];
		}   
	}
}

int main()
{
	int A[NR*NQ*NP] ;
	int sum[NR*NQ*NP] ;
	int C4[NP*NP] ;

	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		doitgen(A, sum, C4) ;
	}

	return 0 ;
}

void cleanup()
{
}
