#include <string>
#include <ostream>
#include "MenuReturnValues.h"

using namespace std;

MenuReturnValues::MenuReturnValues()
{
	//Parameters for Photoreceptor Experiments
	//iRecordType			= 2;
	//iNumReps			= 10;
	//strRecFileDirPath	= string("C:\\Rob\\Experiment");
	////strRecFileDirPath	= string("C:\\Documents and Settings\\LED\\My Documents\\Visual Studio Projects\\PhysDataAcq\\PhysDataAcq\\Release");
	//strRecFileBaseName	= string("test");
	//dRecSampRate		= 1000;
	//dRecMinVoltVal		= -5.0;
	//dRecMaxVoltVal		= 5.0;
	//iNumAIChans			= 1;
	//iNumCIChans			= 0;
	//dRecSampRate		= 1000;
	//dAIRecTimeOut		= 10.0;
	//dCIRecTimeOut		= 20.0;

	//strStimFileDirPath	= string("C:\\zhou\\PhysDataAcq_v2.02");
	//strStimFileDirPath	= string("C:\\Documents and Settings\\LED\\My Documents\\Visual Studio Projects\\PhysDataAcq\\PhysDataAcq\\Release");
	//strStimFileName0	= string("FindCell");
	//dStimSampRate		= 1000;
	//dStimMinVoltVal		= -5.0;
	//dStimMaxVoltVal		= 5.0;
	//iNumNIAOChans		= 1;
	//iNumUEIAOChans		= 0;
	//dStimTimeOut		= -1.0;
	//bStimAutoStart		= 0;
	//iStimBufferSize		= 2048;


	//Parameters for H1 Extracellular Experiments - USING THE LED ARRAY
	//iRecordType			= 2;
	//iNumReps			= 1;
	//strRecFileDirPath	= string("C:\\Data\\Physiology\\");
	//strRecFileBaseName	= string("LED_test");
	//dRecSampRate		= 1000;
	//dRecMinVoltVal		= 0;//-10
	//dRecMaxVoltVal		= 10;
	//iNumAIChans			= 0;
	//iNumCIChans			= 1;
	//dAIRecTimeOut		= 10.0;
	//dCIRecTimeOut		= 180.0;
	//slDOArraySize		= 100;
	//slNumSampsPerChan	= 1000;
	//ulReadBufferSize	= 10000;

	//strStimFileDirPath	= string("C:\\Documents and Settings\\LED\\My Documents\\Visual Studio Projects\\AnalogOutBufferedAsync\\Debug");
	//strStimFileName0	= string("stimulusFile0");
	//strStimFileName1	= string("stimulusFile1");
	//dStimSampRate		= 1000;
	//dStimMinVoltVal		= -10.0;
	//dStimMaxVoltVal		= 10.0;
	//iNumNIAOChans		= 1;
	//iNumUEIAOChans		= 32;
	//dStimTimeOut		= 0.0;
	//bStimAutoStart		= 0;

	//Parameters for H1 Extracellular Experiments - USING THE 608 OSCILLOSCOPE
	iRecordType			= 4;
	strRecFileDirPath	= string("C:\\Data\\Physiology\\");
	strRecFileBaseName	= string("LED_Test_");
	//strRecFileBaseName= string("HorzMovSinWav_Rep12_Vel051020_Len5");//test20080508
	dRecSampRate		= 1.25e6;
	dRecMinVoltVal		= -5;
	dRecMaxVoltVal		= 5;
	iNumAIChans			= 1;
	iNumCIChans			= 2;
	dAIRecTimeOut		= 10.0;
	dCIRecTimeOut		= 180.0;
	slDOArraySize		= 500;//500
	slNumSampsPerChan	= 1000;//100;
	ulReadBufferSize	= 2000;//5000;

	strStimFileDirPath	= string("C:\\Program Files\\MATLAB\\R2006a\\work\\Physiology\\Tek608\\Stimulus_Files\\");
	strStimFileName0	= string("Stimulus");//Stimulus608Scope; SineVertMotion; Nyquist
	strStimFileName1	= string("XCoords"); //NY_XCoords
	strStimFileName2	= string("YCoords"); //NY_YCoords
	dStimSampRate		= 1250000;
	dStimMinVoltVal		= -5.0;
	dStimMaxVoltVal		= 5.0;
	iNumRepeats			= 1;
	iNumNIAOChans		= 3;
	iNumUEIAOChans		= 0;
	dStimTimeOut		= 10.0;
	bStimAutoStart		= 0;
	iNRasterPoints		= 833;

}

std::ostream & operator <<(std::ostream & out , MenuReturnValues & mValues)
{
	out << mValues.iRecordType			<< endl;

	out << mValues.strRecFileDirPath	<< endl;
	out << mValues.strRecFileBaseName	<< endl;
	out << mValues.dRecSampRate			<< endl;
	out << mValues.dRecMinVoltVal		<< endl;
	out << mValues.dRecMaxVoltVal		<< endl;
	out << mValues.iNumAIChans			<< endl;
	out << mValues.iNumCIChans			<< endl;
	out << mValues.dAIRecTimeOut		<< endl;
	out << mValues.dCIRecTimeOut		<< endl;
	
	out << mValues.strStimFileDirPath	<< endl;
	out << mValues.strStimFileName0		<< endl;
	out << mValues.strStimFileName1		<< endl;
	out << mValues.strStimFileName2		<< endl;
	out << mValues.dStimSampRate		<< endl;
	out << mValues.dStimMinVoltVal		<< endl;
	out << mValues.dStimMaxVoltVal		<< endl;
	out << mValues.iNumRepeats			<< endl;
	out << mValues.iNumNIAOChans		<< endl;
	out << mValues.iNumUEIAOChans		<< endl;
	out << mValues.dStimTimeOut			<< endl;
	out << mValues.bStimAutoStart		<< endl;
	out << mValues.iNRasterPoints		<< endl;

	return out;

};
