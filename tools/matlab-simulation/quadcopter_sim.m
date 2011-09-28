%
%  RoflCopter quadrotor project
%  MATLAB Autopilot & Simulation
%
%  (C) 2011 Attila Incze <attila.incze@gmail.com>
%  http://atimb.me
%
%  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
%  http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
%

clear all, clc

m1 = 0.25; % in kg; the engines
m2 = 0.25;
m3 = 0.25;
m4 = 0.25;
d1 = 0.35;% + normrnd(0, 0.01) % in m; the engines
d2 = 0.35;% + normrnd(0, 0.01)
d3 = 0.35;% + normrnd(0, 0.01)
d4 = 0.35;% + normrnd(0, 0.01)
I_ = [m1*d1^2 + m3*d3^2, 0, 0; 0, m2*d2^2 + m4*d4^2, 0; 0, 0, m1*d1^2 + m2*d2^2 + m3*d3^2 + m4*d4^2];
Ii = inv(I_);
M = m1 + m2 + m3 + m4;
h = 0.01; % in s; the time step

ntime = 2000;

r = zeros(3, ntime);
v = zeros(3, ntime);
A = zeros(3, 3, ntime);
L = zeros(3, ntime);
I = zeros(3, 3, ntime);
w = zeros(3, ntime);
o = zeros(3, ntime);

% Initial conditions
r(:,1) = [0; 0; 1];
v(:,1) = [0; 0; 0];
A(:,:,1) = eye(3);
L(:,1) = [0; 0; 0];
o(:,1) = [0; 0; 0];

I(:,:,1) = A(:,:,1)*Ii*A(:,:,1)';
w(:,1) = I(:,:,1)*L(:,1);

% Force in lab coordinates
F = [0; 0; 0];
% Torque in body coordinates
t = [0; 0; 0];

i_ = [1; 0; 0];
j_ = [0; 1; 0];
k_ = [0; 0; 1];

%L(3,1) = 0.1;
L(2,1) = 0.01;
%L(1,1) = 0.1;

gyro_const = 40; % Modifier, for tuning
accel_const = 1;

gyro_bandwidth = 20; % Hz
accel_bandwidth = 20; %Hz

gyro_noise = 0.05; % Gauss fwhm
accel_noise = 0.05;

samplerate = (1/gyro_bandwidth)/h;
samplecounter = 0;

engine_varspeed = 9; % Newton / second varying speed

e = zeros(4,1);
e_ = zeros(4,1);
e_(1) = 2.5;
e_(2) = 2.5;
e_(3) = 2.5;
e_(4) = 2.5;
e = e_;

