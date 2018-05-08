#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <NIDAQmx.h>

#include "DOContDITimeStamp.h"
#include "FileCtrl.h"
#include "MenuReturnValues.h"
#include "UEILEDCtrl.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

using namespace std;

void DOContDITimeStamp( MenuReturnValues mValues, int idx )
{
	bool32	done = 0;
	char	errBuff[2048]={'\0'};
	int32	totalRead = 0;
	int32	error=0;

	int32	samplesPerChanRead;
	int32	DOArraySize		= mValues.slDOArraySize;
	int32	numSampsPerChan = mValues.slNumSampsPerChan;  //This # is chosen using an estimated H1 firing rate of 500 spikes/s & => at least 4 seconds before acquiring 2000 samples
	uInt32	ReadBufferSize  = mValues.ulReadBufferSize; //This # is chosen based on 2 estimates: 1)Smallish frequency of disk writes, 2)H1 firing rate ~500 Spikes/s
	double	CIReadTimeout	= mValues.dCIRecTimeOut;
	ofstream* ptrAIFile;
	string strAIFullFileName;

	TaskHandle  DOHandle  = 0;
	TaskHandle  CO1Handle = 0;
	TaskHandle  CO2Handle = 0;

	vector<uInt32> readArray;
	readArray.assign(ReadBufferSize,0);
	vector<uInt8> ulWriteArray;
	ulWriteArray.assign(DOArraySize,0);

	/*********************************************/
	// Digital Output Array for Clocking UEI
	/*********************************************/
	/*
	This array is 200 samples long. These samples are played at 5 microseconds/sample,
	therefore at 200,000 samples/sec. As a result I get 32 pulses and then a number of zeros.
	*/
	for(int q=0;q<DOArraySize;q++){
		if (q < 64 && q%2 == 0){
			ulWriteArray[q] = 0;
		}
		else if ( q < 64 && q%2 != 0 ){
			ulWriteArray[q] = 1; // 65535
		}
		else
			ulWriteArray[q] = 0;
	}

	/*********************************************/	
	// Initialize the Response file
	/*********************************************/
	char fileIndex[3];
	itoa(idx,fileIndex,5);
	cout << mValues.strRecFileDirPath << "\\" << mValues.strRecFileBaseName << fileIndex << ".dat" << endl;
	strAIFullFileName=mValues.strRecFileDirPath + "\\" + mValues.strRecFileBaseName + fileIndex + ".dat";
	ptrAIFile = fnOpenFileToWrite( strAIFullFileName );

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	DAQmxErrChk (DAQmxResetDevice("Dev1"));

	/*********************************************/
	// DAQmx Configure Counter 1 Code - Clock signal for UEI
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&CO1Handle));
	DAQmxErrChk (DAQmxCreateCOPulseChanTicks(CO1Handle,"Dev1/ctr0","","20MHzTimebase",DAQmx_Val_Low,0,100,100));//20MHzTimebase
	DAQmxErrChk (DAQmxCfgImplicitTiming(CO1Handle,DAQmx_Val_ContSamps,1000));

	/*********************************************/
	// DAQmx Configure Digital Out Code - Digital output for UEI
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&DOHandle));
	DAQmxErrChk (DAQmxCreateDOChan(DOHandle,"Dev1/port0/line0","",DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxCfgSampClkTiming(DOHandle,"/Dev1/Ctr0InternalOutput",200000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));//200000

	/*********************************************/
	// DAQmx Configure Start Trigger
	/*********************************************/
	DAQmxErrChk (DAQmxSetArmStartTrigType(CO1Handle,DAQmx_Val_DigEdge));
	DAQmxErrChk (DAQmxSetDigEdgeArmStartTrigSrc(CO1Handle,"/Dev1/PFI4"));
	DAQmxErrChk (DAQmxSetDigEdgeArmStartTrigEdge(CO1Handle,DAQmx_Val_Rising));

	/*********************************************/
	// DAQmx Configure Counter 2 Code - Timestamp	
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&CO2Handle));
	DAQmxErrChk (DAQmxCreateCICountEdgesChan(CO2Handle,"Dev1/ctr1","",DAQmx_Val_Rising,0,DAQmx_Val_CountUp));
	DAQmxErrChk (DAQmxCfgSampClkTiming(CO2Handle,"/Dev1/PFI4",1000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));
	DAQmxErrChk (DAQmxSetCICountEdgesTerm(CO2Handle,"","/Dev1/100kHzTimebase"));

	/*********************************************/
	// DAQmx Configure Start Trigger
	/*********************************************/
	DAQmxErrChk (DAQmxSetArmStartTrigType(CO2Handle,DAQmx_Val_DigEdge));
	DAQmxErrChk (DAQmxSetDigEdgeArmStartTrigSrc(CO2Handle,"/Dev1/PFI4"));
	DAQmxErrChk (DAQmxSetDigEdgeArmStartTrigEdge(CO2Handle,DAQmx_Val_Rising));

	/*********************************************/
	// DAQmx Digital Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteDigitalLines(DOHandle,DOArraySize,0,10.0,DAQmx_Val_GroupByScanNumber,&ulWriteArray[0],NULL,NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(DOHandle));
	DAQmxErrChk (DAQmxStartTask(CO1Handle));
	DAQmxErrChk (DAQmxStartTask(CO2Handle));

	cout << "Continuously Generating Clock Signals." << endl;
	cout << "Continuously Timestamping Data." << endl;
	cout << "Press any key to terminate trial." << endl;

	//UEILEDCtrl( mValues, CO2Handle );
	while ( !done && !kbhit() )
	{
		DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,numSampsPerChan,CIReadTimeout,&readArray[0],ReadBufferSize,&samplesPerChanRead,NULL));

		//********************************************
		// DAQmx Check available space code
		//********************************************
		if (samplesPerChanRead > 0){
			totalRead += samplesPerChanRead;
			cout << " Total Samples Read : \t" << totalRead << endl;
			ptrAIFile->write(reinterpret_cast<char*>(&readArray[0]),sizeof(uInt32)*samplesPerChanRead);
		}
	}
	getchar();


Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);

	if( DOHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(DOHandle);
		DAQmxClearTask(DOHandle);
	}
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

	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);

	/*********************************************/
	// DAQmx Reset Device
	/*********************************************/
	ptrAIFile->close();
	DAQmxErrChk (DAQmxResetDevice("Dev1"));

	return;

}