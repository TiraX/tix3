/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"

#if COMPILE_WITH_RHI_DX12
#include "d3dx12.h"
#include "FRHIDx12Conversion.h"
#include "TDeviceWin32.h"
#include "FFrameResourcesDx12.h"
#include "FGPUBufferDx12.h"
#include "FGPUTextureDx12.h"
#include "FPipelineDx12.h"
#include "FRenderTargetDx12.h"
#include "FShaderDx12.h"
#include "FArgumentBufferDx12.h"
#include "FGPUCommandSignatureDx12.h"
#include "FGPUCommandBufferDx12.h"
#include "FRtxPipelineDx12.h"
#include "FAccelerationStructureDx12.h"
#include <DirectXColors.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include "FRHIAsyncDx12.h"

#if !defined(TIX_SHIPPING)
// Include pix function for dx12 profile
#include <WinPixEventRuntime\pix3.h>
#pragma comment (lib, "WinPixEventRuntime.lib")
#define BEGIN_EVENT(CmdList, formatStr) PIXBeginEvent(CmdList, 0xffffffff, formatStr)
#define END_EVENT(CmdList) PIXEndEvent(CmdList)
#else
#define BEGIN_EVENT(CmdList, formatStr)
#define END_EVENT(CmdList)
#endif


// link libraries
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")

namespace tix
{
	FRHIDx12::FRHIDx12(const TString& InRHIName)
		: FRHI(ERHI_DX12, InRHIName)
		, CurrentFrame(0)
		, GraphicsNumBarriersToFlush(0)
		, ComputeNumBarriersToFlush(0)
		, DXR(nullptr)
	{
		// Create frame resource holders
		for (int32 i = 0; i < FRHIConfig::FrameBufferNum; ++i)
		{
			ResHolders[i] = ti_new FFrameResourcesDx12;
			FrameResources[i] = ResHolders[i];
			FenceValues[i] = 0;
		}
		// Reserve spaces for AsyncRHIs
		AsyncRHIs.reserve(4);
	}

	FRHIDx12::~FRHIDx12()
	{
		// delete frame resource holders
		for (int32 i = 0; i < FRHIConfig::FrameBufferNum; ++i)
		{
			ti_delete ResHolders[i];
			ResHolders[i] = nullptr;
			FrameResources[i] = nullptr;
		}

		ti_delete DXR;
	}

	FRHI* FRHIDx12::CreateAsyncRHI(const TString& InRHIName)
	{
		FRHIAsyncDx12* AsyncDx12 = ti_new FRHIAsyncDx12(InRHIName);
		AsyncRHIs.push_back(AsyncDx12);

		AsyncDx12->InitAsyncRHI(D3dDevice);
		return AsyncDx12;
	}

#define ENABLE_DX_DEBUG_LAYER	(1)
	void FRHIDx12::InitRHI()
	{
		HRESULT Hr;

#if defined(TIX_DEBUG) && (ENABLE_DX_DEBUG_LAYER)
		// If the project is in a debug build, enable debugging via SDK Layers.
		{
			ComPtr<ID3D12Debug> DebugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
			{
				// Disable Debug layer for Nsights Graphics to profile shader and GPU trace.
				DebugController->EnableDebugLayer();
			}
			else
			{
				_LOG(ELog::Warning, "Direct3D Debug Device is NOT avaible.\n");
			}

			// Try to create debug factory
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
			{
				VALIDATE_HRESULT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DxgiFactory)));

				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
			}
			else
			{
				// Failed to create debug factory, create a normal one
				VALIDATE_HRESULT(CreateDXGIFactory2(0, IID_PPV_ARGS(&DxgiFactory)));
			}
		}
#else
		// Create D3D12 Device
		VALIDATE_HRESULT(CreateDXGIFactory2(0, IID_PPV_ARGS(&DxgiFactory)));
#endif
		ComPtr<IDXGIAdapter1> Adapter;
		GetHardwareAdapter(&Adapter);

		// Create the Direct3D 12 API device object
		Hr = D3D12CreateDevice(
			Adapter.Get(),					// The hardware adapter.
			D3D_FEATURE_LEVEL_11_0,			// Minimum feature level this app can support.
			IID_PPV_ARGS(&D3dDevice)		// Returns the Direct3D device created.
		);

		VALIDATE_HRESULT(Hr);
#if defined(TIX_DEBUG)
		// Configure debug device (if active).
		ComPtr<ID3D12InfoQueue> D3dInfoQueue;
		if (SUCCEEDED(D3dDevice.As(&D3dInfoQueue)))
		{
			D3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			D3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			D3D12_MESSAGE_ID hide[] =
			{
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
			};
			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			D3dInfoQueue->AddStorageFilterEntries(&filter);
		}
