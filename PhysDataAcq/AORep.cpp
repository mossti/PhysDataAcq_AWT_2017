#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>
#include <conio.h>
#include <NIDAQmx.h>
#include "AORep.h"
#include "FileCtrl.h"

#define DAQmxErrChk(functionCall) { if( DAQmxFailed(error=(functionCall)) ) { goto Error; } }

using namespace std;

void AORep( MenuReturnValues mValues )
{
	// General parameters
	uInt32	SampPerChan_StimBuf		= 0;
	uInt32	SampPerChan_HalfStimBuf	= 0;

	bool32	done			= 0;
	char	errBuff[2048]	= {'\0'};
	int32	error			= 0;
	int32	totalRead		= 0;

	// Stimulus parameters
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int32 		NumAOSampWritten= 0;
	ifstream*	ptrAOFile;
	string		strAOFilename;
	TaskHandle	hAOtask			= 0;
	vector<double>	AOBuffer;
		
	strAOFilename = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0  + ".dat";	//Name of Analog Stimulus file
	ptrAOFile = fnOpenFileToRead(strAOFilename, &SampPerChan_StimBuf);	//Call function to open Analog Output file
	SampPerChan_HalfStimBuf = SampPerChan_StimBuf/2;	// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	AOBuffer.assign(SampPerChan_StimBuf,0);

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev1"));

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&hAOtask));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(hAOtask,"/Dev1/ao0","",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(hAOtask,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SampPerChan_StimBuf));

	/*********************************************/
	// DAQmx Allow Regeneration
	/*********************************************/
	DAQmxErrChk (DAQmxSetWriteRegenMode(hAOtask,DAQmx_Val_AllowRegen));

	/*********************************************/
	// DAQmx Initial Write Code
	/*********************************************/
	LoadFullAOBuffer(&AOBuffer[0],NumAOChannels,SampPerChan_StimBuf,ptrAOFile);
	DAQmxErrChk (DAQmxWriteAnalogF64(hAOtask,SampPerChan_StimBuf,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByChannel,&AOBuffer[0],&NumAOSampWritten,NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(hAOtask));

	int counter = 0;
	while ( !done && !kbhit() )
	{ 
		// PLAY OUTPUT CONTINUOUSLY
		if (counter == 0)
			cout << "Playing output continuously: \t" << endl;
		counter=1;
	}
	getchar();
	ptrAOFile->close();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	/*********************************************/
	// DAQmx Stop Code
	/*********************************************/
	if( hAOtask!=0 )
	{
		DAQmxStopTask(hAOtask);
		DAQmxClearTask(hAOtask);
	}
	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);


	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev1"));


	cout << "End of trial. Press Enter to continue!" << endl;
	getchar();
	return;

}