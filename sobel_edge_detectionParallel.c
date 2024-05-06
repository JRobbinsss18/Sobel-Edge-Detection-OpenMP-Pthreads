// Jasper Robbins

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <omp.h>

// Function to get the current time in milliseconds
long long current_time_ms() {
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

// Struct to hold pgm image values
typedef struct {
	char version[3];
	int width;
	int height;
	int maxGrayLevel;
	int **imageData;
	int **gx;
	int **gy;
} pgm;

// Function to create output images gx,gy,output
void init_out_image(pgm* out, pgm image) {
	int i, j;
	strcpy(out->version, image.version);
	out->width = image.width;
	out->height = image.height;
	out->maxGrayLevel = image.maxGrayLevel;

	out->imageData = (int**) malloc(out->height * sizeof(int*));
	out->gx = (int**) malloc(out->height * sizeof(int*));
	out->gy = (int**) malloc(out->height * sizeof(int*));
	for (i = 0; i < out->height; i++) {
		out->imageData[i] = (int*) calloc(out->width, sizeof(int));
		out->gx[i] = (int*) calloc(out->width, sizeof(int));
		out->gy[i] = (int*) calloc(out->width, sizeof(int));
	}

	// Parallelize for loop with j as private to keep outputs correct
#pragma omp parallel for private(j)
	for (i = 0; i < out->height; i++) {
		for (j = 0; j < out->width; j++) {
			out->imageData[i][j] = image.imageData[i][j];
			out->gx[i][j] = image.imageData[i][j];
			out->gy[i][j] = image.imageData[i][j];
		}
	}
}

// Function to read comments at top of pgm file
void read_comments(FILE *input_image) {
	char ch;
	char line[100];

	while ((ch = fgetc(input_image)) != EOF && (isspace(ch))) {
		;
	}
	if (ch == '#') {
		if (fgets(line, sizeof(line), input_image) == NULL) {
			fprintf(stderr, "Error reading line from file.\n");
			return;
		}
	}
	else {
		fseek(input_image, -2L, SEEK_CUR);
	}
}

// Function to read pgm file in required format
void read_pgm_file(char* dir, pgm* image) {
	FILE* input_image;
	int i, j, num;

	input_image = fopen(dir, "rb");
	if (input_image == NULL) {
		printf("File could not be opened!");
		return;
	}

	if (fgets(image->version, sizeof(image->version), input_image) == NULL) {
		fprintf(stderr, "Error reading image version.\n");
		return;
	}
	read_comments(input_image);

	if (fscanf(input_image, "%d %d %d", &image->width, &image->height, &image->maxGrayLevel) != 3) {
		fprintf(stderr, "Error reading image dimensions.\n");
		return;
	}

	image->imageData = (int**) malloc(image->height * sizeof(int*));
	for (i = 0; i < image->height; i++) {
		image->imageData[i] = (int*) calloc(image->width, sizeof(int));
	}

	if (strcmp(image->version, "P2") == 0) {
		for (i = 0; i < image->height; i++) {
			for (j = 0; j < image->width; j++) {
				if (fscanf(input_image, "%d", &num) != 1) {
					fprintf(stderr, "Error in loop");
					break;
				}
				image->imageData[i][j] = num;
			}
		}
	}
	else if (strcmp(image->version, "P5") == 0) {
		char *buffer;
		int buffer_size = image->height * image->width;
		buffer = (char*) malloc((buffer_size + 1) * sizeof(char));

		if (buffer == NULL) {
			printf("Cannot allocate memory for buffer! \n");
			return;
		}
		size_t itemsRead = fread(buffer, sizeof(char), image->width * image->height, input_image);
		if (itemsRead != image->width * image->height) {
			fprintf(stderr, "Error reading image data from file. Expected %d items, got %zu.\n", image->width * image->height, itemsRead);
			return;
		}
		for (i = 0; i < image->height * image->width; i++) {
			image->imageData[i / image->width][i % image->width] = (unsigned char)buffer[i];
		}
		free(buffer);
	}
	fclose(input_image);
	printf("_______________IMAGE INFO__________________\n");
	printf("Version: %s \nWidth: %d \nHeight: %d \nMaximum Gray Level: %d \n", image->version, image->width, image->height, image->maxGrayLevel);
}

// Function to add padding (sets border pixels to 0)
void padding(pgm* image) {
	int i;
	// parallelize for loop
#pragma omp parallel for
	for (i = 0; i < image->width; i++) {
		image->imageData[0][i] = 0;
		image->imageData[image->height - 1][i] = 0;
	}

	// parallelize for loop
#pragma omp parallel for
	for (i = 0; i < image->height; i++) {
		image->imageData[i][0] = 0;
		image->imageData[i][image->width - 1] = 0;
	}
}

// Function to convolve (mult 3x3 kernel with region of image starting at (row,col)
int convolution(pgm* image, int kernel[3][3], int row, int col) {
	int i, j, sum = 0;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			sum += image->imageData[i + row][j + col] * kernel[i][j];
		}
	}
	return sum;
}