#endif
		FeatureCheck();

		// Prevent the GPU from over-clocking or under-clocking to get consistent timings
		//if (DeveloperModeEnabled)
		//	D3dDevice->SetStablePowerState(TRUE);

		// Create the command queue. allocator, command list and fence
		// Command Queue
		D3D12_COMMAND_LIST_TYPE CmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.Type = CmdListType;

		VALIDATE_HRESULT(D3dDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue)));
		CommandQueue->SetName(FromString(RHIName + "-CmdQueue").c_str());

		// Command Allocators
		char Name[128];
		for (uint32 n = 0; n < FRHIConfig::FrameBufferNum; n++)
		{
			sprintf_s(Name, 128, "%s-CmdAllocator%d", RHIName.c_str(), n);
			VALIDATE_HRESULT(D3dDevice->CreateCommandAllocator(CmdListType, IID_PPV_ARGS(&CommandAllocators[n])));
			CommandAllocators[n]->SetName(FromString(Name).c_str());
		}

		// Command List
		VALIDATE_HRESULT(D3dDevice->CreateCommandList(
			0,
			CmdListType,
			CommandAllocators[0].Get(),
			nullptr,
			IID_PPV_ARGS(&CommandList)));
		CommandList->SetName(FromString(RHIName + "-CmdList").c_str());
		VALIDATE_HRESULT(CommandList->Close());

		// Fence
		VALIDATE_HRESULT(D3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CommandFence)));

		// Create descriptor heaps for render target views and depth stencil views.
		DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Create(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Create(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		
		FenceValues[CurrentFrame]++;

		FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (FenceEvent == nullptr)
		{
			Hr = HRESULT_FROM_WIN32(GetLastError());
			RuntimeFail();
		}

		CreateWindowsSizeDependentResources();

		// Describe and create a shader resource view (SRV) heap for the texture.
		DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Create(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		_LOG(ELog::Log, "  RHI DirectX 12 inited.\n");

		// Init raytracing
		if (RHIConfig.IsFeatureSupported(RHI_FEATURE_RAYTRACING))
		{
			if (InitRaytracing())
			{
				_LOG(ELog::Log, "    DXR inited.\n");
			}
			else
			{
				_LOG(ELog::Error, "    Can not init DXR.\n");
			}
		}
	}

	void FRHIDx12::FeatureCheck()
	{
		// Check for DXR
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 FeatureSupportData = {};
		if (SUCCEEDED(D3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &FeatureSupportData, sizeof(FeatureSupportData)))
			&& FeatureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			SupportFeature(RHI_FEATURE_RAYTRACING);
			RHIConfig.EnableFeature(RHI_FEATURE_RAYTRACING, true);
		}
	}

	bool FRHIDx12::InitRaytracing()
	{
		DXR = ti_new FRHIDXR();

		return DXR->Init(D3dDevice.Get(), CommandList.Get());
	}

	// This method acquires the first available hardware adapter that supports Direct3D 12.
	// If no such adapter can be found, *ppAdapter will be set to nullptr.
	void FRHIDx12::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;

		ComPtr<IDXGIAdapter1> Adapter;
		ComPtr<IDXGIFactory6> Factory6;

		HRESULT Hr = DxgiFactory.As(&Factory6);
		if (FAILED(Hr))
		{
			// Get Adapter use a safe way
			for (uint32 AdapterIndex = 0; DXGI_ERROR_NOT_FOUND != DxgiFactory->EnumAdapters1(AdapterIndex, &Adapter); AdapterIndex++)
			{
				DXGI_ADAPTER_DESC1 desc;
				Adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					continue;
				}

				// Check to see if the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					char AdapterName[128];
					size_t Converted;
					wcstombs_s(&Converted, AdapterName, 128, desc.Description, 128);
					_LOG(ELog::Log, "D3D12-capable hardware found:  %s (%u MB)\n", AdapterName, desc.DedicatedVideoMemory >> 20);
					break;
				}
			}
		}
		else
		{
			for (uint32 AdapterIndex = 0; DXGI_ERROR_NOT_FOUND != Factory6->EnumAdapterByGpuPreference(AdapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&Adapter)); AdapterIndex++)
			{
				DXGI_ADAPTER_DESC1 Desc;
				Adapter->GetDesc1(&Desc);

				if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					continue;
				}

				// Check to see if the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					char AdapterName[128];
					size_t Converted;
					wcstombs_s(&Converted, AdapterName, 128, Desc.Description, 128);
					_LOG(ELog::Log, "D3D12-capable hardware found:  %s (%u MB)\n", AdapterName, Desc.DedicatedVideoMemory >> 20);
					break;
				}
			}

		}

		*ppAdapter = Adapter.Detach();
		TI_ASSERT(*ppAdapter != nullptr);
	}

	void FRHIDx12::CreateWindowsSizeDependentResources()
	{
		// Clear the previous window size specific content and update the tracked fence values.
		for (uint32 n = 0; n < FRHIConfig::FrameBufferNum; n++)
		{
			BackBufferRTs[n] = nullptr;
			FenceValues[n] = FenceValues[CurrentFrame];
		}

		// Use windows' device size directly.
		FInt2 WindowSize = TEngine::Get()->GetDevice()->GetDeviceSize();

		// The width and height of the swap chain must be based on the window's
		// natively-oriented width and height. If the window is not in the native
		// orientation, the dimensions must be reversed.
		DXGI_MODE_ROTATION displayRotation = DXGI_MODE_ROTATION_IDENTITY;

		uint32 BackBufferWidth = lround(WindowSize.X);
		uint32 BackBufferHeight = lround(WindowSize.Y);

		HRESULT hr;

		const DXGI_FORMAT BackBufferFormat = GetDxPixelFormat(FRHIConfig::DefaultBackBufferFormat);
		const DXGI_FORMAT DepthBufferFormat = GetDxPixelFormat(FRHIConfig::DefaultDepthBufferFormat);

		if (SwapChain != nullptr)
		{
			// If the swap chain already exists, resize it.
			HRESULT hr = SwapChain->ResizeBuffers(FRHIConfig::FrameBufferNum, BackBufferWidth, BackBufferHeight, BackBufferFormat, 0);

			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
			{
				// If the device was removed for any reason, a new device and swap chain will need to be created.

				// Do not continue execution of this method. DeviceResources will be destroyed and re-created.
				return;
			}
			else
			{
				VALIDATE_HRESULT(hr);
			}
		}
		else
		{
			// Otherwise, create a new one using the same adapter as the existing Direct3D device.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

			swapChainDesc.Width = BackBufferWidth;						// Match the size of the window.
			swapChainDesc.Height = BackBufferHeight;
			swapChainDesc.Format = BackBufferFormat;
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;							// Don't use multi-sampling.
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = FRHIConfig::FrameBufferNum;			// Use triple-buffering to minimize latency.
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// All Windows Universal apps must use _FLIP_ SwapEffects.
			swapChainDesc.Flags = 0;
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

			ComPtr<IDXGISwapChain1> swapChain;

			TDeviceWin32* DeviceWin32 = dynamic_cast<TDeviceWin32*>(TEngine::Get()->GetDevice());
			TI_ASSERT(DeviceWin32);
			HWND HWnd = DeviceWin32->GetWnd();

			hr = DxgiFactory->CreateSwapChainForHwnd(
				CommandQueue.Get(),	// Swap chains need a reference to the command queue in DirectX 12.
				HWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain
			);
			VALIDATE_HRESULT(hr);

			hr = swapChain.As(&SwapChain);
			VALIDATE_HRESULT(hr);
		}

		hr = SwapChain->SetRotation(displayRotation);
		VALIDATE_HRESULT(hr);

		// Create render target views of the swap chain back buffer.
		{
			CurrentFrame = SwapChain->GetCurrentBackBufferIndex();

			BackBufferDescriptorTable = CreateRenderResourceTable(FRHIConfig::FrameBufferNum, EResourceHeapType::RenderTarget);
			for (uint32 n = 0; n < FRHIConfig::FrameBufferNum; n++)
			{
				BackBufferDescriptors[n] = GetCpuDescriptorHandle(EResourceHeapType::RenderTarget, BackBufferDescriptorTable->GetIndexAt(n));
				VALIDATE_HRESULT(SwapChain->GetBuffer(n, IID_PPV_ARGS(&BackBufferRTs[n])));
				D3dDevice->CreateRenderTargetView(BackBufferRTs[n].Get(), nullptr, BackBufferDescriptors[n]);

				WCHAR name[32];
				if (swprintf_s(name, L"BackBufferRTs[%u]", n) > 0)
				{
					BackBufferRTs[n].Get()->SetName(name);
				}
			}
		}

		// Create a depth stencil and view.
		{
			D3D12_HEAP_PROPERTIES depthHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			D3D12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DepthBufferFormat, BackBufferWidth, BackBufferHeight, 1, 1);
			depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			CD3DX12_CLEAR_VALUE depthOptimizedClearValue(DepthBufferFormat, 1.0f, 0);

			hr = D3dDevice->CreateCommittedResource(
				&depthHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&depthResourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&DepthStencil)
			);
			VALIDATE_HRESULT(hr);

			DepthStencil->SetName(L"DepthStencil");

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DepthBufferFormat;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			DepthStencilDescriptorTable = CreateRenderResourceTable(1, EResourceHeapType::DepthStencil);
			DepthStencilDescriptor = GetCpuDescriptorHandle(EResourceHeapType::DepthStencil, DepthStencilDescriptorTable->GetStartIndex());
			D3dDevice->CreateDepthStencilView(DepthStencil.Get(), &dsvDesc, DepthStencilDescriptor);
		}
		
		// Set the 3D rendering viewport to target the entire window.
		FRHI::SetViewport(FViewport(0, 0, BackBufferWidth, BackBufferHeight));
	}

	void FRHIDx12::BeginFrame()
	{
		FRHI::BeginFrame();

		TI_ASSERT(GraphicsNumBarriersToFlush == 0);
		TI_ASSERT(ComputeNumBarriersToFlush == 0);

		// Reset command list
		VALIDATE_HRESULT(CommandAllocators[CurrentFrame]->Reset());
		VALIDATE_HRESULT(CommandList->Reset(CommandAllocators[CurrentFrame].Get(), nullptr));

		// Set the descriptor heaps to be used by this frame.
		ID3D12DescriptorHeap* ppHeaps[] = { DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetHeap() };
		CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// Set the viewport and scissor rectangle.
		D3D12_VIEWPORT ViewportDx = { float(Viewport.Left), float(Viewport.Top), float(Viewport.Width), float(Viewport.Height), 0.f, 1.f };
		CommandList->RSSetViewports(1, &ViewportDx);
		D3D12_RECT ScissorRect = { Viewport.Left, Viewport.Top, Viewport.Width, Viewport.Height };
		CommandList->RSSetScissorRects(1, &ScissorRect);

		// Reset Bounded resource
		CurrentBoundResource.Reset();
	}

	void FRHIDx12::BeginRenderToFrameBuffer()
	{
		// Start render to frame buffer.
		// Indicate this resource will be in use as a render target.
		_Transition(BackBufferRTs[CurrentFrame].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		FlushBarriers(CommandList.Get());

		END_EVENT(CommandList.Get());
		BEGIN_EVENT(CommandList.Get(), "RenderToFrameBuffer");

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = BackBufferDescriptors[CurrentFrame];
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = DepthStencilDescriptor;
		CommandList->ClearRenderTargetView(renderTargetView, DirectX::Colors::CornflowerBlue, 0, nullptr);
		CommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		CommandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);
	}

	void FRHIDx12::EndFrame()
	{
		// Indicate that the render target will now be used to present when the command list is done executing.
		_Transition(BackBufferRTs[CurrentFrame].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		FlushBarriers(CommandList.Get());

		END_EVENT(CommandList.Get());

		VALIDATE_HRESULT(CommandList->Close());
		ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
		CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// The first argument instructs DXGI to block until VSync, putting the application
		// to sleep until the next VSync. This ensures we don't waste any cycles rendering
		// frames that will never be displayed to the screen.
		HRESULT hr = SwapChain->Present(1, 0);

		// If the device was removed either by a disconnection or a driver upgrade, we 
		// must recreate all device resources.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
		}
		else
		{
			VALIDATE_HRESULT(hr);

			MoveToNextFrame();
		}

		// Reset current bound resources
		CurrentBoundResource.Reset();
	}

	void FRHIDx12::BeginEvent(const int8* InEventName)
	{
		BEGIN_EVENT(CommandList.Get(), InEventName);
	}

	void FRHIDx12::BeginEvent(const int8* InEventName, int32 Index)
	{
		int8 TempName[64];
		sprintf(TempName, "%s_%d", InEventName, Index);
		BEGIN_EVENT(CommandList.Get(), TempName);
	}

	void FRHIDx12::EndEvent()
	{
		END_EVENT(CommandList.Get());
	}

	FGPUBufferPtr FRHIDx12::CreateGPUBuffer()
	{
		return ti_new FGPUBufferDx12();
	}
	
	FGPUTexturePtr FRHIDx12::CreateGPUTexture()
	{
		return ti_new FGPUTextureDx12();
	}

	FPipelinePtr FRHIDx12::CreatePipeline(FShaderPtr InShader)
	{
		return ti_new FPipelineDx12(InShader);
	}

	FRenderTargetPtr FRHIDx12::CreateRenderTarget(int32 W, int32 H)
	{
		return ti_new FRenderTargetDx12(W, H);
	}

	FShaderPtr FRHIDx12::CreateShader(const TShaderNames& InNames, E_SHADER_TYPE Type)
	{
		return ti_new FShaderDx12(InNames, Type);
	}

	FShaderPtr FRHIDx12::CreateComputeShader(const TString& ComputeShaderName)
	{
		return ti_new FShaderDx12(ComputeShaderName, EST_COMPUTE);
	}

	FShaderPtr FRHIDx12::CreateRtxShaderLib(const TString& ShaderLibName)
	{
		return ti_new FShaderDx12(ShaderLibName, EST_SHADERLIB);
	}

	FArgumentBufferPtr FRHIDx12::CreateArgumentBuffer(int32 ReservedSlots)
	{
		return ti_new FArgumentBufferDx12(ReservedSlots);
	}

	FGPUCommandSignaturePtr FRHIDx12::CreateGPUCommandSignature(FPipelinePtr Pipeline, const TVector<E_GPU_COMMAND_TYPE>& CommandStructure)
	{
		return ti_new FGPUCommandSignatureDx12(Pipeline, CommandStructure);
	}

	FGPUCommandBufferPtr FRHIDx12::CreateGPUCommandBuffer(FGPUCommandSignaturePtr GPUCommandSignature, uint32 CommandsCount, uint32 Flag)
	{
		return ti_new FGPUCommandBufferDx12(GPUCommandSignature, CommandsCount, Flag);
	}

	FRtxPipelinePtr FRHIDx12::CreateRtxPipeline(FShaderPtr InShader)
	{
		return ti_new FRtxPipelineDx12(InShader);
	}

	FTopLevelAccelerationStructurePtr FRHIDx12::CreateTopLevelAccelerationStructure()
	{
		return ti_new FTopLevelAccelerationStructureDx12();
	}

	FBottomLevelAccelerationStructurePtr FRHIDx12::CreateBottomLevelAccelerationStructure()
	{
		return ti_new FBottomLevelAccelerationStructureDx12();
	}

	int32 FRHIDx12::GetCurrentEncodingFrameIndex()
	{
		return CurrentFrame;
	}

	// Wait for pending GPU work to complete.
	void FRHIDx12::WaitingForGpu()
	{
		// Schedule a Signal command in the queue.
		VALIDATE_HRESULT(CommandQueue->Signal(CommandFence.Get(), FenceValues[CurrentFrame]));

		// Wait until the fence has been crossed.
		VALIDATE_HRESULT(CommandFence->SetEventOnCompletion(FenceValues[CurrentFrame], FenceEvent));
		WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);

		// Increment the fence value for the current frame.
		FenceValues[CurrentFrame]++;
	}

	// Prepare to render the next frame.
	void FRHIDx12::MoveToNextFrame()
	{
		// Schedule a Signal command in the queue.
		const uint64 currentFenceValue = FenceValues[CurrentFrame];
		VALIDATE_HRESULT(CommandQueue->Signal(CommandFence.Get(), currentFenceValue));

		// Advance the frame index.
		CurrentFrame = SwapChain->GetCurrentBackBufferIndex();

		// Check to see if the next frame is ready to start.
		if (CommandFence->GetCompletedValue() < FenceValues[CurrentFrame])
		{
			VALIDATE_HRESULT(CommandFence->SetEventOnCompletion(FenceValues[CurrentFrame], FenceEvent));
			WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		FenceValues[CurrentFrame] = currentFenceValue + 1;

		// Release resources references for next drawing
		FrameResources[CurrentFrame]->RemoveAllReferences();

		FRHI::GPUFrameDone();
	}

	void FRHIDx12::HoldResourceReference(FRenderResourcePtr InResource)
	{
		ResHolders[CurrentFrame]->HoldReference(InResource);
	}

	void FRHIDx12::HoldResourceReference(ComPtr<ID3D12Resource> InDxResource)
	{
		ResHolders[CurrentFrame]->HoldDxReference(InDxResource);
	}

	//------------------------------------------------------------------------------------------------
	// All arrays must be populated (e.g. by calling GetCopyableFootprints)
	uint64 FRHIDx12::UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		uint64 RequiredSize,
		_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		_In_reads_(NumSubresources) const uint32* pNumRows,
		_In_reads_(NumSubresources) const uint64* pRowSizesInBytes,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		// Minor validation
		D3D12_RESOURCE_DESC IntermediateDesc = pIntermediate->GetDesc();
		D3D12_RESOURCE_DESC DestinationDesc = pDestinationResource->GetDesc();
		if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
			IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
			RequiredSize >(SIZE_T) - 1 ||
			(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1)))
		{
			return 0;
		}

		uint8* pData;
		HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		for (uint32 i = 0; i < NumSubresources; ++i)
		{
			if (pRowSizesInBytes[i] > (SIZE_T)-1) return 0;
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
			MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		pIntermediate->Unmap(0, nullptr);

		if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_BOX SrcBox(uint32(pLayouts[0].Offset), uint32(pLayouts[0].Offset + pLayouts[0].Footprint.Width));
			pCmdList->CopyBufferRegion(
				pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		}
		else
		{
			for (uint32 i = 0; i < NumSubresources; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst(pDestinationResource, i + FirstSubresource);
				CD3DX12_TEXTURE_COPY_LOCATION Src(pIntermediate, pLayouts[i]);
				pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
		}
		return RequiredSize;
	}

	//------------------------------------------------------------------------------------------------
	// Heap-allocating UpdateSubresources implementation
	uint64 FRHIDx12::UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		uint64 IntermediateOffset,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		uint64 RequiredSize = 0;
		uint64 MemToAlloc = static_cast<uint64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(uint32) + sizeof(uint64)) * NumSubresources;
		if (MemToAlloc > SIZE_MAX)
		{
			return 0;
		}
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == nullptr)
		{
			return 0;
		}
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		uint64* pRowSizesInBytes = reinterpret_cast<uint64*>(pLayouts + NumSubresources);
		uint32* pNumRows = reinterpret_cast<uint32*>(pRowSizesInBytes + NumSubresources);

		D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
		ID3D12Device* pDevice;
		pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		pDevice->Release();

		uint64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
		HeapFree(GetProcessHeap(), 0, pMem);
		return Result;
	}

	void FRHIDx12::_Transition(
		_In_ ID3D12Resource* pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		uint32 subresource,
		D3D12_RESOURCE_BARRIER_FLAGS flags)
	{
		TI_ASSERT(pResource != nullptr);
		if (GraphicsNumBarriersToFlush >= FRHIConfig::MaxResourceBarrierBuffers)
		{
			FlushBarriers(CommandList.Get());
		}
		D3D12_RESOURCE_BARRIER& Barrier = GraphicsBarrierBuffers[GraphicsNumBarriersToFlush];
		GraphicsNumBarriersToFlush++;
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Flags = flags;
		Barrier.Transition.pResource = pResource;
		Barrier.Transition.StateBefore = stateBefore;
		Barrier.Transition.StateAfter = stateAfter;
		Barrier.Transition.Subresource = subresource;
	}

	void FRHIDx12::UAVBarrier(_In_ ID3D12Resource* pResource)
	{
		D3D12_RESOURCE_BARRIER& barrier = GraphicsBarrierBuffers[GraphicsNumBarriersToFlush++];
		TI_ASSERT(GraphicsNumBarriersToFlush <= FRHIConfig::MaxResourceBarrierBuffers);
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = pResource;
	}

	void FRHIDx12::FlushBarriers(
		_In_ ID3D12GraphicsCommandList* pCmdList)
	{
		if (GraphicsNumBarriersToFlush > 0)
		{
			pCmdList->ResourceBarrier(GraphicsNumBarriersToFlush, GraphicsBarrierBuffers);
			GraphicsNumBarriersToFlush = 0;
		}
	}

	inline FShaderDx12* GetDx12Shader(TPipelinePtr Pipeline, E_SHADER_STAGE Stage)
	{
		TI_ASSERT(0);
		if (Pipeline->GetDesc().Shader != nullptr)
		{
			//return static_cast<FShaderDx12*>(Pipeline->GetDesc().Shaders[Stage]->ShaderResource.get());
		}
		return nullptr;
	}

	bool FRHIDx12::UpdateHardwareResourceGraphicsPipeline(FPipelinePtr Pipeline, const TPipelineDesc& Desc)
	{
		FPipelineDx12 * PipelineDx12 = static_cast<FPipelineDx12*>(Pipeline.get());
		FShaderPtr Shader = Pipeline->GetShader();

		TI_ASSERT(Shader->GetShaderType() == EST_RENDER);

		TVector<E_MESH_STREAM_INDEX> VertexStreams = TVertexBuffer::GetSteamsFromFormat(Desc.VsFormat);
		TVector<E_INSTANCE_STREAM_INDEX> InstanceStreams = TInstanceBuffer::GetSteamsFromFormat(Desc.InsFormat);
		TVector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
		InputLayout.resize(VertexStreams.size() + InstanceStreams.size());

		// Fill layout desc
		uint32 VertexDataOffset = 0;
		for (uint32 i = 0; i < VertexStreams.size(); ++i)
		{
			E_MESH_STREAM_INDEX Stream = VertexStreams[i];
			D3D12_INPUT_ELEMENT_DESC& InputElement = InputLayout[i];
			InputElement.SemanticName = TVertexBuffer::SemanticName[Stream];
			InputElement.SemanticIndex = TVertexBuffer::SemanticIndex[Stream];
			InputElement.Format = k_MESHBUFFER_STREAM_FORMAT_MAP[Stream];
			InputElement.InputSlot = 0;
			InputElement.AlignedByteOffset = VertexDataOffset;
			VertexDataOffset += TVertexBuffer::SemanticSize[Stream];
			InputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			InputElement.InstanceDataStepRate = 0;
		}
		uint32 InstanceDataOffset = 0;
		for (uint32 i = 0; i < InstanceStreams.size(); ++i)
		{
			E_INSTANCE_STREAM_INDEX Stream = InstanceStreams[i];
			D3D12_INPUT_ELEMENT_DESC& InputElement = InputLayout[VertexStreams.size() + i];
			InputElement.SemanticName = TInstanceBuffer::SemanticName[Stream];
			InputElement.SemanticIndex = TInstanceBuffer::SemanticIndex[Stream];
			InputElement.Format = k_INSTANCEBUFFER_STREAM_FORMAT_MAP[Stream];
			InputElement.InputSlot = 1;
			InputElement.AlignedByteOffset = InstanceDataOffset;
			InstanceDataOffset += TInstanceBuffer::SemanticSize[Stream];
			InputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			InputElement.InstanceDataStepRate = 1;
		}

		FShaderDx12* ShaderDx12 = static_cast<FShaderDx12*>(Shader.get());
		FShaderBindingPtr Binding = ShaderDx12->ShaderBinding;
		TI_ASSERT(Binding != nullptr);
		FRootSignatureDx12* PipelineRS = static_cast<FRootSignatureDx12*>(Binding.get());

		D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.InputLayout = { &(InputLayout[0]), uint32(InputLayout.size()) };
		state.pRootSignature = PipelineRS->Get();

		state.VS = { ShaderDx12->ShaderCodes[ESS_VERTEX_SHADER]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[ESS_VERTEX_SHADER]->GetLength()) };
		if (ShaderDx12->ShaderCodes[ESS_PIXEL_SHADER] != nullptr)
		{
			TI_ASSERT(ShaderDx12->ShaderCodes[ESS_PIXEL_SHADER]->GetLength() > 0);
			state.PS = { ShaderDx12->ShaderCodes[ESS_PIXEL_SHADER]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[ESS_PIXEL_SHADER]->GetLength()) };
		}
		if (ShaderDx12->ShaderCodes[ESS_DOMAIN_SHADER] != nullptr)
		{
			TI_ASSERT(ShaderDx12->ShaderCodes[ESS_DOMAIN_SHADER]->GetLength() > 0);
			state.PS = { ShaderDx12->ShaderCodes[ESS_DOMAIN_SHADER]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[ESS_DOMAIN_SHADER]->GetLength()) };
		}
		if (ShaderDx12->ShaderCodes[ESS_HULL_SHADER] != nullptr)
		{
			TI_ASSERT(ShaderDx12->ShaderCodes[ESS_HULL_SHADER]->GetLength() > 0);
			state.PS = { ShaderDx12->ShaderCodes[ESS_HULL_SHADER]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[ESS_HULL_SHADER]->GetLength()) };
		}
		if (ShaderDx12->ShaderCodes[ESS_GEOMETRY_SHADER] != nullptr)
		{
			TI_ASSERT(ShaderDx12->ShaderCodes[ESS_GEOMETRY_SHADER]->GetLength() > 0);
			state.PS = { ShaderDx12->ShaderCodes[ESS_GEOMETRY_SHADER]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[ESS_GEOMETRY_SHADER]->GetLength()) };
		}

		MakeDx12RasterizerDesc(Desc, state.RasterizerState);
		MakeDx12BlendState(Desc, state.BlendState);
		MakeDx12DepthStencilState(Desc, state.DepthStencilState);
		state.SampleMask = UINT_MAX;
		state.PrimitiveTopologyType = GetDx12TopologyType(Desc.PrimitiveType);
		TI_ASSERT(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED != state.PrimitiveTopologyType);
		state.NumRenderTargets = Desc.RTCount;
		TI_ASSERT(Desc.RTCount >= 0);
		for (int32 r = 0; r < Desc.RTCount; ++r)
		{
			state.RTVFormats[r] = GetDxPixelFormat(Desc.RTFormats[r]);
		}
		if (Desc.DepthFormat != EPF_UNKNOWN)
		{
			state.DSVFormat = GetDxPixelFormat(Desc.DepthFormat);
		}
		else
		{
			state.DSVFormat = DXGI_FORMAT_UNKNOWN;
		}
		TI_ASSERT(DXGI_FORMAT_UNKNOWN != state.RTVFormats[0] || DXGI_FORMAT_UNKNOWN != state.DSVFormat);
		state.SampleDesc.Count = 1;

		VALIDATE_HRESULT(D3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&(PipelineDx12->PipelineState))));
		DX_SETNAME(PipelineDx12->PipelineState.Get(), Pipeline->GetResourceName());

		// Shader data can be deleted once the pipeline state is created.
		ShaderDx12->ReleaseShaderCode();
		HoldResourceReference(Pipeline);

		return true;
	}


	bool FRHIDx12::UpdateHardwareResourceComputePipeline(FPipelinePtr Pipeline)
	{
		FPipelineDx12* PipelineDx12 = static_cast<FPipelineDx12*>(Pipeline.get());
		FShaderPtr Shader = Pipeline->GetShader();

		TI_ASSERT(Shader->GetShaderType() == EST_COMPUTE);

		// Compute pipeline 
		FShaderDx12* ShaderDx12 = static_cast<FShaderDx12*>(Shader.get());
		FShaderBindingPtr Binding = ShaderDx12->ShaderBinding;
		TI_ASSERT(Binding != nullptr);
		FRootSignatureDx12* PipelineRS = static_cast<FRootSignatureDx12*>(Binding.get());

		D3D12_COMPUTE_PIPELINE_STATE_DESC state = {};
		state.pRootSignature = PipelineRS->Get();
		state.CS = { ShaderDx12->ShaderCodes[0]->GetBuffer(), uint32(ShaderDx12->ShaderCodes[0]->GetLength()) };

		VALIDATE_HRESULT(D3dDevice->CreateComputePipelineState(&state, IID_PPV_ARGS(&(PipelineDx12->PipelineState))));

		// Shader data can be deleted once the pipeline state is created.
		ShaderDx12->ReleaseShaderCode();

		HoldResourceReference(Pipeline);

		return true;
	}

	bool FRHIDx12::UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc)
	{
		// This is for Metal
		RuntimeFail();
		return false;
	}

