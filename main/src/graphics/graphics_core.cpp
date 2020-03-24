#include "graphics/graphics_core.h"
#include "graphics/command_list_manager.h"
#include "postprocess/depth_of_field.h"


namespace Graphics {

using namespace std;

GraphicsCore Core;

GraphicsCore::~GraphicsCore() {
  //  CommandListManager::Instance().GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).FlushGpu();

  for (UINT i = 0; i < kSwapBufferCount; i++) {
    display_planes_[i].Destroy();
  }
}

void GraphicsCore::Initialize(HWND  h_window_) {
#if defined(DEBUG) || defined(_DEBUG)
  // Enable the D3D12 debug layer.
  {
    ComPtr<ID3D12Debug> debugController;
    ASSERT_SUCCESSED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
  }
#endif
  ASSERT_SUCCESSED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

  HRESULT device_result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));

  if(FAILED(device_result)) {
    ComPtr<IDXGIAdapter> pWarpAdapter;
    ASSERT_SUCCESSED(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    ASSERT_SUCCESSED(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_)));
  }
  CommandListManager::Instance().Initialize(device_.Get());

  CreateSwapChain(h_window_);
  CreateMRTBuffers();

  Graphics::InitializeCommonState();
#ifdef _DEBUG
  LogAdapters();
#endif
}

void GraphicsCore::CreateSwapChain(HWND h_window_) {
  swap_chain_.Reset();

  DXGI_SWAP_CHAIN_DESC sd;
  sd.BufferDesc.Width = client_width_;
  sd.BufferDesc.Height = client_height_;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferDesc.Format = BackBufferFormat;
  sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  sd.SampleDesc.Quality = msaa_4x_state_ ? (msaa_4x_quanlity_-1) : 0;
  sd.SampleDesc.Count = msaa_4x_state_ ? 4 : 1;
  sd.BufferCount = kSwapBufferCount;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = h_window_;
  sd.Windowed = true;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  ASSERT_SUCCESSED(dxgi_factory->CreateSwapChain(CommandListManager::Instance().GetCommandQueue(), &sd, swap_chain_.GetAddressOf()));

  CreateDisplayPlanes();
  CreateViewportAndRect();
}

void GraphicsCore::PreparePresent() {
  CommandContext* command_context = ContextManager::Instance().Allocate(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
  GraphicsContext& graphics_context = command_context->GetGraphicsContext();

  auto& display_plane = display_planes_[current_back_buffer_index_];

  graphics_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  graphics_context.SetViewports(&screen_viewport_);
  graphics_context.SetScissorRects(&scissor_rect_);

  graphics_context.ClearColor(display_plane);
  graphics_context.ClearDepthStencil(depth_buffer_);  
  
  graphics_context.SetRenderTargets(1, &display_plane.Rtv(), depth_buffer_.DSV());

  auto fence_value = graphics_context.Finish();
}

void GraphicsCore::Present() {
  
  auto present_hr = swap_chain_->Present(0, 0);
  if (FAILED(present_hr)) {
    auto device_hr = device_->GetDeviceRemovedReason();
    ASSERT_SUCCESSED(device_hr);
    ASSERT_SUCCESSED(present_hr);
  }
  current_back_buffer_index_ = (current_back_buffer_index_ + 1) % kSwapBufferCount;
}

void GraphicsCore::OnResize(UINT width, UINT height) {
  assert(swap_chain_ != nullptr);

  if (width == 0 || height == 0)
    return;

  if (client_width_ == width && client_height_ == height)
    return;

  client_width_ = width;
  client_height_ = height;
  DebugUtility::Log(L"GraphicsCore resize : width %0, height %1", to_wstring(client_width_), to_wstring(client_height_));
  for (int i = 0; i < kSwapBufferCount; ++i) {
    display_planes_[i].Destroy();
  }
  
  swap_chain_->ResizeBuffers(kSwapBufferCount, client_width_, client_height_,
    BackBufferFormat, DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

  CreateDisplayPlanes();
  CreateViewportAndRect();
  CreateMRTBuffers();
}

void GraphicsCore::CreateDisplayPlanes() {
  current_back_buffer_index_ = 0;
  wstring name(L"Display Plane ");
  for (UINT i = 0; i < kSwapBufferCount; i++) {
    Microsoft::WRL::ComPtr<ID3D12Resource> plane;
    ASSERT_SUCCESSED(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&plane)));

    display_planes_[i].CreateFromSwapChain(name + to_wstring(i), device_.Get(), plane.Detach());
    display_planes_[i].SetColor(Colors::LightSteelBlue);
  }
  
  depth_buffer_.Create(L"Depth Stencil", client_width_, client_height_, DepthStencilFormat);
  for (int i = 0; i < kMRTBufferCount; ++i) {
    mrt_buffers_[i].Destroy();
  }
}

void GraphicsCore::CreateViewportAndRect() {
  screen_viewport_.TopLeftX = 0;
  screen_viewport_.TopLeftY = 0;
  screen_viewport_.Width = static_cast<float>(client_width_);
  screen_viewport_.Height = static_cast<float>(client_height_);
  screen_viewport_.MinDepth = 0.0f;
  screen_viewport_.MaxDepth = 1.0f;

  scissor_rect_ = { 0, 0, client_width_, client_height_ };
}

void GraphicsCore::CreateMRTBuffers() {


  for (int i=0; i<kMRTBufferCount; ++i) {
    auto& buffer = mrt_buffers_[i];

    buffer.SetColor(Colors::Black);
    buffer.Create(L"MRT Buffer " + to_wstring(i), 
      client_width_, client_height_, 1, BackBufferFormat);

    mrt_rtvs_[i] = buffer.Rtv();
    mrt_formats_[i] = BackBufferFormat;
  }

  deferred_depth_buffer_.Destroy();
  deferred_depth_buffer_.Create(L"Deferred Depth Stencil", client_width_, client_height_, DepthStencilFormat);
}

void GraphicsCore::LogAdapters()
{
  UINT i = 0;
  IDXGIAdapter* adapter = nullptr;
  std::vector<IDXGIAdapter*> adapterList;
  while (dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
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

  for (size_t i = 0; i < adapterList.size(); ++i)
  {
    LogAdapterOutputs(adapterList[i]);
    ReleaseCom(adapterList[i]);
  }
}

void GraphicsCore::LogAdapterOutputs(IDXGIAdapter* adapter)
{
  UINT i = 0;
  IDXGIOutput* output = nullptr;
  while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
  {
    DXGI_OUTPUT_DESC desc;
    output->GetDesc(&desc);

    std::wstring text = L"***Output: ";
    text += desc.DeviceName;
    text += L"\n";
    OutputDebugString(text.c_str());

    LogOutputDisplayModes(output, BackBufferFormat);

    ReleaseCom(output);

    ++i;
  }
}

void GraphicsCore::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
  UINT count = 0;
  UINT flags = 0;

  // Call with nullptr to get list count.
  output->GetDisplayModeList(format, flags, &count, nullptr);

  std::vector<DXGI_MODE_DESC> modeList(count);
  output->GetDisplayModeList(format, flags, &count, &modeList[0]);

  for (auto& x : modeList)
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