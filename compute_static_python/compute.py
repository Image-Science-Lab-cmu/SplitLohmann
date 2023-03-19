import glob
import os
import numpy as np
import skimage.io
import cv2
import copy
from params import Params

def digitize(diopter_map, numDepths):
    '''
    Discretize input diopter_map to numDepths different depths.
    '''
    diopterBins = np.linspace(np.min(diopter_map),np.max(diopter_map),numDepths)
    dig = np.digitize(diopter_map, diopterBins) - 1
    return diopterBins[dig]

def load_images(load_path, discretize=False, numdepths=None,
                texture_map_name=None, diopter_map_name=None):
    '''
    Load texture and diopter maps from separate pngs.
    Assumes that texture map is color and the diopter map is grayscale.
    '''
    texture_map = skimage.io.imread(
                os.path.join(load_path,texture_map_name)).astype(np.float64)/255
    diopter_map = skimage.io.imread(
                os.path.join(load_path,diopter_map_name)).astype(np.float64)/255
    texture_map = texture_map[:,:,:3]

    if discretize:
        diopter_map = digitize(diopter_map, numdepths)

    return texture_map, diopter_map

def crop_grayscale_image(image, slm_shape):
    '''
    Assumes image is grayscale and has shape (h,w).
    If image is bigger than slm_shape, crop image to fit slm_shape.
    If image is smaller than slm_shape, pad images with zeros to fit slm_shape.
    '''
    ## crop image to fit slm_shape
    ystart, xstart = 0, 0
    if image.shape[0]>slm_shape[0]:
        ystart = (image.shape[0]-slm_shape[0])//2
    if image.shape[1]>slm_shape[1]:
        xstart = (image.shape[1]-slm_shape[1])//2
    image = image[ystart:ystart+slm_shape[0],xstart:xstart+slm_shape[1]]

    ## pad along y
    if image.shape[0]<slm_shape[0]:
        numpix2fill = slm_shape[0]-image.shape[0]
        if numpix2fill%2==1:
            image = np.pad(image,((numpix2fill//2,numpix2fill//2+1),(0,0)))
        else:
            image = np.pad(image,((numpix2fill//2,numpix2fill//2),(0,0)))

    ## pad along x
    if image.shape[1]<slm_shape[1]:
        numpix2fill = slm_shape[1]-image.shape[1]
        if numpix2fill%2==1:
            image = np.pad(image,((0,0),(numpix2fill//2,numpix2fill//2+1)))
        else:
            image = np.pad(image,((0,0),(numpix2fill//2,numpix2fill//2)))

    return image

def crop_color_image(image, slm_shape):
    '''
    Assumes image is a color image and has shape (h,w,3).
    If image is bigger than slm_shape, crop image to fit slm_shape.
    If image is smaller than slm_shape, pad images with zeros to fit slm_shape.
    '''
    ## crop image to fit slm_shape
    ystart, xstart = 0, 0
    if image.shape[0]>slm_shape[0]:
        ystart = (image.shape[0]-slm_shape[0])//2
    if image.shape[1]>slm_shape[1]:
        xstart = (image.shape[1]-slm_shape[1])//2
    image = image[ystart:ystart+slm_shape[0],xstart:xstart+slm_shape[1],:]

    ## pad along y
    if image.shape[0]<slm_shape[0]:
        numpix2fill = slm_shape[0]-image.shape[0]
        if numpix2fill%2==1:
            image = np.pad(image,((numpix2fill//2,numpix2fill//2+1),(0,0),(0,0)))
        else:
            image = np.pad(image,((numpix2fill//2,numpix2fill//2),(0,0),(0,0)))

    ## pad along x
    if image.shape[1]<slm_shape[1]:
        numpix2fill = slm_shape[1]-image.shape[1]
        if numpix2fill%2==1:
            image = np.pad(image,((0,0),(numpix2fill//2,numpix2fill//2+1),(0,0)))
        else:
            image = np.pad(image,((0,0),(numpix2fill//2,numpix2fill//2),(0,0)))

    return image

def fit_images(H, texture_map, diopter_map, oled_shape, slm_shape):
    '''
    Crop and warp texture and depth maps.
    For depth map, crop or pad it (no resizing) to fit the slm shape.
    For texture map, crop or pad it (no resizing) to fit the slm shape then
    backward warp it to fit on the oled.
    '''
    image_slm = np.flip(np.flip(diopter_map,0),1)
    image_slm = crop_grayscale_image(image_slm, slm_shape)

    image_oled = np.flip(np.flip(texture_map,0),1)
    image_oled = crop_color_image(image_oled, slm_shape)
    image_oled = cv2.warpPerspective(image_oled, np.linalg.inv(H), oled_shape)

    return image_oled, image_slm

def compute_phase_mask(diopterMap, params, name, modNum=1):
    '''
    Computes the phase mask from the normalized diopter map with the desired
    working range.
    '''
    # fit diopter range
    diopterMap = diopterMap * params.W

    # define coordinates
    xidx = np.arange(-params.slmWidth/2,params.slmWidth/2)
    yidx = np.arange(-params.slmHeight/2,params.slmHeight/2)
    X, Y = np.meshgrid(xidx,yidx)

    # compute phase mask
    factorY = (params.nominal_a/np.sqrt(1+params.nominal_a**2))
    scaleY = ((params.C0*params.SLMpitch*params.fe**2)\
                /(3*params.lbda*params.f0**3))*(params.W/2-diopterMap)
    scaleX = scaleY/params.nominal_a
    DeltaX = -scaleX*((params.lbda*params.f0)/(2*params.SLMpitch))
    DeltaY = -scaleY*((params.lbda*params.f0)/(2*params.SLMpitch))
    N = (params.lbda*params.f0)/params.SLMpitch

    thetaX = DeltaX/N
    thetaY = DeltaY/N
    factor = modNum/(2*np.pi)
    phaseData = ((modNum*(thetaX*X+thetaY*Y))%(modNum))/factor
    phaseData -= np.min(phaseData)
    phaseData /= np.max(phaseData)

    return phaseData

def save2disk(path, device_name, img, name):
    img = (np.round(img*255)).astype(np.uint8)
    os.mkdir(path) if not os.path.isdir(path) else None
    savepath = os.path.join(path,device_name+'_'+name)
    skimage.io.imsave(savepath,img)
    print('Image saved to', savepath)

if __name__=='__main__':

    ### specifiy path to texture and diopter images
    data_folder = 'data'
    output_folder = 'output'
    texture_map_name = 'texture_map.png'
    diopter_map_name = 'diopter_map.png'

    ### define parameters
    discretize = True
    numdepths = 50
    params = Params()
    params.W = 4.0
    oled_shape = (2560,2560)
    slm_shape = (2464,4000)
    print('OLED target image shape:',oled_shape)
    print('SLM target image shape:',slm_shape)
    H = np.load(os.path.join(data_folder,'HomographyMatrixOLED2SLM.npy'))

    ### perform computation
    texture_map, diopter_map = load_images(data_folder, discretize, numdepths,
                                            texture_map_name, diopter_map_name)
    texture_map_out, diopter_map_out = fit_images(H, texture_map, diopter_map,
                                            oled_shape, slm_shape)
    phase_mask = compute_phase_mask(diopter_map_out, params, diopter_map_name)

    ### save output images
    save2disk(output_folder, 'OLED', texture_map_out, texture_map_name)
    save2disk(output_folder, 'SLM', diopter_map_out, diopter_map_name)
    save2disk(output_folder, 'SLM', phase_mask, 'phase_mask_'+diopter_map_name)