// Function to apply sobel edge detection
void sobel_edge_detector(pgm* image, pgm* out_image) {
	int i, j, gx, gy;
	int mx[3][3] = {
		{-1, 0, 1},
		{-2, 0, 2},
		{-1, 0, 1}
	};
	int my[3][3] = {
		{-1, -2, -1},
		{0, 0, 0},
		{1, 2, 1}
	};

	long long start = current_time_ms();
	// Paralellize using for schedule dynamic to split up the work evenly, privatizing j gx and gy as gx and gy are the output gradient images and j is inside nested for loop, each of these need to be protected.
	// Dynamic: each thread will work on a chunk of values and then take the next chunk which hasnt been worked on by any thread, better for load balancing when workload is not the same always
#pragma omp parallel for schedule(dynamic) private(j, gx, gy)
	for (i = 1; i < image->height - 2; i++) {
		for (j = 1; j < image->width - 2; j++) {
			gx = convolution(image, mx, i, j);
			gy = convolution(image, my, i, j);
			out_image->imageData[i][j] = sqrt(gx * gx + gy * gy);
			out_image->gx[i][j] = gx;
			out_image->gy[i][j] = gy;
		}
	}
	long long end = current_time_ms();
	printf("Sobel edge detection time: %lld ms\n", (end - start));
}

// Normalization function, keep matrix values from [0,255] range
void min_max_normalization(pgm* image, int** matrix) {
	int min = 1000000, max = 0, i, j;

	long long start = current_time_ms();
	// Paralellization with reduction, find min and max values, each thread calculates local min/max and only updates the global values when parallel section concludes
#pragma omp parallel for reduction(min: min) reduction(max: max)
	for (i = 0; i < image->height; i++) {
		for (j = 0; j < image->width; j++) {
			if (matrix[i][j] < min) {
				min = matrix[i][j];
			}
			else if (matrix[i][j] > max) {
				max = matrix[i][j];
			}
		}
	}

	// Normalize to range of [0,255]
#pragma omp parallel for private(j)
	for (i = 0; i < image->height; i++) {
		for (j = 0; j < image->width; j++) {
			double ratio = (double)(matrix[i][j] - min) / (max - min);
			matrix[i][j] = ratio * 255;
		}
	}
	long long end = current_time_ms();
	printf("Min-max normalization time: %lld ms\n", (end - start));
}

// Struct for saving
typedef struct {
	pgm* image;
	char* dir;
	int** matrix;
	char* name;
} write_args;

// Function to write pgm file (output)
void* write_pgm_file(void* args) {
	write_args* wargs = (write_args*)args;
	pgm* image = wargs->image;
	char* dir = wargs->dir;
	int** matrix = wargs->matrix;
	char* name = wargs->name;

	FILE* out_image;
	int i, j, count = 0;

	char* token = strtok(dir, ".");
	if (token != NULL) {
		strcat(token, name);
		out_image = fopen(token, "wb");
	}

	out_image = fopen(dir, "wb");
	fprintf(out_image, "%s\n", image->version);
	fprintf(out_image, "%d %d\n", image->width, image->height);
	fprintf(out_image, "%d\n", image->maxGrayLevel);

	if (strcmp(image->version, "P2") == 0) {
		for (i = 0; i < image->height; i++) {
			for (j = 0; j < image->width; j++) {
				fprintf(out_image, "%d", matrix[i][j]);
				if (count % 17 == 0)
					fprintf(out_image, "\n");
				else
					fprintf(out_image, " ");
				count++;
			}
		}
	}
	else if (strcmp(image->version, "P5") == 0) {
		for (i = 0; i < image->height; i++) {
			for (j = 0; j < image->width; j++) {
				char num = matrix[i][j];
				fprintf(out_image, "%c", num);
			}
		}
	}
	fclose(out_image);
	return NULL;
}

// Driver
int main(int argc, char **argv) {

	if (argc != 5) {
		printf("Usage: %s <input_file> <output_dir_G> <output_dir_GX> <output_dir_GY>\n", argv[0]);
		return 1;
	}

	
	char* input_file = argv[1];
	char* output_dir_G = argv[2];
	char* output_dir_GX = argv[3];
	char* output_dir_GY = argv[4];

	pgm image, out_image;

	read_pgm_file(input_file, &image);
	padding(&image);
	init_out_image(&out_image, image);
	sobel_edge_detector(&image, &out_image);

	min_max_normalization(&out_image, out_image.imageData);
	min_max_normalization(&out_image, out_image.gx);
	min_max_normalization(&out_image, out_image.gy);

	pthread_t threads[3];
	write_args args_g = {&out_image, output_dir_G, out_image.imageData, ".G.pgm"};
	write_args args_gx = {&out_image, output_dir_GX, out_image.gx, ".GX.pgm"};
	write_args args_gy = {&out_image, output_dir_GY, out_image.gy, ".GY.pgm"};

	pthread_create(&threads[0], NULL, write_pgm_file, (void*)&args_g);
	pthread_create(&threads[1], NULL, write_pgm_file, (void*)&args_gx);
	pthread_create(&threads[2], NULL, write_pgm_file, (void*)&args_gy);

	for (int i = 0; i < 3; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("\nGradient saved: %s \n", output_dir_G);
	printf("Gradient X saved: %s \n", output_dir_GX);
	printf("Gradient Y saved: %s \n", output_dir_GY);

	free(image.imageData);
	free(out_image.imageData);
	free(out_image.gx);
	free(out_image.gy);

	return 0;
}

