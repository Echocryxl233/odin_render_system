#pragma once

#ifndef GAMESETTING_H
#define GAMESETTING_H

//#include <fstream>
//#include <map>
//  #include <string>

#include "utility.h"

namespace GameCore {

namespace GameSetting {

using namespace std;

bool GetBoolValue(const string& name);
float GetFloatValue(const string& name);
int GetIntValue(const string& name);
string GetStringValue(const string& name);

void LoadConfig(const string& filename); 

};

};


#endif // !GAMESETTING_H
