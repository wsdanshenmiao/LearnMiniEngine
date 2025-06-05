#include "TextureManager.h"

#include "Graphics/GraphicsCommon.h"
#include "Utilities/FormatUtil.h"
#include "Graphics/RenderContext.h"


namespace DSM {
	
	void TextureManager::ManagedTexture::Create(const std::string& filename, bool forceSRGB)
	{
		m_IsValid = CreateTextureFromFile(*this, filename, filename, forceSRGB);

		if (m_IsValid) {
			m_Descriptor = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CreateShaderResourceView(m_Descriptor);
		}
		else {
			g_RenderContext.GetDevice()->CopyDescriptorsSimple(
				1, m_Descriptor,
				Graphics::GetDefaultTexture(Graphics::kMagenta2D),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		
		m_IsLoaded.store(false);
	}

	void TextureManager::ManagedTexture::Create(const std::string& name, const TextureDesc& texDesc, const void* data)
	{
		ASSERT(data != nullptr);
		m_IsValid = true;

		D3D12_SUBRESOURCE_DATA subresourceData{};
		subresourceData.pData = data;
		subresourceData.RowPitch = texDesc.m_Width * Utility::GetFormatStride(texDesc.m_Format);
		subresourceData.SlicePitch = subresourceData.RowPitch * texDesc.m_Height;
		Texture::Create(Utility::UTF8ToWString(name), texDesc, {&subresourceData, 1});

		m_Descriptor = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CreateShaderResourceView(m_Descriptor);

		m_IsLoaded.store(false);
	}

	void TextureManager::ManagedTexture::WaitForLoad() const noexcept
	{
		while (m_IsLoaded.load()) {
			std::this_thread::yield();
		}
	}

	void TextureManager::ManagedTexture::Destroy()
	{
		if (m_Resource != nullptr) {
			g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Descriptor);
		}
		Texture::Destroy();
	}

	void TextureManager::ManagedTexture::Unload()
	{
		g_TexManager.DestroyTexture(m_Name);
	}


	TextureRef TextureManager::LoadTextureFromFile(const std::string& fileName, bool forceSRGB)
	{
		std::shared_ptr<ManagedTexture> tex = nullptr;

		std::string key = forceSRGB ? (fileName + "_SRGB") : fileName;

		{
			std::lock_guard lock{m_Mutex}; 

			// 防止多线程的情况
			if (auto it = m_Textures.find(key); it != m_Textures.end()) {
				tex = it->second;
				tex->WaitForLoad();
				return tex;
			}
			else {
				tex = std::make_shared<ManagedTexture>();
				m_Textures[key] = tex;
			}
		}

		tex->Create(fileName, forceSRGB);
		return tex;
	}
	
	TextureRef TextureManager::LoadTextureFromMemory(const std::string& name, const TextureDesc& texDesc, const void* data)
	{
		std::shared_ptr<ManagedTexture> tex = nullptr;

		{
			std::lock_guard lock{m_Mutex}; 

			// 防止多线程的情况
			if (auto it = m_Textures.find(name); it != m_Textures.end()) {
				tex = it->second;
				tex->WaitForLoad();
				return tex;
			}
			else {
				tex = std::make_shared<ManagedTexture>();
				m_Textures[name] = tex;
			}
		}

		tex->Create(name, texDesc, data);
		return tex;
	}

	void TextureManager::DestroyTexture(const std::string& name)
	{
		std::lock_guard lock(m_Mutex);

		// 调用Unload的时候还有一个引用，因此是小于等于两个
		if (auto it = m_Textures.find(name); it != m_Textures.end() && it->second.use_count() <= 2) {
			m_Textures.erase(it);
		}
	}

	size_t TextureManager::GetTextureCount() const noexcept
	{
		return m_Textures.size();
	}



	
	TextureRef::~TextureRef()
	{
		if (m_Texture != nullptr) {
			m_Texture->Unload();
			m_Texture = nullptr;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureRef::GetSRV() const noexcept
	{
		return (m_Texture != nullptr) ? m_Texture->GetSRV() : Graphics::GetDefaultTexture(Graphics::kMagenta2D);
	}
}
