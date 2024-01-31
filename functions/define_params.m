function [hardware, eye, sim, io] = define_params(mode, scene_name, flags)

    %%% ================// Define hardware specifications //===============
    hardware.f0 = 100e-03; % focal length of the relay lens
    hardware.lambda = 530e-09; % simulation wavelength
    hardware.working_range = 4.0; % working range of display
    hardware.oled_pixel_pitch = 4.0e-06;
    hardware.slm_pixel_pitch = 4.0e-06;
    hardware.cam_pixel_pitch = 4.0e-06;

    %%% ===============// Define cubic phase plate parameter //============
    % C0 is the curvature parameter of the Cubic Phase Plate (CPP)
    % as explained in Section 2.2 of the paper.
    % We used this calue to fabricate our CPP.
    hardware.C0 = 1.9337e-02; 

    %%% ================// Define eye specifications //====================
    eye.eyepiece_f = 40e-03; % focal length of the eyepiece
    eye.eye_diameter = 5e-03; % diameter of the ocular lens
    eye.eye_pupil_diameter = 5e-03; % diameter of the ocular pupil
    eye.eye_retina_distance = 25e-03; % distance from ocular lens to retina
    
    %%% ===============// Define simulation parameters //==================
    if mode==1
        sim.num_iter = 20;
        sim.resolution = 5.0e-06;
    elseif mode==2
        sim.num_iter = 100;
        sim.resolution = 4.0e-06;
    else
        fprintf("Please select a valid simulation mode (see README).\n");
    end
    sim.scene_file_resolution = [1080 1920]; % resolution to resize scenes
    sim.num_depths = 50; % number of depth planes for quantization
    sim.N = round(hardware.lambda*hardware.f0/sim.resolution^2);
    sim.resolution = sqrt(hardware.lambda*hardware.f0/sim.N);
    if flags.color
        sim.color_name_list = ["Red" "Green" "Blue"];
    else
        sim.color_name_list = "Green";
    end

    %%% ================// Define scene I/O parameters //==================
    io.folder_name = 'scenes/mat';
    io.scene_name = scene_name;
    io.gamma_correction_constant = 2.7;
    io.gamma_correction_gamma_inv = 2.2;
    io.gif_frame_duration = 0.4;

end