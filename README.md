# SplitLohmannVR_Code

This folder demonstrates the computation needed to display 3D scenes in Split-Lohmann Display.

The inputs to the pipeline are two images, one RGB image in uint8, and a grayscale depth map image in uint8.

The outputs to the two displays are two images, the warpped RGB image in uint8, and the warpped and computed phase mask image in uint8.

`Python` version is in folder `compute_static_python`. This pipeline demonstrates the computation we do for 1 static scene.

`C++` version is in folder `compute_stream_cpp`. This pipeline demonstrates the computation we do for live videos.
