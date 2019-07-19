//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Application.h"


using Microsoft::WRL::ComPtr;
using namespace std;
//  using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
    return OdinRenderSystem::Application::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

namespace OdinRenderSystem {

Application* Application::app_instance_ = nullptr;
Application* Application::GetApp()
{
    return app_instance_;
}

Application::Application(HINSTANCE hInstance)
:	h_instance_(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(app_instance_ == nullptr);
    app_instance_ = this;
}

Application::~Application()
{
	if(d3d_device_ != nullptr)
		FlushCommandQueue();
}

HINSTANCE Application::AppInst()const
{
	return h_instance_;
}

HWND Application::MainWnd()const
{
	return h_window_;
}

float Application::AspectRatio()const
{
	return static_cast<float>(client_width_) / client_height_;
}

//bool Application::Get4xMsaaState()const
//{
//    return use_4x_msaa_state_;
//}
//
//void Application::Set4xMsaaState(bool value)
//{
//    if(use_4x_msaa_state_ != value)
//    {
//        use_4x_msaa_state_ = value;
//
//        // Recreate the swapchain and buffers with new multisample settings.
//        CreateSwapChain();
//        OnResize();
//    }
//}

int Application::Run() {
  MSG msg = {0};
  timer_.Reset();
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
      
		}
    else {
      timer_.Tick();

      if (!pause_) {
        Update(timer_);
        Draw(timer_);
      }
      else {
        Sleep(100);
      }
      
    }
  }

  return (int)msg.wParam;
}

bool Application::Initialize()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

    // Do the initial resize code.
    OnResize();

	return true;
}
 
void Application::CreateRtvAndDsvDescriptorHeaps() {
  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.NumDescriptors = kSwapBufferCount;
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtv_heap_desc.NodeMask = 0;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&rtv_heap_desc, 
    IID_PPV_ARGS(rtv_heap_.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
  dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsv_heap_desc.NumDescriptors = 1;
  dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  dsv_heap_desc.NodeMask = 0;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&dsv_heap_desc, 
    IID_PPV_ARGS(dsv_heap_.GetAddressOf())));
}


void Application::OnResize() {
  assert(d3d_device_);
  assert(swap_chain_);
  assert(d3d_command_allocator_);

  FlushCommandQueue();

  ThrowIfFailed(d3d_command_list_->Reset(d3d_command_allocator_.Get(), nullptr));

  /*for (auto& buffer : swap_chain_buffers_) {
    buffer.Reset();
  }*/
  for (int i = 0; i < kSwapBufferCount; ++i)
		swap_chain_buffers_[i].Reset();
  depth_stencil_buffer.Reset();

  swap_chain_->ResizeBuffers(kSwapBufferCount, client_width_, client_height_,
    back_buffer_format_, DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

  current_back_buffer_index_ = 0;

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());

  for (UINT i = 0; i < kSwapBufferCount; i++)
	{
		ThrowIfFailed(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&swap_chain_buffers_[i])));
		d3d_device_->CreateRenderTargetView(swap_chain_buffers_[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, rtv_descriptor_size);
	}

  D3D12_RESOURCE_DESC depth_stencil_desc;
  depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Alignment = 0;
  depth_stencil_desc.Width = client_width_;
  depth_stencil_desc.Height = client_height_;
  depth_stencil_desc.DepthOrArraySize = 1;
  depth_stencil_desc.MipLevels = 1;
  depth_stencil_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
  depth_stencil_desc.SampleDesc.Quality = use_4x_msaa_state_ ? use_4x_msaa_quanlity_ - 1 : 0;
  depth_stencil_desc.SampleDesc.Count = use_4x_msaa_state_ ? 4 : 1;
  depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depth_stencil_desc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear;
  optClear.Format = depth_stencil_format_;
  optClear.DepthStencil.Depth = 1.0f;
  optClear.DepthStencil.Stencil = 0;

  ThrowIfFailed(d3d_device_->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
        &depth_stencil_desc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(depth_stencil_buffer.GetAddressOf())));
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Format = depth_stencil_format_;
	dsv_desc.Texture2D.MipSlice = 0;

  d3d_device_->CreateDepthStencilView(depth_stencil_buffer.Get(), &dsv_desc, DepthStencilView());

  // Transition the resource from its initial state to be used as a depth buffer.
	d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_buffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

  // Execute the resize commands.
  ThrowIfFailed(d3d_command_list_->Close());
  ID3D12CommandList* cmdsLists[] = { d3d_command_list_.Get() };
  d3d_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();

  // Update the viewport transform to cover the client area.
	screen_viewport_.TopLeftX = 0;
	screen_viewport_.TopLeftY = 0;
	screen_viewport_.Width    = static_cast<float>(client_width_);
	screen_viewport_.Height   = static_cast<float>(client_height_);
	screen_viewport_.MinDepth = 0.0f;
	screen_viewport_.MaxDepth = 1.0f;

  scissor_rect_ = { 0, 0, client_width_, client_height_ };

}

 
LRESULT Application::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			pause_ = true;
			timer_.Stop();
		}
		else
		{
			pause_ = false;
			timer_.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		client_width_  = LOWORD(lParam);
		client_height_ = HIWORD(lParam);
		if( d3d_device_ )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				pause_ = true;
				minimized_ = true;
				maximized_ = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				pause_ = false;
				minimized_ = false;
				maximized_ = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( minimized_ )
				{
					pause_ = false;
					minimized_ = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( maximized_ )
				{
					pause_ = false;
					maximized_ = false;
					OnResize();
				}
				else if( resizing_ )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or swap_chain_->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		pause_ = true;
		resizing_  = true;
		timer_.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		pause_ = false;
		resizing_  = false;
		timer_.Start();
		OnResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            //Set4xMsaaState(!use_4x_msaa_state_);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Application::InitMainWindow() {
  WNDCLASS wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = h_instance_;
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

  h_window_ = CreateWindow(L"MainWnd", main_wnd_caption_.c_str(),
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, h_instance_, 0);

  if (!h_window_) {
    MessageBox(0, L"CreateWindow Failed.", 0, 0);
    return false;
  }

  ShowWindow(h_window_, SW_SHOW);
  UpdateWindow(h_window_);
  return true;
}

bool Application::InitDirect3D() {

#if defined(DEBUG) || defined(_DEBUG)
// Enable the D3D12 debug layer.
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
  ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

  HRESULT device_result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d_device_));

  if(FAILED(device_result)) {
    ComPtr<IDXGIAdapter> pWarpAdapter;
    ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d_device_)));
  }

  ThrowIfFailed(d3d_device_->CreateFence(0, 
    D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d_fence_)));

  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = back_buffer_format_;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(d3d_device_->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    use_4x_msaa_quanlity_ = msQualityLevels.NumQualityLevels;
	assert(use_4x_msaa_quanlity_ > 0 && "Unexpected MSAA quality level.");

  rtv_descriptor_size = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsv_descriptor_size = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uav_descriptor_size = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef _DEBUG
    LogAdapters();
#endif

  CreateCommand();
  CreateSwapChain();
  CreateRtvAndDsvDescriptorHeaps();

  return true;
}

