#pragma once
#ifndef __MESH_H__
#define __MESH_H__

#include <DirectXCollision.h>
#include "Graphics/Resource/GpuBuffer.h"

namespace DSM {
	struct Material;

	enum PSOFlags : std::uint16_t
	{
		kHasPosition = ( 1 << 0 ),
		kHasNormal = ( 1 << 1 ),
		kHasTangent = ( 1 << 2 ),
		kHasUV = ( 1 << 3 ),
		kAlphaBlend = ( 1 << 4 ),
		kAlphaTest = ( 1 << 5 ),
		kBothSide = ( 1 << 6 ),
	};

	struct Mesh
	{
		std::string m_Name;
		
		DirectX::BoundingBox m_BoundingBox;
		// 设置顶点缓冲区使用的数据
		D3D12_VERTEX_BUFFER_VIEW m_PositionStream;
		D3D12_VERTEX_BUFFER_VIEW m_NormalStream;
		D3D12_VERTEX_BUFFER_VIEW m_UVStream;
		D3D12_VERTEX_BUFFER_VIEW m_TangentStream;
		// 索引缓冲区使用的数据
		D3D12_INDEX_BUFFER_VIEW m_IndexBufferViews;
		std::uint16_t m_PSOFlags;
		std::uint16_t m_PSOIndex;

		// 每次绘制需要使用的数据
		struct SubMesh
		{
			std::uint32_t m_IndexCount;
			std::uint32_t m_IndexOffset;
			std::uint32_t m_VertexOffset;
			std::uint16_t m_MaterialIndex;
			// 使用的纹理在描述符堆中的偏移
			std::uint16_t m_SRVTableOffset;
		};
		std::map<std::string, SubMesh> m_SubMeshes;

		GpuBuffer m_MeshData{};
	};
	
	


}

#endif