for i=1:ntime-1
    % ----- Calculate forces -------
    % Controlling (feedback)
    samplecounter = samplecounter + 1;
    if (samplecounter == samplerate)
        samplecounter = 0;
        gyro = zeros(3,1);
        accel = zeros(3,1);
        %for j=i-samplerate:i
            gyro(:) = gyro(:) + w(:,i);
            %accel(:) = accel(:) + o(:,i);
        %end
        gyro(1) = gyro(1)/samplerate + normrnd(0, gyro_noise);
        gyro(2) = gyro(2)/samplerate + normrnd(0, gyro_noise);
        gyro(3) = gyro(3)/samplerate + normrnd(0, gyro_noise);
        accel(1) = accel(1)/samplerate + normrnd(0, accel_noise);
        accel(2) = accel(2)/samplerate + normrnd(0, accel_noise);
        accel(3) = accel(3)/samplerate + normrnd(0, accel_noise);
        e_(1) = 2.5 - gyro(3)*gyro_const - gyro(1)*gyro_const - accel(1)*accel_const;
        e_(2) = 2.5 + gyro(3)*gyro_const - gyro(2)*gyro_const - accel(2)*accel_const;
        e_(3) = 2.5 - gyro(3)*gyro_const + gyro(1)*gyro_const + accel(1)*accel_const;
        e_(4) = 2.5 + gyro(3)*gyro_const + gyro(2)*gyro_const + accel(2)*accel_const;
    end
    % Engines does not respond instantly, there is a delay
    %e = e + (e_ - e)*0.05;
    e = e + min(e_-e, engine_varspeed*h*sign(e_-e));
    % Calc sum force
    n_ = A(:,:,i)*k_; % Normal of the body plane
    F = e(1)*n_ + e(2)*n_ + e(3)*n_ + e(4)*n_; % Engines pull upwards
    % Gravity
    F = F + 10*M*[0; 0; -1];
    % Calc torque
    t = d1^2*e(1)*k_ - d2^2*e(2)*k_ + d3^2*e(3)*k_ - d4^2*e(4)*k_; % Yaw
    t = t + d1^2*e(1)*i_ + d2^2*e(2)*j_ - d3^2*e(3)*i_ - d4^2*e(4)*j_; % Roll and Pitch
    % ----- Integrate quantities -------
    r(:,i+1) = r(:,i) + h*v(:,i);
    v(:,i+1) = v(:,i) + F*h/M;
    wm = [0, -w(3,i), w(2,i); w(3,i), 0, -w(1,i); -w(2,i), w(1,i), 0];
    A(:,:,i+1) = A(:,:,i) + h*wm*A(:,:,i);
    L(:,i+1) = L(:,i) + h*t;
    o(:,i+1) = o(:,i) + h*w(:,i);
    % ------ Reorthogonalize A --------
    %A(:,:,i+1) = orth(A(:,:,i+1));
    A(:,1,i+1) = A(:,1,i+1)./norm(A(:,1,i+1));
    A(:,3,i+1) = cross(A(:,1,i+1),A(:,2,i+1))./norm(cross(A(:,1,i+1),A(:,2,i+1)));
    A(:,2,i+1) = cross(A(:,3,i+1),A(:,1,i+1))./norm(cross(A(:,3,i+1),A(:,1,i+1)));
    % ------ Auxiliary quantities ---------
    I(:,:,i+1) = A(:,:,i+1)*Ii*A(:,:,i+1)';
    w(:,i+1) = I(:,:,i+1)*L(:,i+1);
end

figure(1);
subplot(2,2,1);
time = linspace(0, h*ntime, ntime);
plot(time, r(1,:), time, r(2,:), time, r(3,:));
title('r1, r2, r3');
subplot(2,2,2);
plot(time, v(1,:), time, v(2,:), time, v(3,:));
title('v1, v2, v3');
subplot(2,2,3);
plot(time, o(1,:), time, o(2,:), time, o(3,:));
title('o1, o2, o3');
subplot(2,2,4);
plot(time, w(1,:), time, w(2,:), time, w(3,:));
title('w1, w2, w3');

scrsz = get(0,'ScreenSize');
figure('Position',[1 1 scrsz(3) scrsz(4)])
figure(2);
x0 = [-5, -5, -5, -5, 5, 5, 5, 5];
y0 = [-5, 5, -5, 5, -5, 5, -5, 5];
z0 = [0, 0, 3, 3, 0, 0, 3, 3];
x = zeros(length(x0));
y = zeros(length(y0));
z = zeros(length(z0));
tri = [1, 2, 3; 2, 3, 4; 5, 6, 7; 6, 7, 8; 2, 6, 4; 6, 8, 4; 5, 1, 7; 1, 3, 7];

for i=1:5:1000
    for j=1:length(x0)
        vect = [x0(j); y0(j); z0(j)];
        vect = A(:,:,i)*vect;
        x(j) = vect(1) + r(1,i);
        y(j) = vect(2) + r(2,i);
        z(j) = vect(3) + r(3,i);
    end
    trisurf(tri, x, y, z);
    axis([-30 30 -30 30 -30 30])
    title(['Seconds passed: ',num2str(i*h)]);
    pause(0.01);
end

