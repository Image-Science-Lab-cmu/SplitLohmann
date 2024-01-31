function [io, sim] = load_scene(io, sim, flags)

    %%% Set input and output paths
    scenePath = fullfile(io.folder_name, strcat(io.scene_name, ".mat"));
    savefolder = fullfile('results', io.scene_name);
    if ~exist(savefolder, 'dir')
        mkdir(savefolder)
    end
    io.save_path = fullfile(savefolder, strcat(io.scene_name, '-simRes_', ...
                    num2str(round(sim.resolution*1e6,2)),'um-numIter_', ...
                    num2str(sim.num_iter),'-quantize_', ...
                    num2str(flags.quantization),'-hasColor_', ...
                    num2str(flags.color)));
    fprintf(strcat('Results will be saved to:\n', io.save_path, '\n'))
    
    %%% Read scene
    scene = load(scenePath);
    texture_map = scene.textureMap;
    diopter_map = scene.diopterMap;

    %%% Define eye focus settings (which distance to focus at)
    sim.eye_focus_diopters = scene.example_focus_diopters;
    sim.num_focus_images = numel(sim.eye_focus_diopters);

    %%% Resize scene files
    height_old = size(texture_map,1);
    width_old = size(texture_map,2);
    asp_ratio = sim.scene_file_resolution(1)/sim.scene_file_resolution(2);
    if height_old/width_old < asp_ratio
        height_new = sim.scene_file_resolution(1);
        width_new = uint16(round(width_old*height_new/height_old));
    else
        width_new = sim.scene_file_resolution(2);
        height_new = uint16(round(height_old*width_new/width_old));
    end
    io.texture_map = imresize(texture_map, [height_new, width_new]);
    io.diopter_map = imresize(diopter_map, [height_new, width_new]);
    sim.cam_resolution = [size(io.texture_map,1) size(io.texture_map,2)];

    disp('----------------------// Scene created //----------------------')
    
end