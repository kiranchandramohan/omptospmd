#include "barrier.h"
#include "test.h"
#define MAXGRID 64
#define LENGTH 2048

void reg_detect(int* sum_tang, int* mean, int* diff, int* sum_diff, int* path, int* tmp)
{
        int t, i, j, cnt;

	#pragma omp parallel for
	for (i = 0; i < MAXGRID; i++) {
		for (j = 0; j < MAXGRID; j++) {
			sum_tang[i*MAXGRID+j] = (i+1)*(j+1);
			mean[i*MAXGRID+j] = (i-j)/MAXGRID;
			path[i*MAXGRID+j] = (i*(j-1))/MAXGRID;
		}   
	}

	#pragma omp parallel for
	for (j = 0; j <= MAXGRID - 1; j++) {
		for (i = j; i <= MAXGRID - 1; i++)
			for (cnt = 0; cnt <= LENGTH - 1; cnt++) {
				diff[j*MAXGRID*LENGTH + i*LENGTH + cnt] =
					sum_tang[j*MAXGRID + i];
			}
	}

	#pragma omp parallel for
	for (j = 0; j <= MAXGRID - 1; j++)
	{
		for (i = j; i <= MAXGRID - 1; i++)
		{   
			sum_diff[j*MAXGRID*LENGTH + i*LENGTH + 0] = diff[j*MAXGRID*LENGTH + i*LENGTH + 0] ;
			for (cnt = 1; cnt <= LENGTH - 1; cnt++) {
				sum_diff[j*MAXGRID*LENGTH + i*LENGTH + cnt] = 
					sum_diff[j*MAXGRID*LENGTH + i*LENGTH + cnt-1] + 
					diff[j*MAXGRID*LENGTH + i*LENGTH + cnt];
			}   

			mean[j*MAXGRID+i] = sum_diff[j*MAXGRID*LENGTH + i*LENGTH + LENGTH-1];
		}   
	}   

	#pragma omp parallel for
        for (j = 0 ; j < MAXGRID ; j++) {
                int x = 0 ;
                tmp[j*MAXGRID+j] = mean[x*MAXGRID+j] ;
                //printf("tmp[%d][%d] = %d\n", j, j, tmp[j*MAXGRID+j]) ;
                for (i = j+1; i <= MAXGRID - 1; i++) {
                        x++ ;
                        tmp[j*MAXGRID+i] = tmp[j*MAXGRID+i-1] + mean[x*MAXGRID+i];
                        //printf("tmp[%d][%d] = %d\n", j, i, tmp[j*MAXGRID+i]) ;
                }
        }

	#pragma omp parallel for
        for (j = 0 ; j < MAXGRID ; j++) {
                int x = 0 ;
                for (i = j ; i <= MAXGRID - 1; i++) {
                        path[j*MAXGRID+i] = tmp[x*MAXGRID+i] ;
                        x++ ;
                }
        }
}
