/*********************************************************************
*
* ANSI C Example program:
*    ContWriteDigPort-ExtClk.c
*
* Example Category:
*    DO
*
* Description:
*    This example demonstrates how to output a continuous digital
*    pattern using an external clock.
*
* Instructions for Running:
*    1. Select the Physical Channel to correspond to where your
*       signal is output on the DAQ device.
*    2. Select the Clock Source for the generation.
*    3. Specify the Rate of the output digital pattern.
*    4. Enter the digital pattern data.
*
* Steps:
*    1. Create a task.
*    2. Create an Digital Output Channel.
*    3. Call the DAQmxCfgSampClkTiming function which sets the sample
*       clock rate. Additionally, set the sample mode to continuous.
*    4. Write the data to the output buffer.
*    5. Call the Start function to start the task.
*    6. Wait until the user presses the Stop button.
*    7. Call the Clear Task function to clear the Task.
*    8. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal output terminal matches the Physical
*    Channel I/O Control. Also, make sure your external clock
*    terminal matches the Clock Source Control. For further
*    connection information, refer to your hardware reference manual.
*
*********************************************************************/


#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <NIDAQmx.h>


#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

using namespace std;
int main(void)
{	
	TaskHandle  COHandle=0;
	TaskHandle  DOHandle=0;
	char        errBuff[2048]={'\0'};
	int32       error=0;
	int32       read;
	int32		DOArraySize = 200;
	
	/*********************************************/
	// Digital Output Array for Clocking UEI
	/*********************************************/
	vector<uInt8>	ulWriteArray;
	ulWriteArray.assign(DOArraySize,0);
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
	// DAQmx Configure Counter 1 Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&COHandle));
	DAQmxErrChk (DAQmxCreateCOPulseChanTicks(COHandle,"Dev1/ctr0","","20MHzTimebase",DAQmx_Val_Low,0,50,50));
	DAQmxErrChk (DAQmxCfgImplicitTiming(COHandle,DAQmx_Val_ContSamps,1000));

	/*********************************************/
	// DAQmx Configure Digital Out Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&DOHandle));
	DAQmxErrChk (DAQmxCreateDOChan(DOHandle,"Dev1/port0/line0","",DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxCfgSampClkTiming(DOHandle,"/Dev1/Ctr0InternalOutput",200000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1000));

    /*********************************************/
    // DAQmx Allow Regeneration
    /*********************************************/
    DAQmxErrChk (DAQmxSetWriteRegenMode(DOHandle,DAQmx_Val_AllowRegen));

	/*********************************************/
	// DAQmx Digital Write Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteDigitalLines(DOHandle,DOArraySize,0,10.0,DAQmx_Val_GroupByScanNumber,&ulWriteArray[0],NULL,NULL));

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(DOHandle));
	DAQmxErrChk (DAQmxStartTask(COHandle));


	cout << "Continuously Generating Clock Signals. Press any key to interrupt" << endl ;
	//while( !done && !kbhit()  ) {
	//	/*********************************************/
	//	// DAQmx Read Code
	//	/*********************************************/
	//	DAQmxErrChk (DAQmxReadCounterU32(CO2Handle,1000,10.0,data,1000,&read,NULL));

	//	printf("\rAcquired %d samples",read);
	//	fflush(stdout);
	//	}
	getchar();

Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( DOHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(COHandle);
		DAQmxStopTask(DOHandle);
		DAQmxClearTask(COHandle);
		DAQmxClearTask(DOHandle);
	}
	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);
	printf("End of program, press Enter key to quit\n");
	getchar();
	return 0;
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error=0;
	char    errBuff[2048]={'\0'};

	// Check to see if an error stopped the task.
	DAQmxErrChk (status);

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n",errBuff);
	}
	return 0;
}
