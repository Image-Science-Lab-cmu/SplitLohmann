function compile_gif(color_stack, io, flags)

    constant = io.gamma_correction_constant;
    gamma_inv = io.gamma_correction_gamma_inv;
    duration = io.gif_frame_duration;
    save_path = io.save_path;

    num_images = size(color_stack, 4);

    max_img_val = max(max(max(max((color_stack)))));
    img_stack = uint8(zeros(size(color_stack)));
    color_stack = color_stack./max_img_val;
    color_stack = constant.*double(color_stack).^(1/gamma_inv);
    max_img_val = max(max(max(max((color_stack)))));
    color_stack = color_stack./max_img_val;

    %%% process image
    for ii = 1:num_images
        img_u8 = color_stack(:,:,:,ii);
        img_to_save = uint8(round(img_u8*255));
        img_to_save = flip(flip(img_to_save,2),1);
        img_stack(:,:,:,ii) = img_to_save;

        %%% write image
        imgsave_path = strcat(save_path,'-focus_',num2str(ii),'.png');
        if flags.color
            imwrite(img_to_save, imgsave_path)
        else
            imwrite(img_to_save(:,:,1), imgsave_path)
        end
    end

    %%% create gif
    for jj = 1:uint16(num_images*2-2)
        if jj > num_images
            kk = num_images - mod(jj,num_images);
        else
            kk = jj;
        end
        img_to_save = img_stack(:,:,:,kk);
        [A,map] = rgb2ind(uint8(img_to_save),256);
        gif_save_path = strcat(save_path,'.gif');

        if jj == 1
            if flags.color
                imwrite(A, map, gif_save_path, ...
                    'gif', 'LoopCount', Inf, 'DelayTime', duration);
            else
                imwrite(uint8(img_to_save(:,:,1)), gif_save_path, ...
                    'gif', 'LoopCount', Inf, 'DelayTime', duration);
            end
        else
            if flags.color
                imwrite(A, map, gif_save_path, ...
                    'gif', 'WriteMode', 'append', 'DelayTime', duration);
            else
                imwrite(uint8(img_to_save(:,:,1)), gif_save_path, ...
                    'gif', 'WriteMode', 'append', 'DelayTime', duration);
            end
        end
    end

    disp(strcat('Gif movie written to disk at: ', gif_save_path));

end
