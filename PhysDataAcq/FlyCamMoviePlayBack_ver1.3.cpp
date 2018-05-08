/* 
------------------------------------------------------------------------
This file is designed to produce Analog Output (AO). The AO data is read from a previosuly created binary file with 'double' precision.
Functions:
ifstream* OpenFileToRead(string)
void CloseFile()
void ReadDataFromFile(double*, int, int, ifstream*)
void GenerateSinWave(double*, int, int, int)
void GenerateSquareWave(double*, int, int, int)

Classes:
class CAnalogOutEvent

This program has a UEIBoardInitialization class instantiated
------------------------------------------------------------------------
*/

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <windows.h>
#include "UeiDaq.h"
#include <UeiException.h>
#include <UeiTiming.h>
#include <UeiWriter.h>
#include <Menu.h>

using namespace UeiDaq;
using namespace std;

CUeiSession mySs0;						// Open the 1st UEI AO card
CUeiSession mySs1;						// Open the 2nd UEI AO card
CUeiAnalogScaledWriter *writer0;		// Pointer to session is used to store the DAQ device settings and control its operation, device 0
CUeiAnalogScaledWriter *writer1;		// Pointer to session is used to store the DAQ device settings and control its operation, device 1
CUeiDataStream *ptrDataStream0;			// Pointer to class that represents the data streaming out of the device 0
CUeiDataStream *ptrDataStream1;			// Pointer to class that represents the data streaming out of the device 1

int iNumOfChannels = 32;
int iNumSamples = 1025;					// Set the number of samples to load per channel
const double dDataFileSize = 32800 ;	//iNumOfChannels*iNumSamples;
double dataBoard0[dDataFileSize];		// This vector holds the data for all the channels  = 32 channels x iNumSamples
double dataBoard1[32800];				// This vector holds the data for all the channels  = 32 channels x iNumSamples
double dSampleRate = 1000;				// Set the aggregate output sampling rate
const long typeStimFile = sizeof(double); // The INPUT data is in binary, little-endian, double percision

ifstream* fnOpenFileToRead(string strStimulusFilename) {
	ifstream *ptrStimFile = new ifstream(); 
	ptrStimFile->open(strStimulusFilename.c_str(), ios::in|ios::binary|ios::ate);	// input, binary, move file pointer to end
	
	if (!ptrStimFile->good()) {
		cerr << "File could not be opened." << endl;
		exit(1);
	}
	else {
		cerr << "File opened successfully " << endl;
	}

	//Calculate the number of points in the INPUT file
	int slSizeFrameFile = ptrStimFile->tellg();
	int slFrameNumElem = slSizeFrameFile/typeStimFile;
	cout << "Stimulus File Size (bytes): " << slSizeFrameFile << endl;
	cout << "# of Elements in Stimulus File: " << slFrameNumElem << endl;
	ptrStimFile->seekg(0, ios::beg);
	
	return ptrStimFile;
}	
void fnCloseFile(ifstream* ptrStimFile)
{
	ptrStimFile->close();

	if ( !ptrStimFile->good() ) {
		cerr << "File did not close." << endl;
		exit(1);
	}
	else {
		cerr << "File closed successfully " << endl;
	}
}	
void ReadDataFromFile(double* pBuffer, int nbChannels, int nbSamplePerChannel, ifstream* ptrStimFile)
{
	//Initialize the arrays for the INPUT buffer
	double outputBuffer;

	//Read the data from the INPUT file into the INPUT buffer
	for(int i=0; i<nbSamplePerChannel; i++)
	{	
		for(int j=0; j<nbChannels; j++)
		{
			ptrStimFile->read( reinterpret_cast<char *>( &outputBuffer ), typeStimFile);
			if( ptrStimFile->bad() ) 
			{
				cerr << "Error reading data" << endl;
				exit( 0 );
			}	
			pBuffer[i*nbChannels+j] = outputBuffer;
			//cout << outputBuffer << endl;
		}
	}
}
void GenerateSinWave(double* pBuffer, int nbChannels, int nbSamplePerChannel, int iteration)
{
   int amplitude = (iteration % 10 + 1);

   for(int i=0; i<nbSamplePerChannel; i++)
   {
      for(int j=0; j<nbChannels; j++)
      {
         pBuffer[i*nbChannels+j] = amplitude * sin(2*3.1415*(1)*i/nbSamplePerChannel);
		 //pBuffer[i*nbChannels+j] = amplitude * sin(2*3.1415*(j+1)*i/nbSamplePerChannel);
      }
   }
}
void GenerateSquareWave(double* pBuffer, int nbChannels, int nbSamplePerChannel, int iteration)
{
	int amplitude = 1;

	for(int j=0; j<nbSamplePerChannel; j++)
	{
		for(int k =0; k<nbChannels; k++)
		{
			if(j<nbSamplePerChannel/2)
                pBuffer[j*nbChannels+k] = amplitude;
			else
				pBuffer[j*nbChannels+k] = -amplitude;
		}
	}
}

