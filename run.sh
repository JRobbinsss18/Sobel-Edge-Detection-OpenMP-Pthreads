#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_video.mp4> <output_file_base_name>"
    exit 1
fi

input_video="$1"
output_video="$2"

mkdir -p images output_images_G output_images_GX output_images_GY

ffmpeg -i "$input_video" -vf fps=24 images/frame_%04d.pgm

echo "Frames extracted to 'images/' directory."

for image in images/*.pgm; do
    filename=$(basename -- "$image")
    base="${filename%.*}"
    ./sobel "$image" "output_images_G/${base}.G.pgm" "output_images_GX/${base}.GX.pgm" "output_images_GY/${base}.GY.pgm"
done

echo "Sobel transformation applied to all frames."

ffmpeg -framerate 20 -i output_images_G/frame_%04d.G.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_G.mp4"
ffmpeg -framerate 20 -i output_images_GX/frame_%04d.GX.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_GX.mp4"
ffmpeg -framerate 20 -i output_images_GY/frame_%04d.GY.pgm -c:v libx264 -pix_fmt yuv420p "${output_video}_GY.mp4"

echo "Videos created: ${output_video}_G.mp4, ${output_video}_GX.mp4, ${output_video}_GY.mp4"

rm -r images output_images_G output_images_GX output_images_GY