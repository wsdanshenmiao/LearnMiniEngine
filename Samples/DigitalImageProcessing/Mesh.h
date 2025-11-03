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
		uint16_t m_PSOFlags;
		uint16_t m_PSOIndex;

		// 每次绘制需要使用的数据
		struct SubMesh
		{
			std::string m_Name;
			uint32_t m_IndexCount;
			uint32_t m_IndexOffset;
			uint32_t m_VertexCount;
			uint32_t m_VertexOffset;
			uint16_t m_MaterialIndex;
			// 使用的纹理在描述符堆中的偏移
			uint16_t m_SRVTableOffset;
		};
		std::vector<SubMesh> m_SubMeshes;

		GpuBuffer m_MeshData{};
	};
	
	


}

#endif
