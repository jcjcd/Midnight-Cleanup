#pragma once
#include "RaytracingHlslCompat.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"

class ChangDXII;


namespace GpuKernels
{
	class ReduceSum
	{
	public:
		enum Type {
			Uint = 0,
			Float
		};

		void Initialize(ID3D12Device5* device, Type type);
		void CreateInputResourceSizeDependentResources(
			ChangDXII* renderer,
			UINT frameCount,
			UINT width,
			UINT height,
			UINT numInvocationsPerFrame);
		void Run(
			ID3D12GraphicsCommandList4* commandList,
			ID3D12DescriptorHeap* descriptorHeap,
			UINT frameIndex,
			D3D12_GPU_DESCRIPTOR_HANDLE inputResourceHandle,
			void* resultSum,
			UINT invocationIndex = 0);

	private:
		Type                                m_resultType;
		UINT                                m_resultSize;
		winrt::com_ptr<ID3D12RootSignature>         m_rootSignature;
		winrt::com_ptr<ID3D12PipelineState>         m_pipelineStateObject;
		std::vector<std::shared_ptr<DX12Texture>>			m_csReduceSumOutputs;
		std::vector<winrt::com_ptr<ID3D12Resource>>	m_readbackResources;
	};

	class DownsampleGBufferDataBilateralFilter
	{
	public:
		void Initialize(ID3D12Device5* device, UINT frameCount, UINT numCallsPerFrame = 1);
		void Run(
			ID3D12GraphicsCommandList4* commandList,
			UINT width,
			UINT height,
			ID3D12DescriptorHeap* descriptorHeap,
			D3D12_GPU_DESCRIPTOR_HANDLE inputNormalResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputPositionResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputPartialDistanceDerivativesResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputMotionVectorResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputPrevFrameHitPositionResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputDepthResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputSurfaceAlbedoResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputNormalResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputPositionResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputPartialDistanceDerivativesResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputMotionVectorResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputPrevFrameHitPositionResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputDepthResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputSurfaceAlbedoResourceHandle);

	private:
		winrt::com_ptr<ID3D12RootSignature>         m_rootSignature;
		winrt::com_ptr<ID3D12PipelineState>         m_pipelineStateObject;
		ConstantBuffer<TextureDimConstantBuffer> m_CB;
		UINT                                m_CBinstanceID = 0;
	};

	class UpsampleBilateralFilter
	{
	public:
		enum FilterType {
			Filter2x2FloatR = 0,
			Filter2x2UintR,
			Filter2x2FloatRG,
			Count
		};

		void Initialize(ID3D12Device5* device, UINT frameCount, UINT numCallsPerFrame = 1);
		void Run(
			ID3D12GraphicsCommandList4* commandList,
			UINT width,
			UINT height,
			FilterType type,
			ID3D12DescriptorHeap* descriptorHeap,
			D3D12_GPU_DESCRIPTOR_HANDLE inputResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputLowResNormalResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputHiResNormalResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE inputHiResPartialDistanceDerivativeResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputResourceHandle);

	private:
		winrt::com_ptr<ID3D12RootSignature>         m_rootSignature;
		winrt::com_ptr<ID3D12PipelineState>         m_pipelineStateObjects[FilterType::Count];
		ConstantBuffer<DownAndUpsampleFilterConstantBuffer> m_CB;
		UINT                                m_CBinstanceID = 0;
	};

	class CalculatePartialDerivatives
	{
	public:
		void Initialize(ID3D12Device5* device, UINT frameCount, UINT numCallsPerFrame = 1);
		void Run(
			ID3D12GraphicsCommandList4* commandList,
			ID3D12DescriptorHeap* descriptorHeap,
			UINT width,
			UINT height,
			D3D12_GPU_DESCRIPTOR_HANDLE inputResourceHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE outputResourceHandle);

	private:
		winrt::com_ptr<ID3D12RootSignature>         m_rootSignature;
		winrt::com_ptr<ID3D12PipelineState>         m_pipelineStateObject;
		ConstantBuffer<AtrousWaveletTransformFilterConstantBuffer> m_CB;
		UINT                                m_CBinstanceID = 0;
	};

// 	class GenerateGrassPatch
// 	{
// 	public:
// 		void Initialize(ID3D12Device5* device, const wchar_t* windTexturePath, DX::DescriptorHeap* descriptorHeap, ResourceUploadBatch* resourceUpload, UINT frameCount, UINT numCallsPerFrame = 1);
// 		void Run(
// 			ID3D12GraphicsCommandList4* commandList,
// 			const GenerateGrassStrawsConstantBuffer_AppParams& appParams,
// 			ID3D12DescriptorHeap* descriptorHeap,
// 			D3D12_GPU_DESCRIPTOR_HANDLE outputVertexBufferResourceHandle);
// 
// 		UINT GetVertexBufferSize(UINT grassStrawsX, UINT grassStrawsY)
// 		{
// 			return grassStrawsX * grassStrawsY * N_GRASS_VERTICES * sizeof(VertexPositionNormalTextureTangent);
// 		}
// 
// 	private:
// 		winrt::com_ptr<ID3D12RootSignature>         m_rootSignature;
// 		winrt::com_ptr<ID3D12PipelineState>         m_pipelineStateObject;
// 
// 		ConstantBuffer<GenerateGrassStrawsConstantBuffer> m_CB;
// 		UINT                                m_CBinstanceID = 0;
// 		D3DTexture                          m_windTexture;
// 	};
}

