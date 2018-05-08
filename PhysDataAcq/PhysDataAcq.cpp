/*
Current Version 2.02
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <conio.h>
#include <windows.h>
#include "MenuReturnValues.h"
#include "Menu.h"
#include "AO_TimeStamp_RTInterp.h"

using namespace std;

int main(void)
{
	int16 iIndex = -1;
	Menu DAQmenu;
	MenuReturnValues mRetVal;

	while (true)
	{
		iIndex ++;
		DAQmenu.fnReadUserInput();
		mRetVal=DAQmenu.getMValues();

		/*********************************************/	
		// Execute Appropriate DAQ Sub-routine
		/*********************************************/
		if (mRetVal.iRecordType == 4){
			cout << "Continuous Analog Output, Timestamping, and RT Interpolation." << endl;		
			AO_TimeStamp_RTInterp( mRetVal, iIndex ); //Configuraiton to use for photoreceptor recordings
		}

		cout << " ----------> End of trial <---------- " << endl;
		cout << endl;
		cout << endl;
	}
	return 0;
}