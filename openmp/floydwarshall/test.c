#define N 512

inline int mod(int n)
{
	if(n<0)
		return -n ;
	else
		return n ;
}

void serial_floyd_warshall(int path[N][N])
{
	int i, j, k;

	#pragma omp parallel for
	for(i=0 ; i<N ; i++) {
		for(j=0 ; j<N ; j++) {
			path[i][j] = mod(i-j) ;
		}
	}

	for (k = 0; k < N; k++) {
		#pragma omp parallel for
		for(i = 0; i < N; i++) {
			for (j = 0; j < N; j++) {
				path[i][j] = path[i][j] < path[i][k] + path[k][j] ? 
					path[i][j] : path[i][k] + path[k][j] ;
			}
		}
	}
}
