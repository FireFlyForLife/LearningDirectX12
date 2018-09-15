#pragma once
#include <DIrectXMath.h>
#include <vector>
#include <RootSignature.h>
#include <VertexBuffer.h>

using namespace DirectX;

class DebugRenderer
{
	struct ColoredPoint
	{
		XMFLOAT4 color;
		XMFLOAT3 position;
	};

public:
	DebugRenderer();

	void Initialize(class RenderTarget& renderTarget, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect);

	void AddLine(XMFLOAT3 positionFrom, XMFLOAT3 positionTo, XMFLOAT4 color);


	void Render(const class Camera& camera);
	void ClearLines();

private:
	std::vector<std::pair<ColoredPoint, ColoredPoint>> lines;
	RootSignature debugRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> debugDrawPipelineState;
	VertexBuffer vertexBuffer{L"Debug Line points"};

	//Main renderer data
	class RenderTarget* m_RenderTarget;
	D3D12_VIEWPORT m_Viewport; 
	D3D12_RECT m_ScissorRect;
};