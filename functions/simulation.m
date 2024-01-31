function focal_stack = simulation(io, hardware, eye, sim, flags)

    focal_stack = zeros(sim.cam_resolution(1), ...
                        sim.cam_resolution(2), 3, ...
                        sim.num_focus_images);
    
    for idx=1:numel(sim.color_name_list)

        %%% get color channel image
        color_name = sim.color_name_list(idx);
        if flags.color
            texture_map = io.texture_map(:,:,idx); % RGB channels
        else
            texture_map = io.texture_map(:,:,2); % green only
        end
    
        %%% Define simulatioin parameters
        eye.eye_v = eye.eye_retina_distance;
        eye.eye_focal_lengths = 1./(sim.eye_focus_diopters + 1./eye.eye_v);
        eye.delZ_max = hardware.working_range*eye.eyepiece_f^2;
        eye.delZ_half_max = eye.delZ_max/2;
        hardware.aperture_T2 = eye.eye_pupil_diameter;
        hardware.max_shift_by_slm = hardware.lambda*hardware.f0/( ...
                                    2*hardware.slm_pixel_pitch);
        dbpp_max = eye.delZ_half_max*(hardware.C0/(6*hardware.f0^2));
        hardware.aperture_T1 = hardware.aperture_T2 + 2*sqrt(2)*dbpp_max;
        disp('Simulation parameters defined.')
    
        %%% Define display coordinates
        display.d1y = hardware.oled_pixel_pitch;
        display.N1y = size(texture_map, 1);
        display.w1y = display.N1y*display.d1y;  
        display.c1y = -display.w1y/2-hardware.oled_pixel_pitch/2;
        display.n1y = 1:size(texture_map, 1); 
        display.y1 = display.c1y+display.n1y*display.d1y;
        display.d1x = hardware.oled_pixel_pitch;
        display.N1x = size(texture_map, 2);
        display.w1x = display.N1x*display.d1x;  
        display.c1x = -display.w1x/2-hardware.oled_pixel_pitch/2;
        display.n1x = 1:size(texture_map, 2);
        display.x1 = display.c1x+display.n1x*display.d1x;
        display.apture = 25.4e-3;
        disp('Display coordinates defined.')    
        
        %%% Define simulation coordinates
        W = sim.N*sim.resolution;
        coor.d1y = sim.resolution;
        coor.N1y = sim.N;
        coor.w1y = W;  
        coor.c1y = -coor.w1y/2+coor.d1y/2;
        coor.n1y = 0:(coor.N1y-1);
        coor.y1 = coor.c1y+coor.d1y*coor.n1y;
        coor.d1x = sim.resolution;
        coor.N1x = sim.N;
        coor.w1x = W;  
        coor.c1x = -coor.w1x/2+coor.d1x/2;
        coor.n1x = 0:(coor.N1x-1);
        coor.x1 = coor.c1x+coor.d1x*coor.n1x;
        [X1, Y1] = meshgrid(coor.x1, coor.y1);
        disp('Simulation coordinates defined.')

        %%% Sample the scene
        texture_map_sampled = interp2(display.x1, display.y1, ...
                texture_map, X1, Y1, 'nearest', 0);
        diopter_map_sampled = interp2(display.x1, display.y1, ...
                io.diopter_map, X1, Y1, 'nearest', 0);
        binary_mask_sampled = interp2(display.x1, display.y1, ...
                ones(size(io.diopter_map)), X1, Y1, 'nearest', 0);

        %%% Quantize the depth map
        if flags.quantization
            [map_idx,map_vals] = discretize(diopter_map_sampled, ...
                                            sim.num_depths);
            diopter_map_sampled = map_vals(map_idx);
        end

        %%% Crop out excess regions
        io.texture_map_sampled = texture_map_sampled.*( ...
                abs(X1)<=display.apture/2).*(abs(Y1)<=display.apture/2);
        io.diopter_map_sampled = diopter_map_sampled.*( ...
                abs(X1)<=display.apture/2).*(abs(Y1)<=display.apture/2);
        io.binary_mask_sampled = binary_mask_sampled.*( ...
                abs(X1)<=display.apture/2).*(abs(Y1)<=display.apture/2);
        disp('Scene sampled.')

        %%% Define viewing coordinates
        d9y = hardware.cam_pixel_pitch; N9y = sim.cam_resolution(1);
        d9x = hardware.cam_pixel_pitch; N9x = sim.cam_resolution(2);
        w9y = sim.cam_resolution(1) * hardware.cam_pixel_pitch;  
        w9x = sim.cam_resolution(2) * hardware.cam_pixel_pitch;  
        c9y = -w9y/2 + d9y/2; n9y = 0:(N9y-1); y9 = c9y + d9y * n9y;
        c9x = -w9x/2 + d9x/2; n9x = 0:(N9x-1); x9 = c9x + d9x * n9x;
        [sim.X9, sim.Y9] = meshgrid(x9, y9);
        d7 = (eye.eyepiece_f/hardware.f0)*sim.resolution;
        N7 = abs(round(hardware.lambda*eye.eyepiece_f/(sim.resolution*d7)));
        d7 = hardware.lambda*eye.eyepiece_f/(sim.resolution*N7);
        d8 = (eye.eye_v/eye.eyepiece_f)*d7;
        sim.N8 = abs(round(hardware.lambda*eye.eye_v/(d7*d8)));
    
        %%% Begin Propagation
        disp('Preparing for simulation...')
        num_focus_images = numel(eye.eye_focal_lengths);
        u8_stack_total = zeros(sim.N8, sim.N8, num_focus_images);
        disp(strcat('Simulation started... 0/',num2str(sim.num_iter)))
        for kk=1:sim.num_iter

            % initialize each point source with a random phase
            io.u1_phi = rand(size(io.texture_map_sampled)); 
            io.u1 = io.texture_map_sampled.*exp(1j*2*pi.*io.u1_phi);
            
            % propagate from u1 to u8
            [u8_stack, sim] = render(hardware, coor, eye, io, sim);

            % add up iterations with random initial phases
            u8_stack_normalized = (1/sim.num_iter)*abs(u8_stack).^2;
            u8_stack_total = u8_stack_total + u8_stack_normalized;

            % report progress
            disp(strcat('Finished iteration: ', color_name, ', ', ...
                        num2str(kk), '/',num2str(sim.num_iter)))
            toc
        end
    
        %%% Interpolate to camera coordinates
        focal_stack(:,:,idx,:) = interp_for_camera(u8_stack_total, sim);

        %%% Print progress
        fprintf(strcat('-------------------//', color_name, ...
                    '//-------------------\n'))

    end

end