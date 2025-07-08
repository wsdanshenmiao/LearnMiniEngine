#include "ModelLoader.h"
#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "Geometry.h"
#include "Material.h"
#include "Renderer.h"
#include "Graphics/CommandList/CommandList.h"
#include "Graphics/GraphicsCommon.h"
#include <filesystem>

#include "ConstantData.h"

using namespace DirectX;

namespace DSM {
	
	struct MeshData
	{
		std::string m_Name{};
		std::vector<XMFLOAT3> m_Positions{};
		std::vector<XMFLOAT3> m_Normals{};
		std::vector<XMFLOAT2> m_Texcoords{};
		std::vector<XMFLOAT4> m_Tangents{};
		std::vector<XMFLOAT3> m_Bitangents{};
		std::vector<std::uint32_t> m_Indices{};
		BoundingBox m_BoundingBox;
		std::uint32_t m_MaterialIndex = 0;
		std::uint16_t m_PSOFlags;
	};

	
    void ProcessNode(Model& model, aiNode* node, const aiScene* scene);
    void ProcessMaterial(Model& model,const std::string& filename,const aiScene* scene);
    MeshData ProcessMesh(aiMesh* mesh);
    void CreateMesh(Mesh& mesh, const std::span<MeshData>& meshDatas);

	
	std::shared_ptr<Model> LoadModelFromeGeometry(const std::string& name, const Geometry::GeometryMesh& geometryMesh)
	{
		if (geometryMesh.m_Vertices.empty()){
			return nullptr;
		}

		auto model = std::make_shared<Model>();
		model->m_Name = name;
		model->m_Materials.emplace_back(std::make_shared<Material>());
		auto& mesh = model->m_Meshes.emplace_back(std::make_shared<Mesh>());
		mesh->m_Name = name;

		MeshData meshData{};
		meshData.m_Indices = geometryMesh.m_Indices32;
		meshData.m_Name = name;
		meshData.m_MaterialIndex = 0;
		meshData.m_BoundingBox = BoundingBox{};
		meshData.m_PSOFlags |= kHasPosition | kHasNormal | kHasTangent | kHasUV;
		for (const auto& vertex : geometryMesh.m_Vertices) {
			meshData.m_Positions.push_back(vertex.m_Position);
			meshData.m_Normals.push_back(vertex.m_Normal);
			meshData.m_Texcoords.push_back(vertex.m_TexCoord);
			meshData.m_Tangents.push_back(vertex.m_Tangent);
			meshData.m_Bitangents.push_back(vertex.m_BiTangent);
		}

		CreateMesh(*mesh, {&meshData, 1});

		model->m_BoundingBox = mesh->m_BoundingBox;
		
		return model;
	}

	std::shared_ptr<Model> LoadModel(const std::string& filename)
	{
		auto model = std::make_shared<Model>();
		
		Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(
			filename,
			aiProcess_ConvertToLeftHanded |     // 转为左手系
			aiProcess_GenBoundingBoxes |        // 获取碰撞盒
			aiProcess_Triangulate |             // 将多边形拆分
			aiProcess_ImproveCacheLocality |    // 改善缓存局部性
			aiProcess_SortByPType);             // 按图元顶点数排序用于移除非三角形图元

		if (nullptr == pScene || !pScene->HasMeshes()) {
			std::string warning = "[Warning]: Failed to load \"";
			warning += filename;
			warning += "\"\n";
			OutputDebugStringA(warning.c_str());
			return nullptr;
		}

		ProcessNode(*model, pScene->mRootNode, pScene);
		ProcessMaterial(*model, filename, pScene);

		model->m_BoundingBox = BoundingBox{{0,0,0}, {0,0,0}};
		model->m_Name = pScene->mRootNode->mName.C_Str();
		for (const auto& mesh : model->m_Meshes) {
			BoundingBox::CreateMerged(model->m_BoundingBox, model->m_BoundingBox, mesh->m_BoundingBox);
		}

		return model;
	}

