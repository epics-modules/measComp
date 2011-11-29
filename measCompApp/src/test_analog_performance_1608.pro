pro test_analog_performance_1608,  ao=ao, ai=ai, min_volts=min_volts, max_volts=max_volts, $
                                   step_volts=step_volts, num_samples=num_samples, delay=delay, $
                                   keithley=keithley, results
                                   
  if (n_elements(ao)          eq 0) then ao          = '1608G:Ao1'
  if (n_elements(ai)          eq 0) then ai          = '1608G:Ai1'
  if (n_elements(min_volts)   eq 0) then min_volts   = -10.0
  if (n_elements(max_volts)   eq 0) then max_volts   =  10.0
  if (n_elements(step_volts)  eq 0) then step_volts  = 0.1
  if (n_elements(num_samples) eq 0) then num_samples = 10 
  if (n_elements(delay)       eq 0) then delay       = 0.1
  if (n_elements(keithley)    eq 0) then keithley    = '13LAB:DMM2Dmm_raw.VAL'

  output = min_volts
  samples = dblarr(num_samples)
  num_points = ((max_volts - min_volts) / step_volts + 0.5) + 1
  results = dblarr(4, num_points)
  for i=0, num_points-1 do begin
    output = min_volts + i*step_volts
    t = caput(ao, output)
    wait, 2*delay
    for j=0, num_samples-1 do begin
      wait, delay
      t = caget(ai, temp)
      samples[j] = temp
    endfor
    m = moment(samples)
    results[0,i] = output
    results[1,i] = m[0]
    results[2,i] = sqrt(m[1])
    t = caget(keithley, temp)
    results[3,i] = temp
    print, results[0,i], results[1,i], results[2,i], results[3,i]
  endfor
end
