# Computing Images to Display on OLED and SLM

### Requirements
python 3.6 and above, `numpy`, `skimage`, `opencv`

### Usage
Run in this folder
```sh
python compute.py
```

`compute.py` is a script that takes an input texture map (color, png or jpg) and an input diopter map (grayscale, png or jpg) and computes the texture image to display on the oled and the phase mask to display on the slm. It saves the output texture map, diopter map, and phase mask to `output_folder`.

To change input images, change the file names at line 153 and 154.

To change parameters, change the desired parameter values in `params.py`.

To change the homography matrix, modify line 165 to the desired homography matrix.
