# BackgroundBandDetection
Detect background bands (blackening) on videos with C++/OpenCV 

The goal is to detect exact positions in video when background blackening appears and disappears.
The program detects video frame numbers where the background band appears and disappears and its coordinates.

The results are saved to CSV files for each video.

![alt text](frame_descr.png)

To compile and run

> cmake .  
> make
>
> ./BackgroundBand <path to folder with .mp4 files> 
>
