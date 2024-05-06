#!/bin/bash
export OMP_NUM_THREADS=16
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_video.mp4> <output_file_base_name>"
    exit 1
fi

input_video="$1"
output_video="$2"
c_program_output="c_program_time_output.txt"

current_time_ms() {
    echo $(($(date +%s%N) / 1000000))
}

start_time=$(current_time_ms)

mkdir -p images output_images_G output_images_GX output_images_GY

echo "Extracting frames..."
frame_extraction_start=$(current_time_ms)
ffmpeg -i "$input_video" -vf fps=24 images/frame_%04d.pgm
frame_extraction_end=$(current_time_ms)
echo "Frames extracted to 'images/' directory."
echo "Frame extraction took $((frame_extraction_end - frame_extraction_start)) milliseconds."

# Initialize variables for storing C code-specific timing
total_c_code_time=0

echo "Applying Sobel edge detection..."
sobel_start=$(current_time_ms)
> "$c_program_output"  # Clear the output file
for image in images/*.pgm; do
    filename=$(basename -- "$image")
    base="${filename%.*}"
    # Run the C program and capture the output, including timing information
    c_output=$(./sobel_edge_detectionParallel "$image" "output_images_G/${base}.G.pgm" "output_images_GX/${base}.GX.pgm" "output_images_GY/${base}.GY.pgm")
    echo "$c_output" >> "$c_program_output"

    # Extract the timing values (assuming the format "Sobel edge detection time: X ms" and "Min-max normalization time: X ms")
    sobel_time=$(echo "$c_output" | grep "Sobel edge detection time" | awk '{print $5}')
    normalization_times=$(echo "$c_output" | grep "Min-max normalization time" | awk '{print $5}')

    # Add to the total C code execution time
    total_c_code_time=$((total_c_code_time + sobel_time))
    for norm_time in $normalization_times; do
        total_c_code_time=$((total_c_code_time + norm_time))
    done
done
sobel_end=$(current_time_ms)
echo "Sobel transformation applied to all frames."
echo "Sobel edge detection took $((sobel_end - sobel_start)) milliseconds."

echo "Creating videos..."
video_creation_start=$(current_time_ms)
ffmpeg -framerate 20 -i output_images_G/frame_%04d.G.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_G.mp4"
ffmpeg -framerate 20 -i output_images_GX/frame_%04d.GX.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_GX.mp4"
ffmpeg -framerate 20 -i output_images_GY/frame_%04d.GY.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_GY.mp4"
video_creation_end=$(current_time_ms)
echo "Videos created: ${output_video}_G.mp4, ${output_video}_GX.mp4, ${output_video}_GY.mp4"
end_time=$(current_time_ms)
total_time=$((end_time - start_time))

# Display both C code execution time and total script runtime
echo "__________________SUMMARY__________________"
echo "Total time spent overall: ${total_time} milliseconds"
echo "Total time spent in C code: ${total_c_code_time} milliseconds"
echo "Video creation took $((video_creation_end - video_creation_start)) milliseconds."
# Cleanup intermediate directories
rm -r images output_images_G output_images_GX output_images_GY