	void ProcessNode(Model& model, aiNode* node, const aiScene* scene)
	{
		// 导入当前节点的网格
		auto mesh = std::make_shared<Mesh>();
		mesh->m_Name = node->mName.C_Str();
		std::vector<MeshData> meshDatas{};
		meshDatas.reserve(node->mNumMeshes);
		for (UINT i = 0; i < node->mNumMeshes; ++i) {
			meshDatas.emplace_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]]));
		}

		if (!meshDatas.empty()) {
			CreateMesh(*mesh, meshDatas);
			model.m_Meshes.push_back(std::move(mesh));
		}

		// 导入子节点的网格
		for (UINT i = 0; i < node->mNumChildren; ++i) {
			ProcessNode(model, node->mChildren[i], scene);
		}
	}

	MeshData ProcessMesh(aiMesh* mesh)
	{
		MeshData meshData{};
		meshData.m_Name = mesh->mName.C_Str();

		// 获取顶点数据
		ASSERT(mesh->HasPositions());
		meshData.m_Positions.resize(mesh->mNumVertices);
		meshData.m_PSOFlags |= kHasPosition;
		if (mesh->HasNormals()) {
			meshData.m_Normals.resize(mesh->mNumVertices);
			meshData.m_PSOFlags |= kHasNormal;
		}
		if (mesh->HasTextureCoords(0)) {
			meshData.m_Texcoords.resize(mesh->mNumVertices);
			meshData.m_PSOFlags |= kHasUV;
		}
		if (mesh->HasTangentsAndBitangents()) {
			meshData.m_Tangents.resize(mesh->mNumVertices);
			meshData.m_Bitangents.resize(mesh->mNumVertices);
			meshData.m_PSOFlags |= kHasTangent;
		}
		for (UINT i = 0; i < mesh->mNumVertices; ++i) {
			meshData.m_Positions[i] = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
			if (!meshData.m_Normals.empty()) {
				meshData.m_Normals[i] = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
			}
			if (!meshData.m_Texcoords.empty()) {
				meshData.m_Texcoords[i] = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
			}
			if (!meshData.m_Tangents.empty()) {
				meshData.m_Tangents[i] = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f};
				meshData.m_Bitangents[i] = {meshData.m_Bitangents[i].x, meshData.m_Bitangents[i].y, meshData.m_Bitangents[i].z};
			}
		}

		// 获取索引
		auto numIndex = mesh->mFaces->mNumIndices;
		meshData.m_Indices.resize(mesh->mNumFaces * numIndex);
		for (size_t i = 0; i < mesh->mNumFaces; ++i) {
			memcpy(meshData.m_Indices.data() + i * numIndex, mesh->mFaces[i].mIndices, sizeof(uint32_t) * numIndex);
		}

		const auto& AABB = mesh->mAABB;
		BoundingBox::CreateFromPoints(
			meshData.m_BoundingBox,
			XMVECTOR{ AABB.mMin.x, AABB.mMin.y, AABB.mMin.z, 1 },
			XMVECTOR{ AABB.mMax.x, AABB.mMax.y, AABB.mMax.z, 1 });

		meshData.m_MaterialIndex = mesh->mMaterialIndex;

		return meshData;
	}

	void CreateMesh(Mesh& mesh, const std::span<MeshData>& meshDatas)
	{
		if (meshDatas.empty()) return;
		
		std::vector<XMFLOAT3> positions{};
		std::vector<XMFLOAT3> normals{};
		std::vector<XMFLOAT2> uvs{};
		std::vector<XMFLOAT4> tangents{};
		std::vector<std::uint32_t> indices{};
		
		mesh.m_BoundingBox = BoundingBox{{0,0,0},{0,0,0}};

		UINT preIndexCount = 0;
		UINT preVertexCount = 0;
		for (const auto& meshData : meshDatas) {
			auto indexCount = meshData.m_Indices.size();
			Mesh::SubMesh submesh;
			submesh.m_MaterialIndex = meshData.m_MaterialIndex;
			submesh.m_IndexCount = indexCount;
			submesh.m_IndexOffset = preIndexCount;
			submesh.m_VertexOffset = preVertexCount;
			mesh.m_SubMeshes.insert(std::make_pair(meshData.m_Name, std::move(submesh)));
			
			preIndexCount += indexCount;
			preVertexCount += meshData.m_Positions.size();

			std::uint16_t posNormalFlags = kHasPosition;
			ASSERT((meshData.m_PSOFlags & posNormalFlags) != 0);
			mesh.m_PSOFlags = 0xffff;
			mesh.m_PSOFlags &= meshData.m_PSOFlags;
			positions.insert(positions.end(), meshData.m_Positions.begin(), meshData.m_Positions.end());
			normals.insert(normals.end(), meshData.m_Normals.begin(), meshData.m_Normals.end());
			uvs.insert(uvs.begin(), meshData.m_Texcoords.begin(), meshData.m_Texcoords.end());
			tangents.insert(tangents.end(), meshData.m_Tangents.begin(), meshData.m_Tangents.end());
			indices.insert(indices.end(), meshData.m_Indices.begin(), meshData.m_Indices.end());

			BoundingBox::CreateMerged(mesh.m_BoundingBox, mesh.m_BoundingBox, meshData.m_BoundingBox);
		}

		std::uint32_t posByteSize = positions.size() * sizeof(XMFLOAT3);
		std::uint32_t normalByteSize = normals.size() * sizeof(XMFLOAT3);
		std::uint32_t uvsByteSize = uvs.size() * sizeof(XMFLOAT2);
		std::uint32_t tangentsByteSize = tangents.size() * sizeof(XMFLOAT4);
		std::uint32_t indexByteSize = indices.size() * sizeof(std::uint32_t);
		
		GpuBufferDesc meshBufferDesc{};
		meshBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;
		meshBufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		meshBufferDesc.m_Stride = 1;
		meshBufferDesc.m_Size = posByteSize + normalByteSize + uvsByteSize + tangentsByteSize + indexByteSize;
		mesh.m_MeshData.Create(L"MeshData: " + Utility::UTF8ToWString(mesh.m_Name), meshBufferDesc);
		
		D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = mesh.m_MeshData.GetGpuVirtualAddress();
		std::uint32_t offset = 0;
		CommandList::InitBuffer(mesh.m_MeshData, positions.data(), posByteSize, offset);
		mesh.m_PositionStream = {bufferLocation + offset, posByteSize, sizeof(XMFLOAT3)};
		offset += posByteSize;

		if (normals.size() > 0) {
			CommandList::InitBuffer(mesh.m_MeshData, normals.data(), normalByteSize, offset);
			mesh.m_NormalStream = {bufferLocation + offset, normalByteSize, sizeof(XMFLOAT3)};
			offset += normalByteSize;
		}
		if (uvs.size() > 0) {
			CommandList::InitBuffer(mesh.m_MeshData, uvs.data(), uvsByteSize, offset);
			mesh.m_UVStream = {bufferLocation + offset, uvsByteSize, sizeof(XMFLOAT2)};
			offset += uvsByteSize;
		}
		if (tangents.size() > 0) {
			CommandList::InitBuffer(mesh.m_MeshData, tangents.data(), tangentsByteSize, offset);
			mesh.m_TangentStream = {bufferLocation + offset, tangentsByteSize, sizeof(XMFLOAT4)};
			offset += tangentsByteSize;
		}

		CommandList::InitBuffer(mesh.m_MeshData, indices.data(), indexByteSize, offset);
		mesh.m_IndexBufferViews = D3D12_INDEX_BUFFER_VIEW{bufferLocation + offset, indexByteSize, DXGI_FORMAT_R32_UINT};
		offset += indexByteSize;
	}

	void ProcessMaterial(
		Model& model,
		const std::string& filename,
		const aiScene* scene)
	{
		std::vector<std::uint32_t> srvOffsets(scene->mNumMaterials);
		
		model.m_Materials.resize(scene->mNumMaterials);
		for (UINT i = 0; i < scene->mNumMaterials; ++i) {
			auto& modelMaterial = model.m_Materials[i];
			auto& material = scene->mMaterials[i];

			modelMaterial = std::make_shared<Material>();
			
			XMFLOAT3 vector{};
			std::uint32_t num = 3;
			float value{};

			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_BASE_COLOR, (float*)&vector, &num)) {
				modelMaterial->m_BaseColor[0] = vector.x;
				modelMaterial->m_BaseColor[1] = vector.y;
				modelMaterial->m_BaseColor[2] = vector.z;
				modelMaterial->m_BaseColor[3] = 1.0f;
			}
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vector, &num)) {
				modelMaterial->m_EmissiveColor[0] = vector.x;
				modelMaterial->m_EmissiveColor[1] = vector.y;
				modelMaterial->m_EmissiveColor[2] = vector.z;
			}
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, value)) {
				modelMaterial->m_MetallicFactor = value;
			}
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, value)) {
				modelMaterial->m_RoughnessFactor = value;
			}

			aiString aiPath;
			std::filesystem::path texFilename;
			std::string texName;

			D3D12_CPU_DESCRIPTOR_HANDLE defaultTexture[kNumTextures] = {
				Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
				Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
				Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
				Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
				Graphics::GetDefaultTexture(Graphics::kBlackTransparent2D),
				Graphics::GetDefaultTexture(Graphics::kDefaultNormalTex)
			};
			
			D3D12_CPU_DESCRIPTOR_HANDLE srcHandle[kNumTextures];
			
			auto tryCreateTexture = [&](aiTextureType type) {
				MaterialTex materialTex;
				switch (type) {
					case aiTextureType_BASE_COLOR: materialTex = kBaseColor; break;
					case aiTextureType_DIFFUSE_ROUGHNESS: materialTex = kDiffuseRoughness; break;
					case aiTextureType_METALNESS: materialTex = kMetalness; break;
					case aiTextureType_AMBIENT_OCCLUSION : materialTex = kOcclusion; break;
					case aiTextureType_EMISSIVE: materialTex = kEmissive; break;
					case aiTextureType_NORMALS: materialTex = kNormal; break;
					default: materialTex = kBaseColor; break;
				}
				if (material->GetTextureCount(type) == 0) {
					srcHandle[materialTex] = defaultTexture[materialTex];
					return;
				}
				
				material->GetTexture(type, 0, &aiPath);

				// 纹理已经预先加载进来
				if (aiPath.data[0] == '*'){
					texName = filename;
					texName += aiPath.C_Str();
					char* pEndStr = nullptr;
					aiTexture* pTex = scene->mTextures[strtol(aiPath.data + 1, &pEndStr, 10)];
					TextureDesc texDesc{};
					texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					texDesc.m_Format = DXGI_FORMAT_UNKNOWN;
					texDesc.m_Width = pTex->mWidth;
					texDesc.m_Height = pTex->mHeight;
					texDesc.m_MipLevels = 1;
					texDesc.m_SampleDesc = {1,0};
					texDesc.m_DepthOrArraySize = 1;
					TextureRef& texRef = model.m_Textures.emplace_back(
						g_TexManager.LoadTextureFromMemory(texName, texDesc, pTex->pcData));
					srcHandle[materialTex] = texRef.GetSRV();
				}
				else {	// 纹理通过文件名索引
					texFilename = filename;
					texFilename = texFilename.parent_path() / aiPath.C_Str();
					auto texRef = g_TexManager.LoadTextureFromFile(texFilename.string());
					model.m_Textures.push_back(texRef);
					srcHandle[materialTex] = texRef.GetSRV();
				}
			};
			// 加载纹理
			tryCreateTexture(aiTextureType_BASE_COLOR);
			tryCreateTexture(aiTextureType_DIFFUSE_ROUGHNESS);
			tryCreateTexture(aiTextureType_METALNESS);
			tryCreateTexture(aiTextureType_AMBIENT_OCCLUSION);
			tryCreateTexture(aiTextureType_EMISSIVE);
			tryCreateTexture(aiTextureType_NORMALS);

			// 将纹理描述符拷贝到纹理堆中
			DescriptorHandle texHandle = g_Renderer.m_TextureHeap.Allocate(kNumTextures);
			//TODO:测试后删除
			auto cpuHandle = g_Renderer.m_TestTextureHeap->GetCPUDescriptorHandleForHeapStart();
			auto gpuHandle = g_Renderer.m_TestTextureHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle = DescriptorHandle{ cpuHandle, gpuHandle };
			auto offset = g_Renderer.m_HeapOffset * g_RenderContext.GetDevice()->
				GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			texHandle += offset;
			g_Renderer.m_HeapOffset += kNumTextures;

			std::uint32_t destCount = kNumTextures;
			std::uint32_t srcCount[kNumTextures] = {1,1,1,1,1,1};
			g_RenderContext.GetDevice()->CopyDescriptors(
				1, &texHandle, &destCount, destCount, srcHandle, srcCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			//srvOffsets[i] = g_Renderer.m_TextureHeap.GetOffsetOfHandle(texHandle);
			srvOffsets[i] = g_Renderer.m_HeapOffset - kNumTextures;
		}

		for (auto& mesh : model.m_Meshes) {
			int psoFlags = 0;
			std::uint32_t num = 1;
			for (auto& [name, submesh] : mesh->m_SubMeshes) {
				submesh.m_SRVTableOffset = srvOffsets[submesh.m_MaterialIndex];
				auto& material = scene->mMaterials[submesh.m_MaterialIndex];
				if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TWOSIDED, &psoFlags, &num)) {
					mesh->m_PSOFlags |= (psoFlags == 0) ? mesh->m_PSOFlags : kBothSide;
				}
			}
			mesh->m_PSOIndex = g_Renderer.GetPSO(mesh->m_PSOFlags);
		}

		std::vector<MaterialConstants> materialConstants(model.m_Materials.size());
		for (std::size_t i = 0; i < model.m_Materials.size(); i++) {
			memcpy(&materialConstants[i], model.m_Materials[i].get(), sizeof(MaterialConstants));
		}
		GpuBufferDesc bufferDesc = {};
		bufferDesc.m_Size = sizeof(MaterialConstants) * materialConstants.size();
		bufferDesc.m_Stride = sizeof(MaterialConstants);
		bufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		auto matBufferName = L"Model MaterialData" + Utility::UTF8ToWString(model.m_Name);
		model.m_MaterialData.Create(matBufferName, bufferDesc, materialConstants.data());
	}
	
}
