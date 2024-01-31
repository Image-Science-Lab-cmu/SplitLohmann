close all
clear all

addpath('functions');

%%% ====================// Select simulation mode //=======================
% mode = 1: coarse resolution, few iterations, quick and dirty (3 min)
% mode = 2: fine resolution, more iterations, avoids aliasing (43 min)
% runtime is approximate (tested on Apple M2 Max 96GB RAM)
mode = 1;

%%% =========================// Select scene //============================
% Scenes: Whiskey, Motorcycle, CastleCity
% Scene files are stored in the scenes/ folder
scene_name = "Whiskey";

%%% ======================// Select color flag //==========================
% color = true: simulates RGB channel content (using the same wavelength)
% color = false: computes the green channel content only
flags.color = true;

%%% ====================// Set quantization flag //========================
% The simulation should always be ran with this flag being true, 
% otherwise only to replicate the Figure 6c in the paper
flags.quantization = true; 

%%% ==================// Define hardware parameters //=====================
[hardware, eye, sim, io] = define_params(mode, scene_name, flags);

%%% ==========================// Load scene //=============================
[io, sim] = load_scene(io, sim, flags);

%%% ======================// Print specificaions //========================
print_specs(hardware, eye, sim)

%%% ========================// Begin simulation //=========================
tic
result_img_stack = simulation(io, hardware, eye, sim, flags);
toc

%%% =========================// Save results //============================
compile_gif(result_img_stack, io, flags)
disp('Simulation done.')
