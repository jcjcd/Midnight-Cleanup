//--------------------------------------------------------------------------------------
// File: DebugDraw.h
//
// Helpers for drawing various debug shapes using PrimitiveBatch
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <./directxtk/PrimitiveBatch.h>
#include <./directxtk/VertexTypes.h>


namespace DebugDraw
{
	void XM_CALLCONV Draw(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		const DirectX::BoundingSphere& sphere,
		DirectX::FXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV Draw(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		const DirectX::BoundingBox& box,
		DirectX::FXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV Draw(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		const DirectX::BoundingOrientedBox& obb,
		DirectX::FXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV Draw(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		const DirectX::BoundingFrustum& frustum,
		DirectX::FXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV DrawGrid(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis,
		DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs,
		DirectX::GXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV DrawRing(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		DirectX::FXMVECTOR origin, DirectX::FXMVECTOR majorAxis, DirectX::FXMVECTOR minorAxis,
		DirectX::GXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV DrawRay(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		DirectX::FXMVECTOR origin, DirectX::FXMVECTOR direction, bool normalize = true,
		DirectX::FXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV DrawTriangle(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		DirectX::FXMVECTOR pointA, DirectX::FXMVECTOR pointB, DirectX::FXMVECTOR pointC,
		DirectX::GXMVECTOR color = DirectX::Colors::White);

	void XM_CALLCONV DrawQuad(DirectX::DX11::PrimitiveBatch<DirectX::DX11::VertexPositionColor>* batch,
		DirectX::FXMVECTOR pointA, DirectX::FXMVECTOR pointB, DirectX::FXMVECTOR pointC, DirectX::GXMVECTOR pointD,
		DirectX::HXMVECTOR color = DirectX::Colors::White);
}