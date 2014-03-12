int A[100] ;
int B[100] ;
int C[100] ;

int main() {
	int i ;
	#pragma omp for
	for(i=0 ; i<100 ; i++) {
		A[i] = B[i] - C[i] ; 
	}
}
