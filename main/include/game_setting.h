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

extern float LightArea;
extern float ObjectArea;
extern float LightHeight;
extern int UseDeferred;
extern int ObjectCount;


void LoadConfig(const string& filename); 

};

};


#endif // !GAMESETTING_H