#ifdef TIX_DEBUG
	// Pretty-print a state object tree.
	inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
	{
		std::wstringstream wstr;
		wstr << L"\n";
		wstr << L"--------------------------------------------------------------------\n";
		wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
		if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
		if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

		auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
		{
			std::wostringstream woss;
			for (UINT i = 0; i < numExports; i++)
			{
				woss << L"|";
				if (depth > 0)
				{
					for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
				}
				woss << L" [" << i << L"]: ";
				if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
				woss << exports[i].Name << L"\n";
			}
			return woss.str();
		};

		for (UINT i = 0; i < desc->NumSubobjects; i++)
		{
			wstr << L"| [" << i << L"]: ";
			switch (desc->pSubobjects[i].Type)
			{
			case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
				wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
				break;
			case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
				wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
				break;
			case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
				RuntimeFail();	// Not consider NODE_MASK yet.
				//wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
				break;
			case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
			{
				wstr << L"DXIL Library 0x";
				auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
				wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
				wstr << ExportTree(1, lib->NumExports, lib->pExports);
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
			{
				wstr << L"Existing Library 0x";
				auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
				wstr << collection->pExistingCollection << L"\n";
				wstr << ExportTree(1, collection->NumExports, collection->pExports);
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
			{
				wstr << L"Subobject to Exports Association (Subobject [";
				auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
				UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
				wstr << index << L"])\n";
				for (UINT j = 0; j < association->NumExports; j++)
				{
					wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
				}
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
			{
				wstr << L"DXIL Subobjects to Exports Association (";
				auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
				wstr << association->SubobjectToAssociate << L")\n";
				for (UINT j = 0; j < association->NumExports; j++)
				{
					wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
				}
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
			{
				wstr << L"Raytracing Shader Config\n";
				auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
				wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
				wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
			{
				wstr << L"Raytracing Pipeline Config\n";
				auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
				wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
				break;
			}
			case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
			{
				wstr << L"Hit Group (";
				auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
				wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
				wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
				wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
				wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
				break;
			}
			}
			wstr << L"|--------------------------------------------------------------------\n";
		}
		wstr << L"\n";
		TString s = FromWString(wstr.str());
		_LOG(ELog::Log, "%s", s.c_str());
	}
#else
	inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc) {}
#endif

	bool FRHIDx12::UpdateHardwareResourceRtxPL(FRtxPipelinePtr Pipeline, TRtxPipelinePtr InPipelineDesc)
	{
		FRtxPipelineDx12* RtxPipelineDx12 = static_cast<FRtxPipelineDx12*>(Pipeline.get());
		FShaderPtr Shader = Pipeline->GetShaderLib();
		FShaderDx12* ShaderDx12 = static_cast<FShaderDx12*>(Shader.get());

		const TRtxPipelineDesc& RtxPipelineDesc = InPipelineDesc->GetDesc();

		// Names to TWString for DXR Api
		TVector<TWString> ExportNames;
		ExportNames.resize(RtxPipelineDesc.ExportNames.size());
		for (uint32 i = 0; i < RtxPipelineDesc.ExportNames.size(); i++)
		{
			ExportNames[i] = FromString(RtxPipelineDesc.ExportNames[i].c_str());
		}
		TWString HitGroupName = FromString(RtxPipelineDesc.HitGroupName);
		TWString HitGroupShaders[HITGROUP_NUM];
		HitGroupShaders[HITGROUP_ANY_HIT] = FromString(RtxPipelineDesc.HitGroup[HITGROUP_ANY_HIT]);
		HitGroupShaders[HITGROUP_CLOSEST_HIT] = FromString(RtxPipelineDesc.HitGroup[HITGROUP_CLOSEST_HIT]);
		HitGroupShaders[HITGROUP_INTERSECTION] = FromString(RtxPipelineDesc.HitGroup[HITGROUP_INTERSECTION]);

		// Create Rtx Pipeline state object
		{
			TVector<D3D12_STATE_SUBOBJECT> SubObjects;
			SubObjects.reserve(10);
			D3D12_STATE_SUBOBJECT SubObject;

			// Dxil library
			SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

			D3D12_DXIL_LIBRARY_DESC DxilLibDesc;
			SubObject.pDesc = &DxilLibDesc;
			DxilLibDesc.DXILLibrary.pShaderBytecode = ShaderDx12->ShaderCodes[ESS_SHADER_LIB]->GetBuffer();
			DxilLibDesc.DXILLibrary.BytecodeLength = uint32(ShaderDx12->ShaderCodes[ESS_SHADER_LIB]->GetLength());

			const TVector<TString>& ShaderEntries = RtxPipelineDesc.ExportNames;
			TVector<D3D12_EXPORT_DESC> ExportDesc;
			ExportDesc.resize(ShaderEntries.size());
			for (uint32 i = 0; i < ShaderEntries.size(); i++)
			{
				ExportDesc[i].Name = ExportNames[i].c_str();
				ExportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				ExportDesc[i].ExportToRename = nullptr;
			}
			DxilLibDesc.NumExports = (uint32)ShaderEntries.size();
			DxilLibDesc.pExports = ExportDesc.data();
			SubObjects.push_back(SubObject);

			// Create the state
			D3D12_STATE_OBJECT_DESC Desc;
			Desc.NumSubobjects = (uint32)SubObjects.size();
			Desc.pSubobjects = SubObjects.data();
			Desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

			PrintStateObjectDesc(&Desc);

			VALIDATE_HRESULT(DXR->DXRDevice->CreateStateObject(&Desc, IID_PPV_ARGS(&RtxPipelineDx12->StateObject)));
		}

		// Build Shader Table
		{
			ComPtr<ID3D12StateObjectProperties> StateObjectProperties;
			VALIDATE_HRESULT(RtxPipelineDx12->StateObject.As(&StateObjectProperties));

			// Get shader identifiers
			TI_TODO("Use correct raygen/miss/hitgroup name, for now, use ExportNames[0,1] for raygen and miss, HitGroupName for hitgroup");
			void* RayGenShaderId = StateObjectProperties->GetShaderIdentifier(ExportNames[0].c_str());
			void* MissShaderId = StateObjectProperties->GetShaderIdentifier(ExportNames[1].c_str());
			void* HitgroupShaderId = StateObjectProperties->GetShaderIdentifier(HitGroupName.c_str());

			// DispatchRays: 
			// pDesc->MissShaderTable.StartAddress must be aligned to 64 bytes(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) 
			// and .StrideInBytes must be aligned to 32 bytes(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT)
			uint32 ShaderTableSize = 0;
			// Ray gen
			RtxPipelineDx12->RayGenShaderOffsetAndSize.X = ShaderTableSize;
			RtxPipelineDx12->RayGenShaderOffsetAndSize.Y = TMath::Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
			ShaderTableSize += TMath::Align(RtxPipelineDx12->RayGenShaderOffsetAndSize.Y, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			// Miss
			RtxPipelineDx12->MissShaderOffsetAndSize.X = ShaderTableSize;
			RtxPipelineDx12->MissShaderOffsetAndSize.Y = TMath::Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
			ShaderTableSize += TMath::Align(RtxPipelineDx12->MissShaderOffsetAndSize.Y, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			// Hit Group
			RtxPipelineDx12->HitGroupOffsetAndSize.X = ShaderTableSize;
			RtxPipelineDx12->HitGroupOffsetAndSize.Y = TMath::Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
			ShaderTableSize += TMath::Align(RtxPipelineDx12->HitGroupOffsetAndSize.Y, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			TI_TODO("Calc shader table size with shader parameters");

			TI_ASSERT(RtxPipelineDx12->ShaderTable == nullptr);
			RtxPipelineDx12->ShaderTable = ti_new FUniformBuffer(ShaderTableSize, 1);
			// Build shader table data
			TStreamPtr Data = ti_new TStream(ShaderTableSize);
			Data->Put(RayGenShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			int32 SkipBytes = TMath::Align(Data->GetLength(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) - Data->GetLength();
			Data->Seek(Data->GetLength() + SkipBytes);

			Data->Put(MissShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			SkipBytes = TMath::Align(Data->GetLength(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) - Data->GetLength();
			Data->Seek(Data->GetLength() + SkipBytes);

			Data->Put(HitgroupShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			SkipBytes = TMath::Align(Data->GetLength(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) - Data->GetLength();
			Data->Seek(Data->GetLength() + SkipBytes);

			RtxPipelineDx12->ShaderTable->CreateGPUBuffer(Data);
		}

		return true;
	}

	void FRHIDx12::SetRtxPipeline(FRtxPipelinePtr RtxPipeline)
	{
		FRtxPipelineDx12* RtxPipelineDx12 = static_cast<FRtxPipelineDx12*>(RtxPipeline.get());
		FShaderPtr ShaderLib = RtxPipeline->GetShaderLib();

		// Set Rtx Pipeline
		DXR->DXRCommandList->SetPipelineState1(RtxPipelineDx12->StateObject.Get());

		// Bind Global root signature
		FRootSignatureDx12* GlobalRSDx12 = static_cast<FRootSignatureDx12*>(ShaderLib->GetShaderBinding().get());
		DXR->DXRCommandList->SetComputeRootSignature(GlobalRSDx12->Get());
	}

	void FRHIDx12::TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size)
	{
		FlushResourceStateChange();
		FRtxPipelineDx12* RtxPipelineDx12 = static_cast<FRtxPipelineDx12*>(RtxPipeline.get());
		FGPUBufferDx12* ShaderTableBuffer = static_cast<FGPUBufferDx12*>(RtxPipelineDx12->ShaderTable->GetGPUResource().get());
		FShaderPtr ShaderLib = RtxPipeline->GetShaderLib();

		D3D12_DISPATCH_RAYS_DESC RaytraceDesc = {};
		RaytraceDesc.Width = Size.X;
		RaytraceDesc.Height = Size.Y;
		RaytraceDesc.Depth = Size.Z;

		// RayGen is the first entry in the shader-table
		RaytraceDesc.RayGenerationShaderRecord.StartAddress = 
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->RayGenShaderOffsetAndSize.X;
		RaytraceDesc.RayGenerationShaderRecord.SizeInBytes = RtxPipelineDx12->RayGenShaderOffsetAndSize.Y;

		// Miss is the second entry in the shader-table
		RaytraceDesc.MissShaderTable.StartAddress = 
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->MissShaderOffsetAndSize.X;
		RaytraceDesc.MissShaderTable.StrideInBytes = RtxPipelineDx12->MissShaderOffsetAndSize.Y;
		RaytraceDesc.MissShaderTable.SizeInBytes = RtxPipelineDx12->MissShaderOffsetAndSize.Y;   // Only a s single miss-entry

		// Hit is the third entry in the shader-table
		RaytraceDesc.HitGroupTable.StartAddress =
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->HitGroupOffsetAndSize.X;
		RaytraceDesc.HitGroupTable.StrideInBytes = RtxPipelineDx12->HitGroupOffsetAndSize.Y;
		RaytraceDesc.HitGroupTable.SizeInBytes = RtxPipelineDx12->HitGroupOffsetAndSize.Y;

		// Dispatch
		DXR->DXRCommandList->DispatchRays(&RaytraceDesc);
	}

	TStreamPtr FRHIDx12::ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUBuffer.get());

		D3D12_RESOURCE_DESC Desc = BufferDx12->GetResource()->GetDesc();
		TI_ASSERT(Desc.Height == 1);
		TStreamPtr Result = ti_new TStream();
		Result->ReserveAndFill((uint32)Desc.Width);

		D3D12_RANGE ReadbackBufferRange{ 0, Desc.Width };
		uint8* SrcPointer = nullptr;
		HRESULT Hr = BufferDx12->GetResource()->Map(0, &ReadbackBufferRange, reinterpret_cast<void**>(&SrcPointer));
		TI_ASSERT(SUCCEEDED(Hr));

		memcpy(Result->GetBuffer(), SrcPointer, Desc.Width);

		// Code goes here to access the data via pReadbackBufferData.
		D3D12_RANGE EmptyRange{ 0, 0 };
		BufferDx12->GetResource()->Unmap(0, &EmptyRange);
		return Result;
	}

	void FRHIDx12::CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch)
	{
		FlushBarriers(CommandList.Get());
		FGPUBufferDx12* GPUBufferDx12 = static_cast<FGPUBufferDx12*>(DstBuffer.get());
		FGPUTextureDx12* GPUTextureDx12 = static_cast<FGPUTextureDx12*>(SrcTexture.get());
		D3D12_RESOURCE_DESC Desc = GPUTextureDx12->Resource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
		Footprint.Offset = 0;
		Footprint.Footprint.Format = Desc.Format;
		Footprint.Footprint.Width = (uint32)Desc.Width;
		Footprint.Footprint.Height = (uint32)Desc.Height;
		Footprint.Footprint.Depth = Desc.DepthOrArraySize;
		Footprint.Footprint.RowPitch = RowPitch;
		TI_ASSERT(Footprint.Footprint.RowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0);
		CD3DX12_TEXTURE_COPY_LOCATION Dst(GPUBufferDx12->Resource.Get(), Footprint);
		CD3DX12_TEXTURE_COPY_LOCATION Src(GPUTextureDx12->Resource.Get(), 0);
		CommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		HoldResourceReference(GPUTextureDx12->Resource);
		HoldResourceReference(GPUBufferDx12->Resource);
	}

	void FRHIDx12::CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer)
	{
		FlushBarriers(CommandList.Get());
		FGPUBufferDx12* DstDx12 = static_cast<FGPUBufferDx12*>(DstBuffer.get());
		FGPUBufferDx12* SrcDx12 = static_cast<FGPUBufferDx12*>(SrcBuffer.get());

		CommandList->CopyResource(DstDx12->GetResource(), SrcDx12->GetResource());

		HoldResourceReference(DstDx12->Resource);
		HoldResourceReference(SrcDx12->Resource);
	}

	void FRHIDx12::UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS)
	{
		FBottomLevelAccelerationStructureDx12* ASDx12 = static_cast<FBottomLevelAccelerationStructureDx12*>(BLAS.get());
		UAVBarrier(ASDx12->GetASResource());
	}

	void FRHIDx12::SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name)
	{
		FGPUBufferDx12* GPUBufferDx12 = static_cast<FGPUBufferDx12*>(GPUBuffer.get());

		TWString WName = FromString(Name);
		GPUBufferDx12->Resource.Get()->SetName(WName.c_str());
	}

	void FRHIDx12::SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name)
	{
		FGPUTextureDx12* GPUTextureDx12 = static_cast<FGPUTextureDx12*>(GPUTexture.get());

		TWString WName = FromString(Name);
		GPUTextureDx12->Resource.Get()->SetName(WName.c_str());
	}

	void FRHIDx12::SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState)
	{
		FGPUBufferDx12* GPUBufferDx12 = static_cast<FGPUBufferDx12*>(GPUBuffer.get());

		if (GPUBufferDx12->ResourceState == NewState)
			return;

		D3D12_RESOURCE_STATES StateBefore = GetDx12ResourceState(GPUBufferDx12->ResourceState);
		D3D12_RESOURCE_STATES StateAfter = GetDx12ResourceState(NewState);
		_Transition(
			GPUBufferDx12->Resource.Get(), 
			StateBefore, 
			StateAfter, 
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 
			D3D12_RESOURCE_BARRIER_FLAG_NONE);
		GPUBufferDx12->ResourceState = NewState;
	}

	void FRHIDx12::SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState)
	{
		FGPUTextureDx12* GPUTextureDx12 = static_cast<FGPUTextureDx12*>(GPUTexture.get());

		if (GPUTextureDx12->ResourceState == NewState)
			return;

		D3D12_RESOURCE_STATES StateBefore = GetDx12ResourceState(GPUTextureDx12->ResourceState);
		D3D12_RESOURCE_STATES StateAfter = GetDx12ResourceState(NewState);
		_Transition(
			GPUTextureDx12->Resource.Get(),
			StateBefore,
			StateAfter,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAG_NONE);
		GPUTextureDx12->ResourceState = NewState;
	}

	void FRHIDx12::FlushResourceStateChange()
	{
		FlushBarriers(CommandList.Get());
	}

	bool FRHIDx12::UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget)
	{
		FRenderTargetDx12 * RTDx12 = static_cast<FRenderTargetDx12*>(RenderTarget.get());
		// Create render target render resource tables
		int32 ColorBufferCount = RenderTarget->GetColorBufferCount();
		TI_ASSERT(RTDx12->RTColorTable == nullptr);
		if (ColorBufferCount > 0)
		{
			uint32 Mips = 0;
			// Validate mips in all color buffers
			for (int32 i = 0; i < ColorBufferCount; ++i)
			{
				const FRenderTarget::RTBuffer& ColorBuffer = RenderTarget->GetColorBuffer(i);
				TI_ASSERT(Mips == 0 || Mips == ColorBuffer.Texture->GetDesc().Mips);
				Mips = ColorBuffer.Texture->GetDesc().Mips;
			}
			TI_ASSERT(Mips > 0);
			RTDx12->RTColorTable = CreateRenderResourceTable(ColorBufferCount * Mips, EResourceHeapType::RenderTarget);
			for (int32 i = 0; i < ColorBufferCount; ++i)
			{
				const FRenderTarget::RTBuffer& ColorBuffer = RenderTarget->GetColorBuffer(i);
				RTDx12->RTColorTable->PutRTColorInTable(ColorBuffer.Texture, i);
			}
		}

		// Depth stencil buffers
		{
			const FRenderTarget::RTBuffer& DepthStencilBuffer = RenderTarget->GetDepthStencilBuffer();
			FTexturePtr DSBufferTexture = DepthStencilBuffer.Texture;
			if (DSBufferTexture != nullptr)
			{
				TI_ASSERT(RTDx12->RTDepthTable == nullptr);
				RTDx12->RTDepthTable = CreateRenderResourceTable(DSBufferTexture->GetDesc().Mips, EResourceHeapType::DepthStencil);
				RTDx12->RTDepthTable->PutRTDepthInTable(DSBufferTexture, 0);
			}
		}
		return true;
	}

	inline D3D12_SHADER_VISIBILITY GetVisibility(E_SHADER_STAGE Stage)
	{
		switch (Stage)
		{
		case ESS_VERTEX_SHADER:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ESS_PIXEL_SHADER:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case ESS_DOMAIN_SHADER:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ESS_HULL_SHADER:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ESS_GEOMETRY_SHADER:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		default:
			RuntimeFail();
			break;
		};
		return D3D12_SHADER_VISIBILITY_ALL;
	}

	inline int32 GetBindIndex(const D3D12_SHADER_INPUT_BIND_DESC& BindDesc, const D3D12_ROOT_SIGNATURE_DESC& RSDesc, E_SHADER_STAGE Stage)
	{
		if (BindDesc.Type == D3D_SIT_SAMPLER)
		{
			return -1;
		}

		// Find correct bind index in RootSignature
		for (uint32 i = 0; i < RSDesc.NumParameters; ++i)
		{
			const D3D12_ROOT_PARAMETER& Parameter = RSDesc.pParameters[i];
			// check for shader stage visibility
			if (Parameter.ShaderVisibility != GetVisibility(Stage))
			{
				continue;
			}
			switch (Parameter.ParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				const D3D12_ROOT_DESCRIPTOR_TABLE& DescriptorTable = Parameter.DescriptorTable;
				for (uint32 range = 0; range < DescriptorTable.NumDescriptorRanges; ++range)
				{
					const D3D12_DESCRIPTOR_RANGE& DescriptorRange = DescriptorTable.pDescriptorRanges[range];
					if (DescriptorRange.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
					{
						// If this param in the range of this descriptor table,
						// If NOT, try next table
						if (BindDesc.BindPoint >= DescriptorRange.BaseShaderRegister &&
							BindDesc.BindPoint < DescriptorRange.BaseShaderRegister + DescriptorRange.NumDescriptors)
						{
							TI_ASSERT(BindDesc.Type == D3D_SIT_TEXTURE);
							return (int32)i;
						}
					}
					else if (DescriptorRange.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
					{
						// If this param in the range of this descriptor table,
						// If NOT, try next table
						if (BindDesc.BindPoint >= DescriptorRange.BaseShaderRegister &&
							BindDesc.BindPoint < DescriptorRange.BaseShaderRegister + DescriptorRange.NumDescriptors)
						{
							TI_ASSERT(BindDesc.Type == D3D_SIT_CBUFFER);
							return (int32)i;
						}
					}
					else
					{
						// Not support yet.
						RuntimeFail();
					}
				}
			}
				break;
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			case D3D12_ROOT_PARAMETER_TYPE_SRV:
			case D3D12_ROOT_PARAMETER_TYPE_UAV:
			{
				// Not support yet.
				RuntimeFail();
			}
				break;
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			{
				const D3D12_ROOT_DESCRIPTOR& Descriptor = Parameter.Descriptor;
				if (BindDesc.Type == D3D_SIT_CBUFFER)
				{
					if (BindDesc.BindPoint == Descriptor.ShaderRegister)
					{
						return (int32)i;
					}
				}
			}
				break;
			}
		}

		// Not found correspond param bind index.
		RuntimeFail();
		return -1;
	}

	inline uint32 GetRSDescCrc(const D3D12_ROOT_SIGNATURE_DESC& RSDesc)
	{
		TStream RSData;
		RSData.Put(&RSDesc.NumParameters, sizeof(uint32));
		RSData.Put(&RSDesc.NumStaticSamplers, sizeof(uint32));
		RSData.Put(&RSDesc.Flags, sizeof(uint32));
		for (uint32 i = 0; i < RSDesc.NumParameters; ++i)
		{
			const D3D12_ROOT_PARAMETER& Parameter = RSDesc.pParameters[i];
			RSData.Put(&Parameter.ParameterType, sizeof(uint32));
			switch (Parameter.ParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				const D3D12_ROOT_DESCRIPTOR_TABLE& Table = Parameter.DescriptorTable;
				RSData.Put(&Table.NumDescriptorRanges, sizeof(uint32));
				for (uint32 d = 0; d < Table.NumDescriptorRanges; ++d)
				{
					RSData.Put(Table.pDescriptorRanges + d, sizeof(D3D12_DESCRIPTOR_RANGE));
				}
			}
			break;
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			{
				const D3D12_ROOT_CONSTANTS& Constants = Parameter.Constants;
				RSData.Put(&Constants, sizeof(D3D12_ROOT_CONSTANTS));
			}
			break;
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			case D3D12_ROOT_PARAMETER_TYPE_SRV:
			case D3D12_ROOT_PARAMETER_TYPE_UAV:
			{
				const D3D12_ROOT_DESCRIPTOR& Descriptor = Parameter.Descriptor;
				RSData.Put(&Descriptor, sizeof(D3D12_ROOT_DESCRIPTOR));
			}
			break;
			}
		}
		for (uint32 i = 0; i < RSDesc.NumStaticSamplers; ++i)
		{
			const D3D12_STATIC_SAMPLER_DESC& SamplerDesc = RSDesc.pStaticSamplers[i];
			RSData.Put(&SamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
		}
		return TCrc::MemCrc32(RSData.GetBuffer(), RSData.GetLength());
	}

	bool FRHIDx12::UpdateHardwareResourceShader(FShaderPtr ShaderResource, const TVector<TStreamPtr>& ShaderCodes)
	{
		TI_ASSERT(ShaderCodes.size() == ESS_COUNT);
		// Dx12 shader only need load byte code.
		FShaderDx12 * ShaderDx12 = static_cast<FShaderDx12*>(ShaderResource.get());

		ID3D12RootSignatureDeserializer * RSDeserializer = nullptr;
		if (ShaderResource->GetShaderType() == EST_COMPUTE ||
			ShaderResource->GetShaderType() == EST_SHADERLIB)
		{
			TStreamPtr ShaderCode = ShaderCodes[ESS_COMPUTE_SHADER];
			TI_ASSERT(ShaderCode != nullptr && ShaderCode->GetLength() > 0);

			ShaderDx12->ShaderCodes[0] = ShaderCode;
			if (RSDeserializer == nullptr)
			{
				VALIDATE_HRESULT(D3D12CreateRootSignatureDeserializer(ShaderCode->GetBuffer(),
					ShaderCode->GetLength(),
					__uuidof(ID3D12RootSignatureDeserializer),
					reinterpret_cast<void**>(&RSDeserializer)));
			}
		}
		else
		{
			for (int32 s = 0; s < ESS_COUNT; ++s)
			{
				TStreamPtr ShaderCode = ShaderCodes[s];
				//TString ShaderName = ShaderDx12->GetShaderName((E_SHADER_STAGE)s);
				if (ShaderCode != nullptr)
				{
					TI_ASSERT(ShaderCode->GetLength() > 0);

					ShaderDx12->ShaderCodes[s] = ShaderCode;
					if (RSDeserializer == nullptr)
					{
						VALIDATE_HRESULT(D3D12CreateRootSignatureDeserializer(ShaderCode->GetBuffer(),
							ShaderCode->GetLength(),
							__uuidof(ID3D12RootSignatureDeserializer),
							reinterpret_cast<void**>(&RSDeserializer)));
					}
				}
			}
		}

		// Create shader binding, also root signature in dx12
		TI_ASSERT(RSDeserializer != nullptr);
		const D3D12_ROOT_SIGNATURE_DESC* RSDesc = RSDeserializer->GetRootSignatureDesc();
		TI_ASSERT(ShaderDx12->ShaderBinding == nullptr);

		// Search for cached shader bindings
		uint32 RSDescKey = GetRSDescCrc(*RSDesc);
		if (ShaderBindingCache.find(RSDescKey) != ShaderBindingCache.end())
		{
			ShaderDx12->ShaderBinding = ShaderBindingCache[RSDescKey];
		}
		else
		{
			ShaderDx12->ShaderBinding = CreateShaderBinding(*RSDesc);
			ShaderBindingCache[RSDescKey] = ShaderDx12->ShaderBinding;

			if (ShaderResource->GetShaderType() == EST_RENDER)
			{
				// Analysis binding argument types
				for (int32 s = 0; s < ESS_COUNT; ++s)
				{
					TStreamPtr ShaderCode = ShaderCodes[s];
					if (ShaderCode != nullptr)
					{
						TI_ASSERT(ShaderCode->GetLength() > 0);
						THMap<int32, E_ARGUMENT_TYPE> BindingMap;	// Key is Binding Index, Value is ArgumentIndex in Arguments

						ID3D12ShaderReflection* ShaderReflection;
						VALIDATE_HRESULT(D3DReflect(ShaderCode->GetBuffer(), ShaderCode->GetLength(), IID_PPV_ARGS(&ShaderReflection)));

						D3D12_SHADER_DESC ShaderDesc;
						VALIDATE_HRESULT(ShaderReflection->GetDesc(&ShaderDesc));
						for (uint32 r = 0; r < ShaderDesc.BoundResources; ++r)
						{
							D3D12_SHADER_INPUT_BIND_DESC BindDescriptor;
							VALIDATE_HRESULT(ShaderReflection->GetResourceBindingDesc(r, &BindDescriptor));
							int32 BindIndex = GetBindIndex(BindDescriptor, *RSDesc, (E_SHADER_STAGE)s);
							if (BindIndex >= 0)
							{
								TString BindName = BindDescriptor.Name;
								E_ARGUMENT_TYPE ArgumentType = FShaderBinding::GetArgumentTypeByName(BindName);
								if (BindingMap.find(BindIndex) == BindingMap.end())
								{
									// Not binded, bind it
									ShaderDx12->ShaderBinding->AddShaderArgument(
										(E_SHADER_STAGE)s,
										FShaderBinding::FShaderArgument(BindIndex, ArgumentType));
									BindingMap[BindIndex] = ArgumentType;
								}
								else
								{
									TI_ASSERT(RSDesc->pParameters[BindIndex].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
									TI_ASSERT(BindingMap[BindIndex] == ArgumentType);
								}
							}
						}
					}
				}
				ShaderDx12->ShaderBinding->PostInitArguments();
			}
		}

		return true;
	}

	FShaderBindingPtr FRHIDx12::CreateShaderBinding(const D3D12_ROOT_SIGNATURE_DESC& RSDesc)
	{
		// Create new shader binding
		FShaderBindingPtr ShaderBinding = ti_new FRootSignatureDx12(RSDesc.NumParameters, RSDesc.NumStaticSamplers);
		FRootSignatureDx12 * RootSignatureDx12 = static_cast<FRootSignatureDx12*>(ShaderBinding.get());

		RootSignatureDx12->Finalize(D3dDevice.Get(), RSDesc);
		HoldResourceReference(ShaderBinding);
		
		return ShaderBinding;
	}

	// InShader and SpecifiedBindingIndex are used for Metal to create argument buffer, no use in dx12.
	bool FRHIDx12::UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex)
	{
		FArgumentBufferDx12 * ArgumentDx12 = static_cast<FArgumentBufferDx12*>(ArgumentBuffer.get());
		const TVector<FRenderResourcePtr>& Arguments = ArgumentBuffer->GetArguments();
		TI_ASSERT(ArgumentDx12->ResourceTable == nullptr && Arguments.size() > 0);

		ArgumentDx12->ResourceTable = CreateRenderResourceTable((uint32)Arguments.size(), EResourceHeapType::ShaderResource);
		for (int32 i = 0 ; i < (int32)Arguments.size() ; ++ i)
		{
			FRenderResourcePtr Arg = Arguments[i];
			if (Arg->GetResourceType() == ERenderResourceType::UniformBuffer)
			{
				FUniformBufferPtr ArgUB = static_cast<FUniformBuffer*>(Arg.get());
				ArgumentDx12->ResourceTable->PutConstantBufferInTable(ArgUB, i);
			}
			else if (Arg->GetResourceType() == ERenderResourceType::Texture)
			{
				FTexturePtr ArgTex = static_cast<FTexture*>(Arg.get());
				ArgumentDx12->ResourceTable->PutTextureInTable(ArgTex, i);
			}
			else
			{
				_LOG(ELog::Fatal, "Invalid resource type in Argument buffer.\n");
			}
		}

		return true;
	}

	bool FRHIDx12::UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GPUCommandSignature.get());

		// Find arguments in command signature
		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignature->GetCommandStructure();

		const uint32 ArgsCount = (uint32)CommandStructure.size();

		// Fill D3D12 INDIRECT ARGUMENT DESC
		D3D12_INDIRECT_ARGUMENT_DESC * ArgumentDescs = ti_new D3D12_INDIRECT_ARGUMENT_DESC[ArgsCount];
		uint32 ArgBytesStride = 0;
		bool bNeedRootSignature = false;
		GPUCommandSignatureDx12->ArgumentStrideOffset.resize(CommandStructure.size());
		for (uint32 i = 0 ; i < (uint32)CommandStructure.size() ; ++ i)
		{
			E_GPU_COMMAND_TYPE Command = CommandStructure[i];
			if (Command == GPU_COMMAND_SET_VERTEX_BUFFER)
			{
				// Vertex Buffer
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
				ArgumentDescs[i].VertexBuffer.Slot = 0;	// Vertex Buffer always has Slot 0
			}
			else if (Command == GPU_COMMAND_SET_INSTANCE_BUFFER)
			{
				// Instance Buffer
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
				ArgumentDescs[i].VertexBuffer.Slot = 1;	// Instance Buffer always has Slot 1
			}
			else if (Command == GPU_COMMAND_SET_INDEX_BUFFER)
			{
				// Index Buffer
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
			}
			else if (Command == GPU_COMMAND_DRAW_INDEXED)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			}
			else if (Command == GPU_COMMAND_DISPATCH)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
			}
			else if (Command == GPU_COMMAND_CONSTANT)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				ArgumentDescs[i].Constant.RootParameterIndex = i;
				ArgumentDescs[i].Constant.DestOffsetIn32BitValues = 0;
				ArgumentDescs[i].Constant.Num32BitValuesToSet = 4;	// Always use 4 components
				bNeedRootSignature = true;
			}
			else if (Command == GPU_COMMAND_CONSTANT_BUFFER)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
				ArgumentDescs[i].ConstantBufferView.RootParameterIndex = i;
				bNeedRootSignature = true;
			}
			else if (Command == GPU_COMMAND_SHADER_RESOURCE)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
				ArgumentDescs[i].ShaderResourceView.RootParameterIndex = i;
				bNeedRootSignature = true;
			}
			else if (Command == GPU_COMMAND_UNORDERED_ACCESS)
			{
				ArgumentDescs[i].Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
				ArgumentDescs[i].UnorderedAccessView.RootParameterIndex = i;
				bNeedRootSignature = true;
			}
			else
			{
				RuntimeFail();
			}
			GPUCommandSignatureDx12->ArgumentStrideOffset[i] = ArgBytesStride;
			ArgBytesStride += FGPUCommandSignatureDx12::GPU_COMMAND_STRIDE[Command];
		}
		GPUCommandSignatureDx12->CommandStrideInBytes = ArgBytesStride;

		D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
		CommandSignatureDesc.pArgumentDescs = ArgumentDescs;
		CommandSignatureDesc.NumArgumentDescs = ArgsCount;
		CommandSignatureDesc.ByteStride = ArgBytesStride;

		ID3D12RootSignature* RS = nullptr;
		if (bNeedRootSignature)
		{
			FShaderBindingPtr ShaderBinding = GPUCommandSignature->GetPipeline()->GetShader()->GetShaderBinding();
			FRootSignatureDx12* RenderSignature = static_cast<FRootSignatureDx12*>(ShaderBinding.get());
			RS = RenderSignature->Get();
		}
		VALIDATE_HRESULT(D3dDevice->CreateCommandSignature(&CommandSignatureDesc, RS, IID_PPV_ARGS(&GPUCommandSignatureDx12->CommandSignature)));
		DX_SETNAME(GPUCommandSignatureDx12->CommandSignature.Get(), GPUCommandSignature->GetResourceName());
		ti_delete[] ArgumentDescs;

		HoldResourceReference(GPUCommandSignature);
		return true;
	}

	void FRHIDx12::PutConstantBufferInHeap(FUniformBufferPtr InUniformBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBV = GetConstantBufferView(InUniformBuffer);
		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateConstantBufferView(&CBV, Descriptor);
	}

	void FRHIDx12::PutTextureInHeap(FTexturePtr InTexture, EResourceHeapType InHeapType, uint32 InHeapSlot)
	{
		FGPUResourcePtr GPUResource = InTexture->GetGPUResource();
		FGPUTextureDx12* TexDx12 = static_cast<FGPUTextureDx12*>(GPUResource.get());
		TI_ASSERT(TexDx12->Resource.Get() != nullptr);

		const TTextureDesc& Desc = InTexture->GetDesc();
		DXGI_FORMAT DxgiFormat = GetDxPixelFormat(Desc.Format);
		TI_ASSERT(DxgiFormat != DXGI_FORMAT_UNKNOWN);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DXGI_FORMAT DepthSrvFormat = GetDepthSrvFormat(DxgiFormat);
		if (DepthSrvFormat != DXGI_FORMAT_UNKNOWN)
		{
			// A depth texture
			SRVDesc.Format = DepthSrvFormat;
		}
		else
		{
			// This is not a depth related buffer, treat as a normal texture.
			SRVDesc.Format = DxgiFormat;
		}

		switch (Desc.Type)
		{
		case ETT_TEXTURE_2D:
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = Desc.Mips;
			break;
		case ETT_TEXTURE_3D:
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			SRVDesc.Texture3D.MipLevels = Desc.Mips;
			break;
		case ETT_TEXTURE_CUBE:
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			SRVDesc.TextureCube.MipLevels = Desc.Mips;
			break;
		default:
			// Not supported yet
			RuntimeFail();
			break;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateShaderResourceView(TexDx12->Resource.Get(), &SRVDesc, Descriptor);
	}

	void FRHIDx12::PutRWTextureInHeap(
		FTexturePtr InTexture, 
		uint32 InMipLevel, 
		EResourceHeapType InHeapType, 
		uint32 InHeapSlot)
	{
		FGPUResourcePtr GPUResource = InTexture->GetGPUResource();
		FGPUTextureDx12* TexDx12 = static_cast<FGPUTextureDx12*>(GPUResource.get());
		TI_ASSERT(TexDx12->Resource.Get() != nullptr);

		const TTextureDesc& Desc = InTexture->GetDesc();
		DXGI_FORMAT DxgiFormat = GetDxPixelFormat(Desc.Format);
		TI_ASSERT(DxgiFormat != DXGI_FORMAT_UNKNOWN);

		// Create unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format = GetUAVFormat(DxgiFormat);
		if (Desc.Type == ETT_TEXTURE_2D)
		{
			UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice = InMipLevel;
			UAVDesc.Texture2D.PlaneSlice = 0;
		}
		else if (Desc.Type == ETT_TEXTURE_3D)
		{
			UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			UAVDesc.Texture3D.MipSlice = InMipLevel;
			UAVDesc.Texture3D.FirstWSlice = 0;
			UAVDesc.Texture3D.WSize = Desc.Depth;
			TI_ASSERT(Desc.Depth > 1);
		}
		else
		{
			// Not supported yet.
			RuntimeFail();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);

		D3dDevice->CreateUnorderedAccessView(
			TexDx12->Resource.Get(),
			nullptr,
			&UAVDesc,
			Descriptor);
	}

	void FRHIDx12::PutUniformBufferInHeap(
		FUniformBufferPtr InBuffer, 
		EResourceHeapType InHeapType, 
		uint32 InHeapSlot)
	{
		// Create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.NumElements = InBuffer->GetElements();
		SRVDesc.Buffer.StructureByteStride = InBuffer->GetStructureSizeInBytes();
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InBuffer->GetGPUResource().get());
		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateShaderResourceView(BufferDx12->GetResource(), &SRVDesc, Descriptor);
	}

	void FRHIDx12::PutTopAccelerationStructureInHeap(
		FTopLevelAccelerationStructurePtr InTLAS, 
		EResourceHeapType InHeapType, 
		uint32 InHeapSlot)
	{
		FTopLevelAccelerationStructureDx12* TLASDx12 = static_cast<FTopLevelAccelerationStructureDx12*>(InTLAS.get());
		// Create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		SRVDesc.RaytracingAccelerationStructure.Location = TLASDx12->AccelerationStructure.Get()->GetGPUVirtualAddress();

		// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
		// When creating descriptor heap based acceleration structure SRVs, 
		// the resource parameter must be NULL, as the memory location comes 
		// as a GPUVA from the view description (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV) 
		// shown below. E.g. CreateShaderResourceView(NULL,pViewDesc).
		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateShaderResourceView(nullptr, &SRVDesc, Descriptor);
	}

	void FRHIDx12::PutRWUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot)
	{
		TI_ASSERT((InBuffer->GetFlag() & (uint32)EGPUResourceFlag::Uav) != 0);
		const bool HasCounter = (InBuffer->GetFlag() & (uint32)EGPUResourceFlag::UavCounter) != 0;
		// https://docs.microsoft.com/en-us/windows/win32/direct3d12/uav-counters
		// Create unordered access view
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.FirstElement = 0;
		UAVDesc.Buffer.NumElements = InBuffer->GetElements();
		UAVDesc.Buffer.StructureByteStride = InBuffer->GetStructureSizeInBytes();
		UAVDesc.Buffer.CounterOffsetInBytes = HasCounter ? GetUavCounterOffset(InBuffer->GetTotalBufferSize()) : 0;
		UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InBuffer->GetGPUResource().get());

		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateUnorderedAccessView(
			BufferDx12->GetResource(),
			HasCounter ? BufferDx12->GetResource() : nullptr,
			&UAVDesc,
			Descriptor);
	}
	
	void FRHIDx12::PutVertexBufferInHeap(FVertexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InVBHeapSlot)
	{
		FGPUBufferDx12* VBDx12 = static_cast<FGPUBufferDx12*>(InBuffer->GetGPUResource().get());

		// Create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		if (InVBHeapSlot >= 0)
		{
			TI_ASSERT(InBuffer->GetDesc().Stride % 4 == 0);
			SRVDesc.Buffer.NumElements = InBuffer->GetDesc().VertexCount * InBuffer->GetDesc().Stride / sizeof(float);
			SRVDesc.Buffer.StructureByteStride = sizeof(float);

			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InVBHeapSlot);
			D3dDevice->CreateShaderResourceView(VBDx12->GetResource(), &SRVDesc, Descriptor);
		}
	}

	void FRHIDx12::PutIndexBufferInHeap(FIndexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InIBHeapSlot)
	{
		FGPUBufferDx12* IBDx12 = static_cast<FGPUBufferDx12*>(InBuffer->GetGPUResource().get());

		// Create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		if (InIBHeapSlot >= 0)
		{
			TI_ASSERT(InBuffer->GetDesc().IndexType == EIT_32BIT);
			SRVDesc.Buffer.NumElements = InBuffer->GetDesc().IndexCount;
			SRVDesc.Buffer.StructureByteStride = sizeof(uint32);

			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InIBHeapSlot);
			D3dDevice->CreateShaderResourceView(IBDx12->GetResource(), &SRVDesc, Descriptor);
		}
	}

	void FRHIDx12::PutInstanceBufferInHeap(FInstanceBufferPtr InInstanceBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot)
	{
		FGPUBufferDx12* IBDx12 = static_cast<FGPUBufferDx12*>(InInstanceBuffer->GetGPUResource().get());

		// Create shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.NumElements = InInstanceBuffer->GetDesc().InstanceCount;
		SRVDesc.Buffer.StructureByteStride = InInstanceBuffer->GetDesc().Stride;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(InHeapType, InHeapSlot);
		D3dDevice->CreateShaderResourceView(IBDx12->GetResource(), &SRVDesc, Descriptor);
	}

	void FRHIDx12::PutRTColorInHeap(FTexturePtr InTexture, uint32 InHeapSlot)
	{
		FGPUResourcePtr GPUResource = InTexture->GetGPUResource();
		FGPUTextureDx12* TexDx12 = static_cast<FGPUTextureDx12*>(GPUResource.get());
		TI_ASSERT(TexDx12->Resource.Get() != nullptr);

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
		RTVDesc.Format = GetDxPixelFormat(InTexture->GetDesc().Format);
		TI_ASSERT(RTVDesc.Format != DXGI_FORMAT_UNKNOWN);
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.PlaneSlice = 0;

		const uint32 Mips = InTexture->GetDesc().Mips;
		for (uint32 Mip = 0 ; Mip < Mips ; ++ Mip)
		{
			RTVDesc.Texture2D.MipSlice = Mip;
			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(EResourceHeapType::RenderTarget, InHeapSlot + Mip);
			D3dDevice->CreateRenderTargetView(TexDx12->Resource.Get(), &RTVDesc, Descriptor);
		}
	}

	void FRHIDx12::PutRTDepthInHeap(FTexturePtr InTexture, uint32 InHeapSlot)
	{
		FGPUResourcePtr GPUResource = InTexture->GetGPUResource();
		FGPUTextureDx12* TexDx12 = static_cast<FGPUTextureDx12*>(GPUResource.get());
		TI_ASSERT(TexDx12->Resource.Get() != nullptr);

		DXGI_FORMAT DxgiFormat = GetDxPixelFormat(InTexture->GetDesc().Format);
		TI_ASSERT(DXGI_FORMAT_UNKNOWN != DxgiFormat);

		D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
		DsvDesc.Format = GetDSVFormat(DxgiFormat);
		DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		const uint32 Mips = InTexture->GetDesc().Mips;
		for (uint32 Mip = 0; Mip < Mips; ++Mip)
		{
			DsvDesc.Texture2D.MipSlice = Mip;
			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(EResourceHeapType::DepthStencil, InHeapSlot + Mip);
			D3dDevice->CreateDepthStencilView(TexDx12->Resource.Get(), &DsvDesc, Descriptor);
		}
	}

	void FRHIDx12::SetGraphicsPipeline(FPipelinePtr InPipeline)
	{
		if (CurrentBoundResource.Pipeline != InPipeline)
		{
			FPipelineDx12* PipelineDx12 = static_cast<FPipelineDx12*>(InPipeline.get());
			FShaderPtr Shader = InPipeline->GetShader();
			TI_ASSERT(Shader->GetShaderType() == EST_RENDER);
			FShaderBindingPtr ShaderBinding = Shader->GetShaderBinding();
			TI_ASSERT(ShaderBinding != nullptr);

			if (CurrentBoundResource.ShaderBinding != ShaderBinding)
			{
				FRootSignatureDx12 * RSDx12 = static_cast<FRootSignatureDx12*>(ShaderBinding.get());
				CommandList->SetGraphicsRootSignature(RSDx12->Get());
				CurrentBoundResource.ShaderBinding = ShaderBinding;
			}

			CommandList->SetPipelineState(PipelineDx12->PipelineState.Get());

			HoldResourceReference(InPipeline);

			CurrentBoundResource.Pipeline = InPipeline;
		}
	}

	void FRHIDx12::SetComputePipeline(FPipelinePtr InPipeline)
	{
		FPipelineDx12* PipelineDx12 = static_cast<FPipelineDx12*>(InPipeline.get());
		FShaderPtr Shader = InPipeline->GetShader();
		TI_ASSERT(Shader->GetShaderType() == EST_COMPUTE);
		FShaderBindingPtr ShaderBinding = Shader->GetShaderBinding();
		TI_ASSERT(ShaderBinding != nullptr);

		CommandList->SetPipelineState(PipelineDx12->PipelineState.Get());

		FRootSignatureDx12 * RSDx12 = static_cast<FRootSignatureDx12*>(ShaderBinding.get());
		CommandList->SetComputeRootSignature(RSDx12->Get());

		HoldResourceReference(InPipeline);
	}

	void FRHIDx12::SetVertexBuffer(FVertexBufferPtr InVB, FInstanceBufferPtr InInsB)
	{
		const TVertexBufferDesc& VBDesc = InVB->GetDesc();
		if (CurrentBoundResource.PrimitiveType != VBDesc.PrimitiveType)
		{
			CommandList->IASetPrimitiveTopology(GetDx12Topology(VBDesc.PrimitiveType));
			CurrentBoundResource.PrimitiveType = VBDesc.PrimitiveType;
		}

		D3D12_VERTEX_BUFFER_VIEW VBView = GetVertexBufferView(InVB);
		if (InInsB == nullptr)
		{
			CommandList->IASetVertexBuffers(0, 1, &VBView);
		}
		else
		{
			D3D12_VERTEX_BUFFER_VIEW InsView = GetInstanceBufferView(InInsB);
			D3D12_VERTEX_BUFFER_VIEW Views[2] =
			{
				VBView,
				InsView
			};
			CommandList->IASetVertexBuffers(0, 2, Views);
			HoldResourceReference(InInsB);
		}

		HoldResourceReference(InVB);
	}

	void FRHIDx12::SetIndexBuffer(FIndexBufferPtr InIB)
	{
		D3D12_INDEX_BUFFER_VIEW IBView = GetIndexBufferView(InIB);
		CommandList->IASetIndexBuffer(&IBView);
		HoldResourceReference(InIB);
	}

	void FRHIDx12::SetUniformBuffer(E_SHADER_STAGE , int32 BindIndex, FUniformBufferPtr InUniformBuffer)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetGraphicsRootConstantBufferView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress());

		HoldResourceReference(InUniformBuffer);
	}
	
	void FRHIDx12::SetComputeConstant(int32 BindIndex, const FUInt4& InValue)
	{
		CommandList->SetComputeRoot32BitConstants(BindIndex, 4, &InValue, 0);
	}

	void FRHIDx12::SetComputeConstant(int32 BindIndex, const FFloat4& InValue)
	{
		CommandList->SetComputeRoot32BitConstants(BindIndex, 4, &InValue, 0);
	}

	void FRHIDx12::SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetComputeRootConstantBufferView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress() + BufferOffset);

		HoldResourceReference(InUniformBuffer);
	}
	
	void FRHIDx12::SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetComputeRootShaderResourceView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress() + BufferOffset);

		HoldResourceReference(InUniformBuffer);
	}

	void FRHIDx12::SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Descriptor = GetGpuDescriptorHandle(RenderResourceTable->GetHeapType(), RenderResourceTable->GetStartIndex());
		CommandList->SetGraphicsRootDescriptorTable(BindIndex, Descriptor);

		HoldResourceReference(RenderResourceTable);
	}

	void FRHIDx12::SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Descriptor = GetGpuDescriptorHandle(RenderResourceTable->GetHeapType(), RenderResourceTable->GetStartIndex());
		CommandList->SetComputeRootDescriptorTable(BindIndex, Descriptor);

		HoldResourceReference(RenderResourceTable);
	}

	void FRHIDx12::SetComputeArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer)
	{
		FArgumentBufferDx12 * ArgDx12 = static_cast<FArgumentBufferDx12*>(InArgumentBuffer.get());
		SetComputeResourceTable(InBindIndex, ArgDx12->ResourceTable);
	}
	
	void FRHIDx12::SetComputeTexture(int32 BindIndex, FTexturePtr InTexture)
	{
		TI_ASSERT(0);
	}

	void FRHIDx12::DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount)
	{
		FlushResourceStateChange();
		CommandList->Dispatch(GroupCount.X, GroupCount.Y, GroupCount.Z);
	}

	void FRHIDx12::ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer)
	{
		if (GPUCommandBuffer->GetEncodedCommandsCount() > 0)
		{
			FlushResourceStateChange();
			FGPUCommandSignatureDx12 * SignatueDx12 = static_cast<FGPUCommandSignatureDx12*>(GPUCommandBuffer->GetGPUCommandSignature().get());

			// Set Primitive Topology
			CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Execute indirect draw.
			FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUCommandBuffer->GetGPUResource().get());
			if ((GPUCommandBuffer->GetCBFlag() & (uint32)EGPUResourceFlag::UavCounter) != 0)
			{
				uint32 CounterOffset = GetUavCounterOffset(GPUCommandBuffer->GetTotalBufferSize());
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					BufferDx12->GetResource(),
					CounterOffset);
			}
			else
			{
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					nullptr,
					0);
			}

			HoldResourceReference(GPUCommandBuffer);
		}
	}

	void FRHIDx12::ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer)
	{
		if (GPUCommandBuffer->GetEncodedCommandsCount() > 0)
		{
			FlushResourceStateChange();
			FGPUCommandBufferDx12 * GPUCommandBufferDx12 = static_cast<FGPUCommandBufferDx12*>(GPUCommandBuffer.get());
			FGPUCommandSignatureDx12 * SignatueDx12 = static_cast<FGPUCommandSignatureDx12*>(GPUCommandBuffer->GetGPUCommandSignature().get());

			// Execute indirect draw.
			FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUCommandBuffer->GetGPUResource().get());
			if ((GPUCommandBuffer->GetCBFlag() & (uint32)EGPUResourceFlag::UavCounter) != 0)
			{
				uint32 CounterOffset = GetUavCounterOffset(GPUCommandBuffer->GetTotalBufferSize());
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					BufferDx12->GetResource(),
					CounterOffset);
			}
			else
			{
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					nullptr,
					0);
			}

			HoldResourceReference(GPUCommandBuffer);
		}
	}

	void FRHIDx12::SetArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer)
	{
		TI_ASSERT(InBindIndex >= 0);
		FArgumentBufferDx12 * ArgDx12 = static_cast<FArgumentBufferDx12*>(InArgumentBuffer.get());
		SetRenderResourceTable(InBindIndex, ArgDx12->ResourceTable);
	}

	void FRHIDx12::SetStencilRef(uint32 InRefValue)
	{
		CommandList->OMSetStencilRef(InRefValue);
	}

	void FRHIDx12::DrawPrimitiveInstanced(uint32 VertexCount, uint32 InstanceCount, uint32 InstanceOffset)
	{
		FlushResourceStateChange();
		CommandList->DrawInstanced(VertexCount, InstanceCount, 0, 0);

		//FStats::Stats.TrianglesRendered += MeshBuffer->GetIndicesCount() / 3 * InstanceCount;
	}

	void FRHIDx12::DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 InstanceOffset)
	{
		FlushResourceStateChange();
		CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, 0, 0, InstanceOffset);

		FStats::Stats.TrianglesRendered += IndexCount / 3 * InstanceCount;
	}

	void FRHIDx12::DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 IndexOffset, uint32 InstanceOffset)
	{
		FlushResourceStateChange();
		CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, IndexOffset, 0, InstanceOffset);

		FStats::Stats.TrianglesRendered += IndexCount / 3 * InstanceCount;
	}

	void FRHIDx12::SetTilePipeline(FPipelinePtr InPipeline)
	{
		TI_ASSERT(0);
	}

	void FRHIDx12::SetTileBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer)
	{
		TI_ASSERT(0);
	}

	void FRHIDx12::DispatchTile(const FInt3& GroupSize)
	{
		TI_ASSERT(0);
	}

	void FRHIDx12::SetViewport(const FViewport& VP)
	{
		FRHI::SetViewport(VP);

		D3D12_VIEWPORT ViewportDx = { float(VP.Left), float(VP.Top), float(VP.Width), float(VP.Height), 0.f, 1.f };
		CommandList->RSSetViewports(1, &ViewportDx);
	}

	void FRHIDx12::SetRenderTarget(FRenderTargetPtr RT, uint32 MipLevel)
	{
		// Transition Color buffer to D3D12_RESOURCE_STATE_RENDER_TARGET
		FRenderTargetDx12 * RTDx12 = static_cast<FRenderTargetDx12*>(RT.get());
		const int32 CBCount = RT->GetColorBufferCount();
		uint32 RtMips = 0;
		for (int32 cb = 0; cb < CBCount; ++cb)
		{
			FTexturePtr Texture = RT->GetColorBuffer(cb).Texture;
			RtMips = Texture->GetDesc().Mips;
			TI_ASSERT(Texture != nullptr);	// Color can be NULL ?
			FGPUTexturePtr GPUTexture = static_cast<FGPUTexture*>(Texture->GetGPUResource().get());
			SetGPUTextureState(GPUTexture, EGPUResourceState::RenderTarget);
		}
		TI_ASSERT(CBCount == 0 || MipLevel < RtMips);

		// Transition Depth buffer to D3D12_RESOURCE_STATE_DEPTH_WRITE
		{
			FTexturePtr Texture = RT->GetDepthStencilBuffer().Texture;
			if (Texture != nullptr)
			{
				TI_ASSERT(CBCount == 0 || Texture->GetDesc().Mips == RtMips);
				RtMips = Texture->GetDesc().Mips;
				FGPUTexturePtr GPUTexture = static_cast<FGPUTexture*>(Texture->GetGPUResource().get());
				SetGPUTextureState(GPUTexture, EGPUResourceState::DepthWrite);
			}
		}
		FlushBarriers(CommandList.Get());

		TVector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVDescriptors;
		const D3D12_CPU_DESCRIPTOR_HANDLE* Rtv = nullptr;
		if (CBCount > 0)
		{
			FRenderResourceTablePtr ColorTable = RTDx12->RTColorTable;
			RTVDescriptors.reserve(CBCount); 
			for (int32 cb = 0 ; cb < CBCount ; ++ cb)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = GetCpuDescriptorHandle(EResourceHeapType::RenderTarget, ColorTable->GetIndexAt(cb * RtMips + MipLevel));
				RTVDescriptors.push_back(Descriptor);
			}
			Rtv = RTVDescriptors.data();
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* Dsv = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthDescriptor;
		FRenderResourceTablePtr DepthTable = RTDx12->RTDepthTable;
		if (DepthTable != nullptr && DepthTable->GetTableSize() != 0)
		{
			DepthDescriptor = GetCpuDescriptorHandle(EResourceHeapType::DepthStencil, DepthTable->GetIndexAt(MipLevel));
			Dsv = &DepthDescriptor;
		}

		// Set render target
		CommandList->OMSetRenderTargets(RT->GetColorBufferCount(), Rtv, false, Dsv);

		// Clear render target
		if (CBCount > 0)
		{
			for (int32 cb = 0; cb < CBCount; ++cb)
			{
				FTexturePtr Texture = RT->GetColorBuffer(cb).Texture;
				CommandList->ClearRenderTargetView(RTVDescriptors[cb], Texture->GetDesc().ClearColor.GetDataPtr(), 0, nullptr);
			}
		}
		if (Dsv != nullptr)
		{
			CommandList->ClearDepthStencilView(*Dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
	}

	void FRHIDx12::BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName, uint32 MipLevel)
	{
		END_EVENT(CommandList.Get());
		FRHI::BeginRenderToRenderTarget(RT, PassName, MipLevel);
		BEGIN_EVENT(CommandList.Get(), PassName);
		SetRenderTarget(RT, MipLevel);
	}

	void FRHIDx12::InitRHIRenderResourceHeap(EResourceHeapType Heap, uint32 HeapSize, uint32 HeapOffset)
	{
		int32 HeapIndex = static_cast<int32>(Heap);
		TI_ASSERT(HeapIndex >= 0 && HeapIndex < NumResourceHeapTypes);
		RenderResourceHeap[HeapIndex].Create(Heap, HeapSize, HeapOffset);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FRHIDx12::GetCpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex)
	{
		D3D12_DESCRIPTOR_HEAP_TYPE Dx12Heap = GetDxHeapTypeFromTiXHeap(Heap);
		return DescriptorHeaps[Dx12Heap].GetCpuDescriptorHandle(SlotIndex);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE FRHIDx12::GetGpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex)
	{
		D3D12_DESCRIPTOR_HEAP_TYPE Dx12Heap = GetDxHeapTypeFromTiXHeap(Heap);
		return DescriptorHeaps[Dx12Heap].GetGpuDescriptorHandle(SlotIndex);
	}

	void FRHIDx12::CreateD3D12Resource(
		D3D12_HEAP_TYPE HeapType,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		ComPtr<ID3D12Resource>& OutResource
	)
	{
		CD3DX12_HEAP_PROPERTIES HeapProperties(HeapType);
		VALIDATE_HRESULT(D3dDevice->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			pDesc,
			InitState,
			pOptimizedClearValue,
			IID_PPV_ARGS(&OutResource)));
	}

	void FRHIDx12::UpdateD3D12Resource(
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		uint32 NumSubResources,
		const D3D12_SUBRESOURCE_DATA* pData
	)
	{
		UpdateSubresources(
			CommandList.Get(),
			pDestinationResource,
			pIntermediate,
			0, 0, NumSubResources,
			pData
		);
	}

	uint64 FRHIDx12::GetRequiredIntermediateSize(
		const D3D12_RESOURCE_DESC& Desc,
		uint32 FirstSubresource,
		uint32 NumSubresources)
	{
		uint64 RequiredSize = 0;
		D3dDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);

		return RequiredSize;
	}

	uint32 FRHIDx12::GetUavCounterOffset(uint32 InBufferSize)
	{
		// With counter, counter offset must be aligned with D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT
		return TMath::Align(InBufferSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
	}

	uint32 FRHIDx12::GetUavSizeWithCounter(uint32 InBufferSize)
	{
		uint32 BufferSizeWithCounter;
		// Alloc FUint4 for counter, since some shader need to access it.
		BufferSizeWithCounter = GetUavCounterOffset(InBufferSize) + sizeof(FUInt4);

		return BufferSizeWithCounter;
	}

	D3D12_VERTEX_BUFFER_VIEW FRHIDx12::GetVertexBufferView(FVertexBufferPtr VB)
	{
		D3D12_VERTEX_BUFFER_VIEW VBView;
		VBView.BufferLocation = GetGPUBufferGPUAddress(VB->GetGPUResource());
		VBView.SizeInBytes = VB->GetDesc().VertexCount * VB->GetDesc().Stride;
		VBView.StrideInBytes = VB->GetDesc().Stride;
		return VBView;
	}

	D3D12_VERTEX_BUFFER_VIEW FRHIDx12::GetInstanceBufferView(FInstanceBufferPtr IB)
	{
		D3D12_VERTEX_BUFFER_VIEW VBView;
		VBView.BufferLocation = GetGPUBufferGPUAddress(IB->GetGPUResource());
		VBView.SizeInBytes = IB->GetDesc().InstanceCount * IB->GetDesc().Stride;
		VBView.StrideInBytes = IB->GetDesc().Stride;
		return VBView;
	}

	D3D12_INDEX_BUFFER_VIEW FRHIDx12::GetIndexBufferView(FIndexBufferPtr IB)
	{
		D3D12_INDEX_BUFFER_VIEW IBView;
		IBView.BufferLocation = GetGPUBufferGPUAddress(IB->GetGPUResource());
		IBView.SizeInBytes = IB->GetDesc().IndexCount * (IB->GetDesc().IndexType == EIT_16BIT ? 2 : 4);
		IBView.Format = IB->GetDesc().IndexType == EIT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		return IBView;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC FRHIDx12::GetConstantBufferView(FUniformBufferPtr UB)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBView;
		CBView.BufferLocation = GetGPUBufferGPUAddress(UB->GetGPUResource());
		CBView.SizeInBytes = UB->GetTotalBufferSize();
		return CBView;
	}

	D3D12_GPU_VIRTUAL_ADDRESS FRHIDx12::GetGPUBufferGPUAddress(FGPUResourcePtr GPUBuffer)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUBuffer.get());
		return BufferDx12->GetResource()->GetGPUVirtualAddress();
	}
	D3D12_GPU_VIRTUAL_ADDRESS FRHIDx12::GetGPUTextureGPUAddress(FGPUResourcePtr GPUTexture)
	{
		FGPUTextureDx12* TextureDx12 = static_cast<FGPUTextureDx12*>(GPUTexture.get());
		return TextureDx12->GetResource()->GetGPUVirtualAddress();
	}
}
#endif	// COMPILE_WITH_RHI_DX12