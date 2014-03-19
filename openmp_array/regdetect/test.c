#include "test.h"
#define MAXGRID 64
#define LENGTH 2048
	int sum_tang[MAXGRID*MAXGRID] ;
	int mean[MAXGRID*MAXGRID] ; 
	int diff[MAXGRID*MAXGRID*LENGTH] ; 
	int sum_diff[MAXGRID*MAXGRID*LENGTH] ;
	int path[MAXGRID*MAXGRID] ; 
	int tmp[MAXGRID*MAXGRID] ;

void reg_detect(int sum_tang[__restrict__ MAXGRID][MAXGRID], int mean[__restrict__ MAXGRID][MAXGRID], int diff[__restrict__ MAXGRID][MAXGRID][LENGTH], 
	int sum_diff[__restrict__ MAXGRID][MAXGRID][LENGTH], int path[__restrict__ MAXGRID][MAXGRID], int tmp[__restrict__ MAXGRID][MAXGRID])
{
        int t, i, j, cnt;

	#pragma omp parallel for
	for (i = 0; i < MAXGRID; i++) {
		for (j = 0; j < MAXGRID; j++) {
			sum_tang[i][j] = (i+1)*(j+1);
			mean[i][j] = (i-j)/MAXGRID;
			path[i][j] = (i*(j-1))/MAXGRID;
		}   
	}

	#pragma omp parallel for
	for (j = 0; j <= MAXGRID - 1; j++) {
		for (i = j; i <= MAXGRID - 1; i++)
			for (cnt = 0; cnt <= LENGTH - 1; cnt++) {
				diff[j][i][cnt] = sum_tang[j][i];
			}
	}

	#pragma omp parallel for
	for (j = 0; j <= MAXGRID - 1; j++)
	{
		for (i = j; i <= MAXGRID - 1; i++)
		{   
			sum_diff[j][i][0] = diff[j][i][0] ;
			for (cnt = 1; cnt <= LENGTH - 1; cnt++) {
				sum_diff[j][i][cnt] = sum_diff[j][i][cnt-1] + diff[j][i][cnt] ;
			}   

			mean[j][i] = sum_diff[j][i][LENGTH-1];
		}   
	}   

	#pragma omp parallel for
        for (j = 0 ; j < MAXGRID ; j++) {
                int x = 0 ;
                tmp[j][j] = mean[x][j] ;
                //printf("tmp[%d][%d] = %d\n", j, j, tmp[j*MAXGRID+j]) ;
                for (i = j+1; i <= MAXGRID - 1; i++) {
                        x++ ;
                        tmp[j][i] = tmp[j][i-1] + mean[x][i];
                        //printf("tmp[%d][%d] = %d\n", j, i, tmp[j*MAXGRID+i]) ;
                }
        }

	#pragma omp parallel for
        for (j = 0 ; j < MAXGRID ; j++) {
                int x = 0 ;
                for (i = j ; i <= MAXGRID - 1; i++) {
                        path[j][i] = tmp[x][i] ;
                        x++ ;
                }
        }
}

int main()
{
	int sum_tang[MAXGRID][MAXGRID] ;
	int mean[MAXGRID][MAXGRID] ; 
	int diff[MAXGRID][MAXGRID][LENGTH] ; 
	int sum_diff[MAXGRID][MAXGRID][LENGTH] ;
	int path[MAXGRID][MAXGRID] ; 
	int tmp[MAXGRID][MAXGRID] ;
	
	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		reg_detect(sum_tang, mean, diff, sum_diff, path, tmp) ;
	}

	return 0 ;
}

void cleanup()
{
}
