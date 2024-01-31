function [u2, x2, c2x, n2x, d2x, y2, c2y, n2y, d2y] = ...
            lens_fourier_propagate( ...
            u1, n1x, d1x, c1x, n1y, d1y, c1y, lambda, f0, fold)

%%% Define DFT coordinates
d2x = (f0/fold)*d1x;
d2y = (f0/fold)*d1y;
N2x = abs(round(lambda*f0/(d1x*d2x)));
N2y = abs(round(lambda*f0/(d1y*d2y)));
d2x = lambda*f0/(d1x*N2x);
d2y = lambda*f0/(d1y*N2y);
c2x = -N2x*d2x/2; n2x = 0:N2x-1;
c2y = -N2y*d2y/2; n2y = 0:N2y-1;
x2 = c2x+d2x*n2x;
y2 = c2y+d2y*n2y;

%%% Perform DFT
u1_2 = u1.*exp(-1i*2*pi*((d1x*c2x.*n1x)+((d1y*c2y.*n1y).'))/(lambda*f0));
u1_2_hat = fft2(u1_2, N2y, N2x);
scaling_factor = (d1y*d1x/(lambda*f0));
h = scaling_factor*exp(-1i*2*pi*((c1x*x2)+((c1y*y2).'))/(lambda*f0));
u2 = h.*u1_2_hat;

%%% Print out parameter values
disp(strcat('lens_fourier_propagate f=', num2str(f0),', d1x=', ...
    num2str(d1x),', d2x=',num2str(d2x),', N2x=',num2str(N2x), ', w1x=', ...
    num2str(numel(n1x)*d1x),', w2x=',num2str(N2x*d2x)))

end