#include <iostream>
#include <memory>
#include "game/game_core.h"
#include "game/camera.h"
#include "game/camera_controller.h"
#include "game/game_timer.h"
#include "game/game_setting.h"
#include "graphics/graphics_common.h"
#include "graphics/graphics_utility.h"
#include "render_queue.h"
#include "skybox.h"
#include "GI/shadow.h"
#include "GI/ssao.h"
#include "GI/ssr.h"
#include "postprocess/bloom.h"
#include "postprocess/depth_of_field.h"



namespace GameCore {

//  RenderSystem* render_system;

long client_width_ = 800;
long client_height_ = 600;
HWND  window;

POINT last_mouse_pos_;
std::unique_ptr<CameraController> main_camera_controller_;
RenderSystem* render_system;


int UpdateGame();
bool CreateMainWindow();
void CalculateFrameStats();


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch( message )
  {
    case WM_SIZE: {
        UINT width = (UINT)(UINT64)lParam & 0xFFFF;
        UINT height = (UINT)(UINT64)lParam >> 16;

        if (width == 0 || height == 0)
          break;
      
        Graphics::Core.OnResize(width, height);
        Graphics::ResizeBuffers(width, height);
        GI::AO::MainSsao.Resize(width, height);
        GI::Specular::MainSSR.OnResize(width, height);
        //PostProcess::DoF.OnResize(width, height);
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

      float dx = XMConvertToRadians(0.25f * static_cast<float>(x - last_mouse_pos_.x));
      float dy = XMConvertToRadians(0.25f * static_cast<float>(y - last_mouse_pos_.y));

      if ((btnState & MK_LBUTTON) != 0) {
        main_camera_controller_->Pitch(dy);
        main_camera_controller_->RotateY(dx);
      }
      else if ((btnState & MK_RBUTTON) != 0) {
        Graphics::RotateDirectLight(dx, dy);
      }

      last_mouse_pos_.x = x;
      last_mouse_pos_.y = y;
    
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



void Initialize() {
//  cout << "sizeof(PassConstant) = " << sizeof(PassConstant) << endl;
  MainTimer.Reset();
  GameSetting::LoadConfig("config.txt");
  CreateMainWindow();
  Graphics::Core.Initialize(window);
  Graphics::InitCommonBuffers();
  Graphics::InitRenderQueue();
  Graphics::InitializeLights();
  Graphics::SkyBox::Initialize();
  Graphics::Utility::InitBlur();

  GI::AO::MainSsao.Initialize(Graphics::Core.Width(), Graphics::Core.Height());
  GI::Specular::MainSSR.Initialize(Graphics::Core.Width(), Graphics::Core.Height());
  GI::Shadow::MainShadow.Initialize(Graphics::Core.Width(), Graphics::Core.Height());

  
  InitMainCamera();
  main_camera_controller_.reset(new CameraController(MainCamera, 
      XMFLOAT3(0.0f, 1.0f, 0.0f)));
  PostProcess::DoF.Initialize();  
  PostProcess::BloomEffect->Initialize();


  cout << GameSetting::GetStringValue("SkyTexture") << endl;

  //  rs.Initialize();
  render_system = new RenderSystem();
  render_system->Initialize();

  ShowWindow(window, SW_SHOWDEFAULT);
}

void Run() {
  std::cout << DebugUtility::FormatT("asd%0fghj%1", 3, 456) << std::endl;;
  Initialize();
  UpdateGame();
  delete render_system;
}

void OnKeyboardInput() {
  if (GetAsyncKeyState('R') & 0x8000)
    Graphics::ResetDirectLightRotation();

  if (GetAsyncKeyState('1') & 0x8000) {
    GI::Shadow::MainShadow.SetEnabled(!GI::Shadow::MainShadow.Enabled());
  }

  if (GetAsyncKeyState('5') & 0x8000) {
    PostProcess::BloomEffect->SetEnabled(!PostProcess::BloomEffect->Enabled());
  }

  if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
    render_system->PrevOptionalSystem();
  }

  if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
    render_system->NextOptionalSystem();
  }
    
}

void Update() {
  OnKeyboardInput();
  main_camera_controller_->Update(MainTimer);
  GI::Shadow::MainShadow.Update();
  Graphics::UpdateConstants();
  Graphics::MainQueue.Update(MainTimer);
}


int UpdateGame() {
  MSG msg = { 0 };
  //  timer_.Reset();
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      MainTimer.Tick();
      Update();

      render_system->Render();
      Graphics::Core.Present();
      CalculateFrameStats();
      //  std::cout << "total time: " << MainTimer.TotalTime() << endl;
    }
  }

  return (int)msg.wParam;
}


void CalculateFrameStats()
{
  // Code computes the average frames per second, and also the 
  // average time it takes to render one frame.  These stats 
  // are appended to the window caption bar.

  static int frame_cnt = 0;
  static float time_elapsed = 0.0f;

  frame_cnt++;

  // Compute averages over one second period.
  if ((GameCore::MainTimer.TotalTime() - time_elapsed) >= 1.0f)
  {
    float fps = (float)frame_cnt; // fps = frameCnt / 1
    float mspf = 1000.0f / fps;

    wstring fpsStr = to_wstring(fps);
    wstring mspfStr = to_wstring(mspf);

    wstring windowText = // mMainWndCaption +
      L"    fps: " + fpsStr +
      L"   mspf: " + mspfStr;

    SetWindowText(window, windowText.c_str());

    // Reset for next average.
    frame_cnt = 0;
    time_elapsed += 1.0f;
  }
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



