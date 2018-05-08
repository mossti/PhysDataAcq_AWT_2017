#ifndef MENU_H
#define MENU_H

#include "MenuReturnValues.h"

class Menu
{
public:

	Menu();
	~Menu(){};
	
	void fnReadUserInput();			//displays menu, takes input parameters, and stores user input in mValues
	MenuReturnValues getMValues(){return mValues;}	//returns mValues

protected:
	
	MenuReturnValues mValues;
	void fnMenuSettingsOption(MenuReturnValues);
	
};

#endif