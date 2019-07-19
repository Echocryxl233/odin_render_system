#pragma once
#include <windows.h>
#include <WindowsX.h>

#include "GameTimer.h"
#include "Util.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


namespace OdinRenderSystem {

class Application {
 public:
  Application(HINSTANCE h_instance);
  virtual ~Application();

	virtual bool Initialize();
	bool InitMainWindow();
	bool InitDirect3D();

  static Application* GetApp();
  HINSTANCE AppInst() const;
  HWND MainWnd() const;
  float AspectRatio() const;

  LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  int Run();

 protected:
  virtual void Draw(const GameTimer& gt ) = 0;
  virtual void Update(const GameTimer& gt ) = 0;

  virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

  virtual void CreateRtvAndDsvDescriptorHeaps();

 protected:
  void LogAdapters();
  void LogAdapterOutputs(IDXGIAdapter* adapter);
  void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

  void CreateCommand();
  void CreateSwapChain();
  void FlushCommandQueue();
 
  // ID3D12Resource* GetCurrentBackBuffer() const;

  ID3D12Resource* CurrentBackBuffer()const;
  D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
   
  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

 protected:
  virtual void OnResize();

 protected:
  static Application* app_instance_;
  HINSTANCE h_instance_;
  HWND h_window_;
  bool minimized_;
  bool maximized_;
  bool pause_;
  bool resizing_;

 protected:
   /*bool Get4xMsaaState() const;
   void Set4xMsaaState(bool value);*/

   

  GameTimer timer_;

  Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
  Microsoft::WRL::ComPtr<ID3D12Device> d3d_device_;
  Microsoft::WRL::ComPtr<ID3D12Fence> d3d_fence_;
   
  UINT64 current_fence_;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> d3d_command_allocator_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d_command_queue_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> d3d_command_list_;

  Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;

   

  int current_back_buffer_index_;
  const static int kSwapBufferCount = 2;
  Microsoft::WRL::ComPtr<ID3D12Resource> swap_chain_buffers_[kSwapBufferCount];
  Microsoft::WRL::ComPtr<ID3D12Resource> depth_stencil_buffer;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;  //  render target
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap_;  //  depth/stencil 

  D3D12_VIEWPORT screen_viewport_; 
  D3D12_RECT scissor_rect_;
   
  UINT rtv_descriptor_size = 0;
  UINT dsv_descriptor_size = 0;
	UINT cbv_srv_uav_descriptor_size = 0;

  std::wstring main_wnd_caption_ = L"Odin";

  long client_width_ = 800;
  long client_height_ = 600;

  DXGI_FORMAT back_buffer_format_ = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
  DXGI_FORMAT depth_stencil_format_ = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;

  bool use_4x_msaa_state_ = false;
  UINT use_4x_msaa_quanlity_ = 0;

};

}

