#define IMG_SIZE 4096  // size of image matrix
#define HISTO_SIZE 256

void serial_histo(int* image, int* histogram, int* gray_level_mapping)
{
	int i, j ;
	float cdf, pixels ;

	int nIter = 0 ;

	/* Initialize the histogram array. */
	#pragma omp parallel for
	for (i = 0 ; i < IMG_SIZE ; i++) {
		for (j = 0; j < IMG_SIZE; ++j) {
			image[i*IMG_SIZE+j] = (i*j) % 255 ;
		}   
	}   

	for (i = 0 ; i < HISTO_SIZE ; i++)
		histogram[i] = 0;

	/* Compute the image's histogram */
	#pragma omp parallel for
	for (i = 0; i < IMG_SIZE ; i++) {
		for (j = 0; j < IMG_SIZE; ++j) {
			int pix = image[i*IMG_SIZE+j] ;
			#pragma omp critical
			histogram[pix] += 1;
		}   
	}   

	/* Compute the mapping from the old to the new gray levels */

	cdf = 0.0;
	pixels = (float) (IMG_SIZE * IMG_SIZE);
	for (i = 0; i < HISTO_SIZE; i++) {
		cdf += ((float)(histogram[i])) / pixels;
		gray_level_mapping[i] = (int)(255.0 * cdf);
	}

	/* Map the old gray levels in the original image to the new gray levels. */
	#pragma omp parallel for
	for (i = 0 ; i < IMG_SIZE; i++) {
		for (j = 0; j < IMG_SIZE; ++j) {
			image[i*IMG_SIZE+j] = gray_level_mapping[image[i*IMG_SIZE+j]];
		}
	}
}
