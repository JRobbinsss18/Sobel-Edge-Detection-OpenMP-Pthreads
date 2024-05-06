CC=gcc
CFLAGS=-Wall -O3
LIBS=-lm
OMPFLAGS=-fopenmp

all: sobel_edge_detection sobel_edge_detectionParallel

sobel_edge_detection: sobel_edge_detection.c
	$(CC) $(CFLAGS) -o sobel_edge_detection sobel_edge_detection.c $(LIBS)

sobel_edge_detectionParallel: sobel_edge_detectionParallel.c
	$(CC) $(CFLAGS) $(OMPFLAGS) -o sobel_edge_detectionParallel sobel_edge_detectionParallel.c $(LIBS)

clean:
	rm -f sobel_edge_detection sobel_edge_detectionParallel
