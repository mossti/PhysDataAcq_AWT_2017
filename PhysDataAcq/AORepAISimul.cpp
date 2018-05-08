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
#include "AORepAISimul.h"
#include "FileCtrl.h"

#define DAQmxErrChk(functionCall) { if( DAQmxFailed(error=(functionCall)) ) { goto Error; } }

using namespace std;

void AORepAISimul( MenuReturnValues mValues )
{
	// General parameters
	uInt32 SampPerChan_StimBuf		= 0;	// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	uInt32 SampPerChan_HalfStimBuf	= 0;
	int32 SampPerChan_RespBuf		= 0;	// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	int32 SampPerChan_HalfRespBuf	= 0;

	bool32		done			= 0;
	char		errBuff[2048]	= {'\0'};
	int32		error			= 0;
	int32		totalRead		= 0;

	// Stimulus parameters
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int32 		NumAOSampWritten= 0;
	ifstream*	ptrAOFile		= 0;
	string		strAOFilename	= "";
	TaskHandle	hAOtask			= 0;
	vector<double>	AOBuffer;
	
	strAOFilename = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0  + ".dat";	//Name of Analog Stimulus file
	ptrAOFile = fnOpenFileToRead(strAOFilename, &SampPerChan_StimBuf);	//Call function to open Analog Output file
	SampPerChan_HalfStimBuf = SampPerChan_StimBuf/2;	// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	AOBuffer.assign(SampPerChan_StimBuf,0);

	// Recording parameters
	double		RecMaxAmp		= mValues.dRecMaxVoltVal;
	double		RecMinAmp		= mValues.dRecMinVoltVal;
	double		RecSampRate		= mValues.dRecSampRate;
	float64		AIRecTimeout	= mValues.dAIRecTimeOut;
	int16		NumAIChannels	= mValues.iNumAIChans;
	int32		NumAISampRead	= 0;
	ofstream*	ptrAIFile		= 0;
	string		strAIFilename	= "";
	TaskHandle  hAItask			= 0;
	vector<double>	AIBuffer;

	SampPerChan_HalfRespBuf	= 1000;//SampPerChan_HalfStimBuf; //This works well if the RespBuff is same size as StimBuf
	SampPerChan_RespBuf		= 2000;//SampPerChan_StimBuf;	// ONE KEY TO THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	strAIFilename = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + ".dat"; //Name of Analog Input file
	ptrAIFile = fnOpenFileToWrite(strAIFilename);
	AIBuffer.assign(SampPerChan_RespBuf,0);	

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev1"));

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&hAItask));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(hAItask,"/Dev1/ai0","",DAQmx_Val_NRSE,RecMinAmp,RecMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(hAItask,"OnboardClock",RecSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SampPerChan_RespBuf));

	DAQmxErrChk (DAQmxCreateTask("",&hAOtask));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(hAOtask,"/Dev1/ao0","",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(hAOtask,"/Dev1/ai/SampleClock",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,SampPerChan_StimBuf));

	/*********************************************/
	// DAQmx Allow Regeneration
	/*********************************************/
	DAQmxErrChk (DAQmxSetWriteRegenMode(hAOtask,DAQmx_Val_AllowRegen));

	/*********************************************/
	// DAQmx Initial Write Code
	/*********************************************/
	LoadFullAOBuffer(&AOBuffer[0],NumAOChannels,SampPerChan_StimBuf,ptrAOFile);
	// load initial AObuffer[] with full SineWave
	//for (i=0; i<SampPerChan_StimBuf; i++)
	//AObuffer[i] = 9.95*sin((double)i*2.0*PI/SAMPLE_OUTPUT_RATE);        

	DAQmxErrChk (DAQmxWriteAnalogF64(hAOtask,SampPerChan_StimBuf,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByChannel,&AOBuffer[0],&NumAOSampWritten,NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(hAOtask));
	DAQmxErrChk (DAQmxStartTask(hAItask));

	double NumReps = mValues.iNumRepeats*(SampPerChan_StimBuf/SampPerChan_HalfRespBuf);
	cout << "Number of repeats : " << NumReps << endl;
	for (double i=0; i<NumReps; i++)	//Play once and repeat N-1 times
	{      
		//********************************************
		// DAQmx Check available space code
		//********************************************
		DAQmxErrChk (DAQmxReadAnalogF64(hAItask,SampPerChan_HalfRespBuf,AIRecTimeout,DAQmx_Val_GroupByScanNumber,&AIBuffer[0],SampPerChan_RespBuf,&NumAISampRead,NULL));
		if( NumAISampRead>0 ) {
			totalRead += NumAISampRead;
			cout<<" Half Buffers Filled : \t" << i << endl;
			cout<<" Total Samples Read  : \t" << totalRead << endl;
			ptrAIFile->write(reinterpret_cast<char*>(&AIBuffer[0]),sizeof(double)*NumAISampRead);
		}
	}
	ptrAIFile->close();
	ptrAOFile->close();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	/*********************************************/
	// DAQmx Stop Code
	/*********************************************/
	if( hAItask!=0 )
	{
		DAQmxStopTask(hAOtask);
		DAQmxClearTask(hAOtask);
	}
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