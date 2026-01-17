# High-Performance C++ FFT Audio Visualizer

A real-time, multi-threaded audio visualization engine written in C++ using Raylib. 

## Key Features

* Performs Fast Fourier Transform (FFT) on live audio data with sub-millisecond latency.

* Runs audio processing and graphical rendering on separate threads to prevent UI blocking and ensure
a locked 60 FPS.

* Utilizes a Lock-Free Single-Producer-Single-Consumer (SPSC) Ring Buffer to ensure uninterrupted audio streaming.

* Uses Raylib for the graphical rendering of frequency bars.

## Video Examples

* **[Demo 1](https://youtu.be/mTJd4j_Y69w)**

* **[Demo 2](https://youtu.be/GUeI9zm9KNQ)**

* **[Demo 3](https://youtu.be/hQZe7EF5zdE)**