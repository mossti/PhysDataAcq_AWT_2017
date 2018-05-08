//Things to do:
// Need to add a way to re-read from a small x, y coordinate file since these values are repeated
//Add the input to the SCB to record a second channel and test the values recorded

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <NIDAQmx.h>

#include "AOContDITimeStamp.h"
#include "FileCtrl.h"
#include "MenuReturnValues.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void AOContDITimeStamp( MenuReturnValues mValues, int idx )
{
	bool32		done=0;
	char		errBuff[2048]={'\0'};
	int32		totalReadCh0=0;
	int32		totalReadCh1=0;
	int32		error=0;
	uInt32		NHalfBufs=0;
	uInt32		NZSample=0;
	uInt32		TotNZSample=0;
	double		outputBuffer;

	// Initialize variables for the Counters, and Digital Input Timestamps
	int32		samplesPerChanReadCICh0 = 0;
	int32		samplesPerChanReadCICh1 = 0;
	int32		numSampsPerChan = mValues.slNumSampsPerChan; //This # is chosen using an estimated H1 firing rate of 500 spikes/s & => at least 4 seconds before acquiring 2000 samples
	uInt32		ReadBufferSize  = mValues.ulReadBufferSize; //This # is chosen based on 2 estimates: 1)Smallish frequency of disk writes, 2)H1 firing rate ~500 Spikes/s
	double		CIRecTimeout	= mValues.dCIRecTimeOut;
	ofstream*	ptrAIFileCh0	= 0;
	ofstream*	ptrAIFileCh1	= 0;
	string		strAIFileNameCh0;
	string		strAIFileNameCh1;

	// Initialize variables for the Analog Output (Z-signal for 608 oscilloscope)
	bool		AOAutoStart		= mValues.bStimAutoStart;
	double		StimMaxAmp		= mValues.dStimMaxVoltVal;
	double		StimMinAmp		= mValues.dStimMinVoltVal;
	double		StimSampRate	= mValues.dStimSampRate;
	float64		AOStimTimeout	= mValues.dStimTimeOut;
	float64		ActualSampRate	= 0;
	int16		NumAOChannels	= mValues.iNumNIAOChans;
	int16		NRepeats		= mValues.iNumRepeats;
	int32 		NumAOSampWritten= 0;
	ifstream*	ptrAOFile		= 0;
	uInt32		AOOneChanBufSiz	= StimSampRate;
	uInt32		AOBuffer_Siz	= AOOneChanBufSiz*NumAOChannels*2;	//KEY FOR THIS CODE WORKING IS THAT THE DIGITAL BUFFER BE > 2046 SAMPLES
	uInt32		AOHalfBuf_Siz	= AOOneChanBufSiz*NumAOChannels;
	uInt32		AOBufferSpaceFree=0;
	uInt32		AOFileSize		= 0;
	string		strAOFileName;
	
	int			SingleChannel	= mValues.iNumUEIAOChans;
	char		PhysicalAOChannels[15];
	if (SingleChannel == 1)
		sprintf(PhysicalAOChannels, "/Dev2/ao%i", NumAOChannels-1);
	else if (SingleChannel == 0)
		sprintf(PhysicalAOChannels, "/Dev2/ao0:%i", NumAOChannels-1);
	else {
		cout << "Menu Option H Set Incorrectly" << endl;
		return;
	}

	// Initialize variables for the Analog Output (X-signal for 608 oscilloscope)
	ifstream*	ptrAO_X_File	= 0;
	uInt32		AO_X_FileSize	= 0;
	string		strAO_X_FileName;
	vector<double>	X;
	
	// Initialize variables for the Analog Output (Y-signal for 608 oscilloscope)
	ifstream*	ptrAO_Y_File	= 0;
	uInt32		AO_Y_FileSize	= 0;
	string		strAO_Y_FileName;
	vector<double>	Y;
	
	// Create and initialize arrays and vectors
	vector<uInt32>	readArrayCICh0;
	vector<uInt32>	readArrayCICh1;
	vector<double>	dAOBuffer;
	readArrayCICh0.assign(ReadBufferSize,0);
	readArrayCICh1.assign(ReadBufferSize,0);
	dAOBuffer.assign(AOBuffer_Siz,0);

	// Initialize the handle to the NI tasks
	TaskHandle  CO1Handle = 0;
	TaskHandle  CO2Handle = 0;
	TaskHandle  AOHandle  = 0;


	/*********************************************/	
	// Initialize the Response files
	/*********************************************/
	char fileIndex[65];
	itoa(idx,fileIndex,10);
	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "_Ch1.dat" << endl;
	strAIFileNameCh0 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch1.dat";
	ptrAIFileCh0	 = fnOpenFileToWrite( strAIFileNameCh0 );

	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << "_Ch2.dat" << endl;
	strAIFileNameCh1 = mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + "_Ch2.dat";
	ptrAIFileCh1	 = fnOpenFileToWrite( strAIFileNameCh1 );


	/*********************************************/	
	// Initialize the Stimulus file (Z-signal)
	/*********************************************/
	strAOFileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName0 + ".dat";	//Name of Analog Stimulus file
	//ptrAOFile = fnOpenLargeFileToRead(strAOFileName,&AOFileSize);	//Call function to open Analog Output file
	ptrAOFile = fnOpenFileToRead(strAOFileName, &AOFileSize);	//Call function to open Analog Output file

	/*********************************************/	
	// Initialize the Stimulus file (X-signal)
	/*********************************************/
	strAO_X_FileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName1 + ".dat";	//Name of Analog Stimulus file
	ptrAO_X_File = fnOpenFileToRead(strAO_X_FileName, &AO_X_FileSize); //Call function to open Analog Output file
	X.assign(AO_X_FileSize,0);
	for( uInt32 idx2=0; idx2<AO_X_FileSize; idx2++ ) {
		ptrAO_X_File->read( reinterpret_cast<char *>( &outputBuffer ), sizeof(double));
		X[idx2] = outputBuffer;
	}
	ptrAO_X_File->close();

	/*********************************************/	
	// Initialize the Stimulus file (Y-signal)
	/*********************************************/
	strAO_Y_FileName = mValues.strStimFileDirPath + "\\" + mValues.strStimFileName2 + ".dat";	//Name of Analog Stimulus file
	ptrAO_Y_File = fnOpenFileToRead(strAO_Y_FileName, &AO_Y_FileSize); //Call function to open Analog Output file
	Y.assign(AO_Y_FileSize,0);
	for( idx2=0; idx2<AO_Y_FileSize; idx2++ ) {
		ptrAO_Y_File->read( reinterpret_cast<char *>( &outputBuffer ), sizeof(double));
		Y[idx2] = outputBuffer;
	}
	ptrAO_Y_File->close();


	/*********************************************/	
	// Load Analog Output Buffer
	/*********************************************/
	NZSample = ConstructAOBuffer(&dAOBuffer[0], AOOneChanBufSiz*2, AOFileSize, AO_X_FileSize, ptrAOFile, &X[0], &Y[0]); //The first time this should load the full buffer
	TotNZSample += NZSample;
	cout << "Number of Z samples written: " << TotNZSample << endl;
	if ((TotNZSample % AOFileSize) == 0)
		ptrAOFile->seekg(0, ios::beg );
	cout << "Z reset" << endl;


	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev2"));


	/*********************************************/
	// DAQmx Configure Code for Counter 1 - Timestamp channel 1
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO1",&CO1Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO1Handle,"Dev2/ctr0","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO1Handle,"/Dev2/PFI9",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,5000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO1Handle,"","/Dev2/100kHzTimebase"));


	/*********************************************/
	// DAQmx Configure Code for Counter 2 - Timestamp channel 2
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("CO2",&CO2Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO2Handle,"Dev2/ctr1","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO2Handle,"/Dev2/PFI4",100000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,5000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO2Handle,"","/Dev2/100kHzTimebase"));


	/*********************************************/
	// DAQmx Configure Code for Analog Output
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("AO",&AOHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(AOHandle,PhysicalAOChannels,"",StimMinAmp,StimMaxAmp,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(AOHandle,"",StimSampRate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,AOOneChanBufSiz));
	DAQmxErrChk (DAQmxCfgOutputBuffer(AOHandle,AOOneChanBufSiz*2)); // LOOK AT THIS!@!! Made this change and need to check. Switched AOHalfBuf_Siz for AOOneChanBufSiz
	DAQmxErrChk (DAQmxSetWriteRegenMode(AOHandle,DAQmx_Val_DoNotAllowRegen));
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(AOHandle,"/Dev2/PFI6",DAQmx_Val_Rising));

	//Read the actual sample clock rate (eventually coerced depending on the hardware used).
	DAQmxErrChk (DAQmxGetSampClkRate(AOHandle, &ActualSampRate));
	cout << "The actual sample clock rate is: " << ActualSampRate << endl << endl;

	/*********************************************/
	// DAQmx Initial Analog Output Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,AOOneChanBufSiz*2,AOAutoStart,AOStimTimeout,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));


	/*********************************************/
	// Everything is prepared, start operation
	/*********************************************/
	cout << "Continuously Timestamping Data." << endl;
	cout << "Continuously Producing Raster for Tektronix 608." << endl;
	cout << "Press any key to terminate trial." << endl << endl;
	cout << "Number of half buffers to play: " << (AOFileSize/AOOneChanBufSiz*NRepeats) << endl;

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(AOHandle));
	DAQmxErrChk (DAQmxStartTask(CO2Handle));
	DAQmxErrChk (DAQmxStartTask(CO1Handle));

	while ( !done && !kbhit() && NHalfBufs < (AOFileSize/AOOneChanBufSiz*NRepeats) )
	{
		//********************************************
		// DAQmx record the digital events
		//********************************************
		DAQmxErrChk (DAQmxReadCounterU32(CO1Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh0[0],ReadBufferSize,&samplesPerChanReadCICh0,NULL));
		if (samplesPerChanReadCICh0 > 0) {
			totalReadCh0 += samplesPerChanReadCICh0;
			ptrAIFileCh0->write(reinterpret_cast<char*>(&readArrayCICh0[0]),sizeof(uInt32)*samplesPerChanReadCICh0);
			if (totalReadCh0%numSampsPerChan == 0) {
				cout << "Total # of Spikes Recorded on Ch 1: \t" << totalReadCh0 << endl;
			}
		}

		DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,DAQmx_Val_Auto,CIRecTimeout,&readArrayCICh1[0],ReadBufferSize,&samplesPerChanReadCICh1,NULL));
		if (samplesPerChanReadCICh1 > 0) {
			totalReadCh1 += samplesPerChanReadCICh1;
			ptrAIFileCh1->write(reinterpret_cast<char*>(&readArrayCICh1[0]),sizeof(uInt32)*samplesPerChanReadCICh1);
			if (totalReadCh1%numSampsPerChan == 0) {
				cout << "Total # of Spikes Recorded on Ch 2: \t" << totalReadCh1 << endl;
			}
		}

		//********************************************
		// DAQmx Check output buffer space available
		//********************************************
		DAQmxErrChk (DAQmxGetWriteSpaceAvail(AOHandle, &AOBufferSpaceFree));
		if(AOBufferSpaceFree > AOOneChanBufSiz) {  // Made this change and need to check. Switched AOHalfBuf_Siz for AOOneChanBufSiz
			++NHalfBufs;
			NZSample = ConstructAOBuffer(&dAOBuffer[0],AOOneChanBufSiz,AOFileSize,AO_X_FileSize,ptrAOFile,&X[0],&Y[0]);
			DAQmxErrChk (DAQmxWriteAnalogF64(AOHandle,AOOneChanBufSiz,0,1,DAQmx_Val_GroupByScanNumber,&dAOBuffer[0],&NumAOSampWritten,NULL));
			TotNZSample += NZSample;
			cout << "AO Buffer Space: " << AOBufferSpaceFree << " ; AO Samples Written: " << NumAOSampWritten*NumAOChannels << endl << endl;

			if ((TotNZSample % AOFileSize) == 0)
				ptrAOFile->seekg(0, ios::beg );
			cout << "Z reset" << endl;
		}
	}
	getchar();
	cout << "Number of half buffer transfered: " << NHalfBufs << endl;
	cout << "Number of samples transfered : " << TotNZSample << endl;

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( CO1Handle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(CO1Handle);
		DAQmxClearTask(CO1Handle);
	}
	if( CO2Handle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(CO2Handle);
		DAQmxClearTask(CO2Handle);
	}
	if( AOHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(AOHandle);
		DAQmxClearTask(AOHandle);
	}

	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	ptrAIFileCh0->close();
	ptrAIFileCh1->close();
	ptrAOFile->close();
	//ptrAO_X_File->close();
	//ptrAO_Y_File->close();
	DAQmxErrChk (DAQmxResetDevice("Dev2"));

	return;

}