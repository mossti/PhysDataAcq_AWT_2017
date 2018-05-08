// get the coefficients
float64 ai0CoeffVals[4];
DAQmxErrChk( DAQmxGetAIDevScalingCoeff(hAItask, "Dev1/ai1", ai0CoeffVals, 4) );    // 4 = NUM_COEFF_VALS

// get the uncalibrated binary value
UnCalBinVal = AIbuffer[ii];

// get the calibrated, scaled voltage
dCalVoltVal =  ai0CoeffVals[0] + (ai0CoeffVals[1] * UnCalBinVal) + (ai0CoeffVals[2] * (UnCalBinVal^2) ) + (ai0CoeffVals[3] * (UnCalBinVal^3) );

// then convert the voltage back to a CALIBRATED binary value (10.0 is 1/2 the p-to-p voltage).
//This is a bit of a kludge to be worked on later but should be only off  by 1 LSB
CalBinVal = ((dCalVoltVal / 10.0) * 32767);





float64 ao0CoeffVals[4];
DAQmxErrChk( DAQmxGetAODevScalingCoeff(hAItask, "Dev1/ao1", ao0CoeffVals, 4) );    // 4 = NUM_COEFF_VALS

