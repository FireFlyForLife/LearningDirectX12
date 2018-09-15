#include <DebugRenderer.h>
#include <Application.h>
#include <d3dcompiler.h>
#include <iostream>
#include <Windows.h>
#include <d3dx12.h>
#include <Helpers.h>
#include <Mesh.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include "Camera.h"
#include <RenderTarget.h>

DebugRenderer::DebugRenderer()
{
	
}

void DebugRenderer::Initialize(RenderTarget& renderTarget, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect) { 

//	: m_RenderTarget(&renderTarget), m_Viewport(viewport), m_ScissorRect(scissorRect)
	m_RenderTarget = &renderTarget;
	m_Viewport = viewport;
	m_ScissorRect = scissorRect;

	using namespace Microsoft::WRL;

	ComPtr<ID3DBlob> errororor;

	ComPtr<ID3DBlob > debugVertexShaderBlob;
	if(FAILED( D3DCompileFromFile(L"Tutorial3/shaders/DebugRenderer.VS.hlsl", nullptr, nullptr, /*nullptr*/"main", "vs_5_1", D3DCOMPILE_DEBUG, 0, &debugVertexShaderBlob, &errororor)))
	{
		if (errororor) {
			auto* cstr = static_cast<const char*>(errororor->GetBufferPointer());
			std::cout << cstr << '\n';
			MessageBox(NULL, cstr, "Failed vertex shader compile", MB_OK);
		}else {
			std::cout << "D3DCompileFromFile failed to compile vertex shader. no error message\n";
		}
	}

	ComPtr<ID3DBlob > debugPixelShaderBlob;
	if (FAILED(D3DCompileFromFile(L"Tutorial3/shaders/DebugRenderer.PS.hlsl", nullptr, nullptr, /*nullptr*/"main", "ps_5_1", D3DCOMPILE_DEBUG, 0, &debugPixelShaderBlob, &errororor))) {
		if (errororor) {
			auto* cstr = static_cast<const char*>(errororor->GetBufferPointer());
			std::cout << cstr << '\n';
			MessageBox(NULL, cstr, "Failed pixel shader compile", MB_OK);
		}else {
			std::cout << "D3DCompileFromFile failed to compile pixel shader. no error message\n";
		}
	}

	ComPtr<ID3D12Device2> device = Application::Get().GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,  1, 2); 

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	// rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	// rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	// rootParameters[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	// rootParameters[RootParameters::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	// rootParameters[RootParameters::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	// rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRage, D3D12_SHADER_VISIBILITY_PIXEL);
	//
	// CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
	// CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);
	//
	// CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	// rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);
	//
	// m_RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);
	CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
	CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(1, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

	debugRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	// Setup the pipeline state.
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
	} pipelineStateStream;

	// sRGB formats provide free gamma correction!
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	// Check the best multisample quality level that can be used for the given back buffer format.
	DXGI_SAMPLE_DESC sampleDesc = Application::Get().GetMultisampleQualityLevels(backBufferFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	D3D12_INPUT_ELEMENT_DESC inputLayout[2]{
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	pipelineStateStream.pRootSignature = debugRootSignature.GetRootSignature().Get(); 
	pipelineStateStream.InputLayout = { inputLayout, 2 };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(debugVertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(debugPixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = sampleDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&debugDrawPipelineState)));

	puts("dwasd");
}

void DebugRenderer::AddLine(XMFLOAT3 positionFrom, XMFLOAT3 positionTo, XMFLOAT4 color)
{
	lines.push_back({ {color, positionFrom}, {color, positionTo} });
}

void DebugRenderer::Render(const Camera& camera) 
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();


	commandList->SetPipelineState(debugDrawPipelineState);
	commandList->SetGraphicsRootSignature(debugRootSignature);

	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);

	commandList->SetRenderTarget(*m_RenderTarget);

	commandList->CopyVertexBuffer(vertexBuffer, lines.size() * 2, sizeof(ColoredPoint), lines.data());

	//TODO: Calculate MVP
	XMMATRIX p_mat = camera.get_ProjectionMatrix();
	XMMATRIX v_mat = camera.get_ViewMatrix();
	XMMATRIX m_mat = XMMatrixIdentity();

	XMMATRIX mvp_row = m_mat * v_mat * p_mat;
	// XMMATRIX mvp_col = XMMatrixTranspose(mvp_row);
	struct Matrices
	{
		XMMATRIX ModelViewProjectionMatrix;
	};
	Matrices mat{ mvp_row };
	// commandList->SetGraphics32BitConstants(0, 16, &mvp_row);
	commandList->SetGraphicsDynamicConstantBuffer(0, mat);

	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->SetVertexBuffer(0, vertexBuffer);

	commandList->Draw(static_cast<uint32_t>(lines.size() * 2));

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void DebugRenderer::ClearLines()
{
	lines.clear(); 
	//TODO: Clear VBO 
}


