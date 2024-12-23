# Sobel Edge Detection with OpenMP and Pthreads

This project demonstrates the performance enhancement achieved by parallelizing a Sobel edge detection algorithm using OpenMP and Pthreads. The original code, which processed a single image, has been adapted to apply Sobel edge detection to each frame of a video, producing an audio-less video with the edge detection applied.

## Table of Contents

- [Project Overview](#project-overview)
- [Files in the Repository](#files-in-the-repository)
- [Installation and Usage](#installation-and-usage)

## Project Overview

The objective of this project is to showcase the potential speedup gained by parallelizing the Sobel edge detection algorithm using OpenMP and Pthreads. By extending the original single-image processing code to handle videos, the project applies Sobel edge detection to each frame, resulting in an output video that highlights the detected edges.

## Files in the Repository

- `sobel_edge_detection.c`: Original implementation of the Sobel edge detection algorithm for single images.
- `sobel_edge_detectionParallel.c`: Parallelized implementation using OpenMP and Pthreads for processing videos.
- `runSeq.sh`: Shell script to execute the sequential version of the Sobel edge detection.
- `runPar.sh`: Shell script to execute the parallel version of the Sobel edge detection.
- `Makefile`: Build configuration file for compiling the code.
- `TestVideos/`: Directory containing sample videos used for testing.
- `ResultsSeq/`: Directory to store results from the sequential execution.
- `ResultsPar/`: Directory to store results from the parallel execution.

## Installation and Usage

1. **Clone the repository**:

   ```bash
   git clone https://github.com/JRobbinsss18/Sobel-Edge-Detection-OpenMP-Pthreads.git
   cd Sobel-Edge-Detection-OpenMP-Pthreads
