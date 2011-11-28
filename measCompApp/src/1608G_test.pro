; This program loads 2 user-defined waveforms for the 1608G

npts = 2048L
prefix = '1608G:WaveGen'
dwell = 1e-3
time = 2 * !pi * findgen(npts)/npts
volts1 = 3.*sin(time) + 2*sin(time*10) + 1.*cos(time*3)
volts2 = 2.*sin(time*4)
t = caput(prefix + 'Run',           0, /wait)
t = caput(prefix + 'UserNumPoints', npts)
t = caput(prefix + 'UserDwell',     dwell)
t = caput(prefix + '1Enable',       'Enable')
t = caput(prefix + '1Type',         'User-defined')
t = caput(prefix + '1UserWF',       volts1)
t = caput(prefix + '2Enable',       'Enable')
t = caput(prefix + '2Type',         'User-defined')
t = caput(prefix + '2UserWF',       volts2)
t = caput(prefix + 'Run',           1)
end