void Application::CreateCommand() {
  ThrowIfFailed(d3d_device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    IID_PPV_ARGS(&d3d_command_allocator_)));

  D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
  ThrowIfFailed(d3d_device_->CreateCommandQueue(&command_queue_desc,
    IID_PPV_ARGS(&d3d_command_queue_)));

  ThrowIfFailed(d3d_device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    d3d_command_allocator_.Get(), nullptr, IID_PPV_ARGS(&d3d_command_list_)));
  
  d3d_command_list_->Close();
}

void Application::CreateSwapChain() {
  swap_chain_.Reset();

  DXGI_SWAP_CHAIN_DESC sd;
  sd.BufferDesc.Width = client_width_;
  sd.BufferDesc.Height = client_height_;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferDesc.Format = back_buffer_format_;
  sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  sd.SampleDesc.Quality = use_4x_msaa_state_ ? (use_4x_msaa_quanlity_-1) : 0;
  sd.SampleDesc.Count = use_4x_msaa_state_ ? 4 : 1;
  sd.BufferCount = kSwapBufferCount;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = h_window_;
  sd.Windowed = true;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  ThrowIfFailed(dxgi_factory->CreateSwapChain(d3d_command_queue_.Get(), &sd, swap_chain_.GetAddressOf()));
}


void Application::FlushCommandQueue() {
  ++current_fence_;
  ThrowIfFailed(d3d_command_queue_->Signal(d3d_fence_.Get(), current_fence_));
  //  ThrowIfFailed(d3d_fence_->Signal(current_fence_));  //  signal to CPU to wait for GPU

  if(d3d_fence_->GetCompletedValue() < current_fence_)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

    // Fire event when GPU hits current fence.  
    ThrowIfFailed(d3d_fence_->SetEventOnCompletion(current_fence_, eventHandle));

    // Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
	}
}

ID3D12Resource* Application::CurrentBackBuffer()const
{
	return swap_chain_buffers_[current_back_buffer_index_].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Application::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
		current_back_buffer_index_,
		rtv_descriptor_size);
}

D3D12_CPU_DESCRIPTOR_HANDLE Application::DepthStencilView()const
{
	return dsv_heap_->GetCPUDescriptorHandleForHeapStart();
}

//void Application::CalculateFrameStats()
//{
//	// Code computes the average frames per second, and also the 
//	// average time it takes to render one frame.  These stats 
//	// are appended to the window caption bar.
//    
//	static int frameCnt = 0;
//	static float timeElapsed = 0.0f;
//
//	frameCnt++;
//
//	// Compute averages over one second period.
//	if( (timer_.TotalTime() - timeElapsed) >= 1.0f )
//	{
//		float fps = (float)frameCnt; // fps = frameCnt / 1
//		float mspf = 1000.0f / fps;
//
//        wstring fpsStr = to_wstring(fps);
//        wstring mspfStr = to_wstring(mspf);
//
//        wstring windowText = main_wnd_caption_ +
//            L"    fps: " + fpsStr +
//            L"   mspf: " + mspfStr;
//
//        SetWindowText(h_window_, windowText.c_str());
//		
//		// Reset for next average.
//		frameCnt = 0;
//		timeElapsed += 1.0f;
//	}
//}

void Application::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void Application::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, back_buffer_format_);

        ReleaseCom(output);

        ++i;
    }
}

void Application::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}

}