class CAnalogOutEvent : public IUeiEventListener
{
public:
	CUeiSession *ptrSession;
	CUeiAnalogScaledWriter *ptrWriter;
	ifstream *ptrStimFile;
	double *ptrData;

private:
   void OnEvent(tUeiEvent event, void *param)
   {
      static int count = 0;
      if(event == UeiEventFrameDone)
      {
         try
         {
			count++ ;
            cout << "event # " << setw(4) << count << " received" << endl;

            // If regeneration is turned off we can refresh the buffer to generate
            // new data continuously, this call blocks until there is enough
            // room in the buffer to write the required number of samples
            ptrWriter->WriteMultipleScansAsync(iNumSamples, ptrData);

            // Create a new set of data to be generated next time we get an event
            ReadDataFromFile(ptrData, ptrSession->GetNumberOfChannels(), iNumSamples, ptrStimFile);
			//GenerateSinWave(data, ptrSession->GetNumberOfChannels(), iNumSamples, 0);         
         }
         catch(CUeiException e)
         {
            cout << "Error: " << e.GetErrorMessage() << endl;
         }
      }
      else if(event == UeiEventError)
      {
         tUeiError error = (tUeiError)(unsigned long)param;
         cout << "Error: " << CUeiException::TranslateError(error) << endl;
      }
   }
};

int main(int argc, char* argv[])
{
	try
	{
		//Create Menu
		Menu menu;
		menu.fnReadUserInput
		MenuReturnValues mRetVal;
		mRetVal=menu.getMValues();
		//menu.getMValues


		string InputDataDirectoryPath("C:\\LED_Movie");
		string strStimulusFilename0("stimulusFile0.dat");		// Open the stimulus file
		string strStimulusFilename1("stimulusFile1.dat");		// Open the stimulus file
				
		//Call function to open data file
		ifstream *ptrStimFile0 = fnOpenFileToRead(strStimulusFilename0);
		ifstream *ptrStimFile1 = fnOpenFileToRead(strStimulusFilename1);
		
		//ofstream parValFile;
		//parValFile<<menu.getMValues();
		//parValFile.close();

		// Create analog output channels on a powerdaq board
		// From now on the session is AO only
		mySs0.CreateAOChannel("pwrdaq://Dev0/ao0:31", -10.0,mRetVal.channelCount );
		mySs1.CreateAOChannel("pwrdaq://Dev1/ao0:31", -10.0, 10.0);

		// Configure the session to generate 1000 scans clocked by internal scan clock
		mySs0.ConfigureTimingForBufferedIO(iNumSamples, UeiTimingClockSourceExternal, dSampleRate, UeiDigitalEdgeRising, UeiTimingDurationContinuous);
		mySs1.ConfigureTimingForBufferedIO(iNumSamples, UeiTimingClockSourceExternal, dSampleRate, UeiDigitalEdgeRising, UeiTimingDurationContinuous);

		// Use a large number of frames for the Advanced Circular Buffer (ACB)
		ptrDataStream0 = mySs0.GetDataStream();
		ptrDataStream0 -> SetNumberOfFrames(4);		// Configure the number of buffers for ACB
		ptrDataStream1 = mySs1.GetDataStream();
		ptrDataStream1 -> SetNumberOfFrames(4);		// Configure the number of buffers for ACB

		// Create a reader object to read data from file synchronously.
		writer0 = new CUeiAnalogScaledWriter(mySs0.GetDataStream());
		writer1 = new CUeiAnalogScaledWriter(mySs1.GetDataStream());

		// Configure an asynchronous event handler that will be called
		// by the writer object each time the required number of scans has been generated
		CAnalogOutEvent eventListener0;
		writer0->AddEventListener(&eventListener0);
		eventListener0.ptrSession = &mySs0;
		eventListener0.ptrWriter = writer0;
		eventListener0.ptrStimFile = ptrStimFile0;	//The eventListener0 object needs the ifstream pointer
		eventListener0.ptrData = dataBoard0;		//The eventListener0 object needs the address to dataBoard0

		CAnalogOutEvent eventListener1;
		writer1->AddEventListener(&eventListener1);
		eventListener1.ptrSession = &mySs1;
		eventListener1.ptrWriter = writer1;
		eventListener1.ptrStimFile = ptrStimFile1;	//The eventListener1 object needs the ifstream pointer
		eventListener1.ptrData = dataBoard1;		//The eventListener0 object needs the address to dataBoard1


		// Start the generation
		mySs0.Start();
		mySs1.Start();

		// Fill the buffers for the two cards
		//GenerateSinWave(dataBoard1, mySs0.GetNumberOfChannels(), iNumSamples, 0);
		//GenerateSinWave(dataBoard1, mySs1.GetNumberOfChannels(), iNumSamples, 0);
		ReadDataFromFile(dataBoard0, mySs0.GetNumberOfChannels(), iNumSamples, ptrStimFile0);
		ReadDataFromFile(dataBoard1, mySs1.GetNumberOfChannels(), iNumSamples, ptrStimFile1);
		writer0->WriteMultipleScansAsync(iNumSamples, dataBoard0);
		writer1->WriteMultipleScansAsync(iNumSamples, dataBoard1);

		// Data transfer is done asynchronously
		// Press the Enter key to end the generation
		cout << "Press 'Enter' to stop the generation." << endl;
		getchar();

		mySs1.Stop();
		mySs0.Stop();
		
		fnCloseFile(ptrStimFile0);
		delete ptrStimFile0;
		
		fnCloseFile(ptrStimFile1);
		delete ptrStimFile1;
	}
	catch(CUeiException e)
	{
		cout << "Error: " << e.GetErrorMessage() << endl;
	}

	while (cin.get() != 'q') {
		cout << "Your task is complete. Press q key to end!" << endl;
	}
	return 0;
}