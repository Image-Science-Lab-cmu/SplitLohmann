function [u8_stack, sim] = render(hardware, coor, eye, io, sim)

lambda = hardware.lambda;
f0 = hardware.f0;
C0 = hardware.C0;
W = hardware.working_range;
fe = eye.eyepiece_f;
eyepf = eye.eyepiece_f;

%%% propagate from u1 to u2
[u2, x2, c2x, n2x, d2x, y2, c2y, n2y, d2y] = lens_fourier_propagate( ...
    io.u1, coor.n1x, coor.d1x, coor.c1x, coor.n1y, coor.d1y, coor.c1y, ...
    lambda, f0, f0);

%%% propagate from u2 to u2_aptured_bpp (first bicubic phase plate)
T1 = exp((-1i*2*pi/(lambda*C0))*(x2.^3 + (y2.').^3));
u2_bpp_aptured = u2.*T1.*((x2.^2+(y2.').^2) < (hardware.aperture_T1/2)^2);

%%% propagate from u2_bpp_aptured to u3
[u3, x3, c3x, n3x, d3x, y3, c3y, n3y, d3y] = lens_fourier_propagate( ...
    u2_bpp_aptured, n2x, d2x, c2x, n2y, d2y, c2y, lambda, f0, f0);

%%% add phase delays by the SLM
diopterMap_flipped = flip(flip(io.diopter_map_sampled,1),2);
Delta = (fe^2/f0^2).*(C0/6).*(W/2 - diopterMap_flipped);
phase_slm_mask = exp(1i*(2*pi/(lambda*f0)).*Delta.*(x3+y3.'));
u3_SLM = u3 .* io.binary_mask_sampled .* phase_slm_mask;

%%% propagate from u3_SLM to u4
[u4, x4, c4x, n4x, d4x, y4, c4y, n4y, d4y] = lens_fourier_propagate( ...
    u3_SLM, n3x, d3x, c3x, n3y, d3y, c3y, lambda, f0, f0);

%%% propagate from u4 to u4_bpp_aptured (first bicubic phase plate)
T2 = exp((1i*2*pi/(lambda*C0))*(-(x4.^3 + (y4.').^3)));
u4_bpp_aptured = u4.*T2.*((x4.^2+(y4.').^2) < (hardware.aperture_T2/2)^2);

%%$ propagate from u4_bpp_aptured to u5 (nominal plane of focal stack)
[u5, ~, c5x, n5x, d5x, ~, c5y, n5y, d5y] = lens_fourier_propagate( ...
    u4_bpp_aptured, n4x, d4x, c4x, n4y, d4y, c4y, lambda, f0, f0);

%%% propagate from u5 to u7_apertured (eye)
[u7_, x7, c7x, n7x, d7x, y7, c7y, n7y, d7y] = lens_fourier_propagate( ...
    u5, n5x, d5x, c5x, n5y, d5y, c5y, lambda, eyepf, f0);
term2 = -(1j*2*pi./(lambda*2*eyepf))*(eye.delZ_half_max/eyepf);
u7 = u7_ .* exp(-term2.*(x7.^2+(y7.').^2));
u7_apertured = u7 .* ((x7.^2+(y7.').^2) <= (eye.eye_diameter/2)^2);

%%$ propagate from u7_apertured to u8 (retina)
u8_stack = zeros(sim.N8, sim.N8, numel(eye.eye_focal_lengths));
for ii=1:numel(eye.eye_focal_lengths)
    
    Peye = 1/eye.eye_focal_lengths(ii);
    Pv = Peye - 1/eye.eye_v;
    term3 = (-1j*2*pi./(lambda*2*(1/Pv)));

    u7_plus = u7_apertured .* exp(term3.*(x7.^2+(y7.').^2));
    [u8_minus, x8, ~, ~, ~, y8, ~, ~, ~] = lens_fourier_propagate( ...
        u7_plus, n7x, d7x, c7x, n7y, d7y, c7y, lambda, eye.eye_v, eyepf);

    term4 = 1j*2*pi./(lambda*2*eye.eye_v);
    u8 = u8_minus .* exp(term4.*(x8.^2+(y8.').^2));
    u8_stack(:,:,ii) = u8;
end

sim.x8 = x8;
sim.y8 = y8;

end
