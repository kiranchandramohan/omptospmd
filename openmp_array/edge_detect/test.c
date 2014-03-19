#include "barrier.h"
#include "test.h"
#define         K       3
#define         N       128
#define         T       127

#define GAUSSIAN 1
#define VERTICAL_SOBEL 2
#define HORIZONTAL_SOBEL 3
#define NUM_ITER 1

int image_buffer1[N][N] ;
int image_buffer2[N][N] ;
int image_buffer3[N][N] ;
int filter[K][K] ;

int my_abs(int x)
{
	if(x<0)
		return -x ;
	else
		return x ;
}

void set_filter(int filter[__restrict__ K][K], int type)
{
	if(type == GAUSSIAN) {
		filter[0][0] = 1;
		filter[0][1] = 2;
		filter[0][2] = 1;
		filter[1][0] = 2;
		filter[1][1] = 4;
		filter[1][2] = 2;
		filter[2][0] = 1;
		filter[2][1] = 2;
		filter[2][2] = 1;
	} else if(type == VERTICAL_SOBEL) {
		filter[0][0] =  1;
		filter[0][1] =  0;
		filter[0][2] = -1;
		filter[1][0] =  2;
		filter[1][1] =  0;
		filter[1][2] = -2;
		filter[2][0] =  1;
		filter[2][1] =  0;
		filter[2][2] = -1;
	} else if(type == HORIZONTAL_SOBEL) {
		filter[0][0] =  1;
		filter[0][1] =  2;
		filter[0][2] =  1;
		filter[1][0] =  0;
		filter[1][1] =  0;
		filter[1][2] =  0;
		filter[2][0] = -1;
		filter[2][1] = -2;
		filter[2][2] = -1;
	}
}

void initialize(int image_buffer1[__restrict__ N][N], int image_buffer2[__restrict__ N][N], int image_buffer3[__restrict__ N][N])
{
  int i, j ;

  /* Initialize image_buffer2 and image_buffer3 */
  #pragma omp parallel for private(i,j)
  for (i = 0 ; i < N ; i++) {
    for (j = 0; j < N; j++) {
       image_buffer1[i][j] = i+j ;
       image_buffer2[i][j] = 0;
       image_buffer3[i][j] = 0;
     }
  }
  //printf("Finishing initialization\n") ;
}

void apply_threshold(int input_image1[__restrict__ N][N], int input_image2[__restrict__ N][N], int output_image[__restrict__ N][N])
{
  /* Take the larger of the magnitudes of the horizontal and vertical
     matrices. Form a binary image by comparing to a threshold and
     storing one of two values. */
  int i, j ;
  int temp1, temp2, temp3 ;

  #pragma omp parallel for private(i,j,temp1,temp2,temp3)
  for (i = 0 ; i < N ; i++) {
    for (j = 0; j < N; j++) {
       temp1 = my_abs(input_image1[i][j]);
       temp2 = my_abs(input_image2[i][j]);
       temp3 = (temp1 > temp2) ? temp1 : temp2;
       output_image[i][j] = (temp3 > T) ? 255 : 0;
     }
  }
}

/* This function convolves the input image by the kernel and stores the result
   in the output image. */
void convolve2d(int input_image[__restrict__ N][N], int kernel[__restrict__ K][K], int output_image[__restrict__ N][N])
{
  int i;
  int j;
  int c;
  int r;
  int normal_factor;
  int sum;
  int dead_rows;
  int dead_cols;

  /* Set the number of dead rows and columns. These represent the band of rows
     and columns around the edge of the image whose pixels must be formed from
     less than a full kernel-sized compliment of input image pixels. No output
     values for these dead rows and columns since  they would tend to have less
     than full amplitude values and would exhibit a "washed-out" look known as
     convolution edge effects. */

  dead_rows = K / 2;
  dead_cols = K / 2;

  /* Calculate the normalization factor of the kernel matrix. */

  normal_factor = 0;
  for (r = 0; r < K; r++) {
    for (c = 0; c < K; c++) {
      normal_factor += my_abs(kernel[r][c]);
    }
  }

  if (normal_factor == 0)
    normal_factor = 1;

  /* Convolve the input image with the kernel. */
  #pragma omp parallel for private(r,c,i,j)
  for (r = 0; r < N - K + 1; r++) {
    for (c = 0; c < N - K + 1; c++) {
      int sum = 0;
      for (i = 0; i < K; i++) {
        for (j = 0; j < K; j++) {
          sum += input_image[(r+i)][c+j] * kernel[i][j];
        }
      }
      output_image[(r+dead_rows)][c+dead_cols] = (sum / normal_factor);
    }
  }
}

int main()
{
  int temp1;
  int temp2;
  int temp3;
  int j;

  int i ;
  for(i=0 ; i<NUM_ITER ; i++) 
  {
	  initialize(image_buffer1, image_buffer2, image_buffer3) ;

	  set_filter(filter, GAUSSIAN) ;
	  convolve2d(image_buffer1, filter, image_buffer3);

	  set_filter(filter, VERTICAL_SOBEL) ;
	  convolve2d(image_buffer3, filter, image_buffer1);

	  set_filter(filter, HORIZONTAL_SOBEL) ;
	  convolve2d(image_buffer3, filter, image_buffer2);

	  apply_threshold(image_buffer1, image_buffer2, image_buffer3) ;
  }
  return 0 ;
}

void cleanup()
{
}
