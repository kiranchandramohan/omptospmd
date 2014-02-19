#define NUM_ITER 10
#define CNT  500
#define SIZE 2048
#define         K       3
#define         N       SIZE
#define         T       127

#define GAUSSIAN 1
#define VERTICAL_SOBEL 2
#define HORIZONTAL_SOBEL 3

int abs(int n)
{
	return (n<0) ? (-n) : n ;
}

int get_start_indx(int n)
{
        if(n==0)
                return n ; 
        else
                return n-1 ;
}

int get_end_indx(int n)
{
        if(n==SIZE)
                return n ; 
        else
                return n+1 ;
}

void initialize(int* __restrict__ image_buffer1, int* __restrict__ image_buffer2, int* __restrict__ image_buffer3)
{
  int i, j ;

  //printf("Beginning initialization\n") ;

  /* Read input image. */
  //input_dsp(image_buffer1, N*N, 1);

  /* Initialize image_buffer2 and image_buffer3 */
  #pragma omp parallel for
  for (i = start_indx; i < N ; i++) {
    for (j = 0; j < N; ++j) {
       image_buffer1[i*N+j] = i+j ;

       image_buffer2[i*N+j] = 0;

       image_buffer3[i*N+j] = 0;
     }
  }
  //printf("Finishing initialization\n") ;
}

/* This function convolves the input image by the kernel and stores the result
   in the output image. */
void convolve2d(int* __restrict__ input_image, int* __restrict__ kernel, int* __restrict__ output_image,
		int start_indx, int end_indx)
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
      normal_factor += abs(kernel[r*K+c]);
    }
  }

  if (normal_factor == 0)
    normal_factor = 1;

  /* Convolve the input image with the kernel. */
  #pragma omp parallel for
  for (r = start_indx; r < end_indx - K + 1; r++) {
    for (c = 0; c < N - K + 1; c++) {
      sum = 0;
      for (i = 0; i < K; i++) {
        for (j = 0; j < K; j++) {
          sum += input_image[(r+i)*N+c+j] * kernel[i*K+j];
        }
      }
      output_image[(r+dead_rows)*N+c+dead_cols] = (sum / normal_factor);
    }
  }
}

void set_filter(int* __restrict__ filter, int type)
{
	if(type == GAUSSIAN) {
		filter[0*K+0] = 1;
		filter[0*K+1] = 2;
		filter[0*K+2] = 1;
		filter[1*K+0] = 2;
		filter[1*K+1] = 4;
		filter[1*K+2] = 2;
		filter[2*K+0] = 1;
		filter[2*K+1] = 2;
		filter[2*K+2] = 1;
	} else if(type == VERTICAL_SOBEL) {
		filter[0*K+0] =  1;
		filter[0*K+1] =  0;
		filter[0*K+2] = -1;
		filter[1*K+0] =  2;
		filter[1*K+1] =  0;
		filter[1*K+2] = -2;
		filter[2*K+0] =  1;
		filter[2*K+1] =  0;
		filter[2*K+2] = -1;
	} else if(type == HORIZONTAL_SOBEL) {
		filter[0*K+0] =  1;
		filter[0*K+1] =  2;
		filter[0*K+2] =  1;
		filter[1*K+0] =  0;
		filter[1*K+1] =  0;
		filter[1*K+2] =  0;
		filter[2*K+0] = -1;
		filter[2*K+1] = -2;
		filter[2*K+2] = -1;
	}
}


void apply_threshold(int* __restrict__ input_image1, int* __restrict__ input_image2, int* __restrict__ output_image, int start_indx, int end_indx)
{
  /* Take the larger of the magnitudes of the horizontal and vertical
     matrices. Form a binary image by comparing to a threshold and
     storing one of two values. */
  int i, j ;
  int temp1, temp2, temp3 ;

  #pragma omp parallel for
  for (i = start_indx; i < end_indx; i++) {
    for (j = 0; j < N; ++j) {
       temp1 = abs(input_image1[i*N+j]);
       temp2 = abs(input_image2[i*N+j]);
       temp3 = (temp1 > temp2) ? temp1 : temp2;
       output_image[i*N+j] = (temp3 > T) ? 255 : 0;
     }
  }
}

//have to introduce barriers in between function calls
void edge_detect(int* __restrict__ image_buffer1, int* __restrict__ image_buffer2, 
	int* __restrict__ image_buffer3, int start_indx, int end_indx, int synch, int tid)
{
  int filter[K*K];

  int modified_start_indx, modified_end_indx ;
  modified_start_indx = get_start_indx(start_indx) ;
  modified_end_indx = get_end_indx(end_indx) ;

  int i ;
  for(i=0 ; i<NUM_ITER ; i++) 
  {
	  initialize(image_buffer1, image_buffer2, image_buffer3, modified_start_indx, modified_end_indx) ;

	  set_filter(filter, GAUSSIAN) ;
	  convolve2d(image_buffer1, filter, image_buffer3, modified_start_indx, modified_end_indx);

	  set_filter(filter, VERTICAL_SOBEL) ;
	  convolve2d(image_buffer3, filter, image_buffer1, modified_start_indx, modified_end_indx);

	  set_filter(filter, HORIZONTAL_SOBEL) ;
	  convolve2d(image_buffer3, filter, image_buffer2, modified_start_indx, modified_end_indx);

	  apply_threshold(image_buffer1, image_buffer2, image_buffer3, start_indx, end_indx) ;
  }
}
