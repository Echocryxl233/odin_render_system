#include "game_setting.h"



namespace GameCore {

namespace GameSetting {

float LightArea = 1.0f;
float LightHeight = 3.0f;
int  UseDeferred = 1;
float ObjectArea = 1.0f;
int ObjectCount = 1;

void LoadConfig(const string& filename) {

  ifstream fin(filename);
  if (!fin)
  {
    wstring box_msg = DebugUtility::AnsiToWString(filename);
    box_msg += L" not found.";
    MessageBox(0, box_msg.c_str(), 0, 0);
    return;
  }

  string ignore;

  fin >> ignore >> LightArea;
  fin >> ignore >> LightHeight;
  fin >> ignore >> ObjectArea;
  fin >> ignore >> ObjectCount;

  fin >> ignore >> UseDeferred;
}

};

};