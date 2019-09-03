#include "game_core.h"

namespace GameCore {

RenderSystem* render_system;

extern long client_width_ = 1080;
extern long client_height_ = 720;
HWND  window;


int UpdateGame(RenderSystem& rs);
bool CreateMainWindow();


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch( message )
  {
    case WM_SIZE:
      Graphics::Core.OnResize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
      break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      render_system->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      render_system->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    case WM_MOUSEMOVE:
      render_system->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

  default:
    return DefWindowProc( hWnd, message, wParam, lParam );
  }

  return 0;
}

void Initialize(RenderSystem& rs) {

  CreateMainWindow();
  Graphics::Core.Initialize(window);

  rs.Initialize();

  ShowWindow(window, SW_SHOWDEFAULT);
}

void Run(RenderSystem& rs) {
  Initialize(rs);
  render_system = &rs;
  UpdateGame(rs);
}


int UpdateGame(RenderSystem& rs) {
  MSG msg = { 0 };
  //  timer_.Reset();
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

    }
    else {
      //timer_.Tick();

      //if (!pause_) {
      //  //Update(timer_);
      //  //Draw(timer_);
      //}
      //else {
      //  Sleep(100);
      //}
      //  Graphics::Core.PreparePresent();
      rs.Update();
      rs.RenderScene();
      Graphics::Core.Present();
    }
  }

  return (int)msg.wParam;
}


bool CreateMainWindow() {
  HINSTANCE instance = GetModuleHandle(0);

  WNDCLASS wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = instance;
  wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = L"MainWnd";

  if (!RegisterClass(&wc)) {
    MessageBox(0, L"RegisterClass Failed.", 0, 0);
    return false;
  }

  RECT windowRect = { 0, 0, client_width_, client_height_ };
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
  int width = windowRect.right - windowRect.left;
  int height = windowRect.bottom - windowRect.top;
  
  std::wstring main_wnd_caption_ = L"Odin";

  window = CreateWindow(L"MainWnd", main_wnd_caption_.c_str(),
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);

  if (!window) {
    MessageBox(0, L"CreateWindow Failed.", 0, 0);
    return false;
  }

  //  ShowWindow(window, SW_SHOW);

  
  //  UpdateWindow(window);


  return true;
}



};



