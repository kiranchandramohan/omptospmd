#define IMG_SIZE 4096  // size of image matrix
#define HISTO_SIZE 256

void serial_histo(int* image, int* histogram, int* gray_level_mapping)
{
	int i, j ;
	float cdf, pixels ;

	int nIter = 0 ;

	/* Compute the image's histogram */
	#pragma omp parallel for
	for (i = 0; i < IMG_SIZE ; i++) {
		#pragma omp critical
		pixels += 1;
	}   
}
