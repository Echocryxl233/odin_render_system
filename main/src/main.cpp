#include <windows.h>
#include "debug_log.h"
#include "game_core.h"

int main() {
  #if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  #endif
  try {
    RenderSystem* system = new RenderSystem();
    GameCore::Run(*system);
    delete system;

    return 0;
  }
  catch (DxException& e) {
    MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK); 
    return 0;
  }
  return 0;
}
