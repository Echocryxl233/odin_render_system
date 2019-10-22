#include "game_core.h"
#include "camera.h"
#include "camera_controller.h"
#include "depth_of_field.h"
#include "game_timer.h"


namespace GameCore {

RenderSystem* render_system;

long client_width_ = 800;
long client_height_ = 600;
HWND  window;

POINT mLastMousePos;
std::auto_ptr<CameraController> main_camera_controller_;



int UpdateGame(RenderSystem& rs);
bool CreateMainWindow();


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch( message )
  {
    case WM_SIZE: {
        UINT width = (UINT)(UINT64)lParam & 0xFFFF;
        UINT height = (UINT)(UINT64)lParam >> 16;
        Graphics::Core.OnResize(width, height);
        PostProcess::DoF.OnResize(width, height);
          render_system->OnResize(width, height);
      }

      break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      //  render_system->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  

      return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      //  render_system->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    case WM_MOUSEMOVE:
      //  render_system->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    {
      WPARAM btnState = wParam;
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);

      float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
      float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

      if ((btnState & MK_LBUTTON) != 0) {
        main_camera_controller_->Pitch(dy);
        main_camera_controller_->RotateY(dx);
      }

      mLastMousePos.x = x;
      mLastMousePos.y = y;
    
      return 0;
    }
    

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
  InitMainCamera();
  main_camera_controller_.reset(new CameraController(MainCamera, 
      XMFLOAT3(0.0f, 1.0f, 0.0f)));
  PostProcess::DoF.Initialize();  

  rs.Initialize();

  ShowWindow(window, SW_SHOWDEFAULT);
}

void Run(RenderSystem& rs) {
  render_system = &rs;
  Initialize(rs);
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
      MainTimer.Tick();

      main_camera_controller_->Update(MainTimer);

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



