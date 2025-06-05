#pragma once
#ifndef __TEXTUREMANAGER__H__
#define __TEXTUREMANAGER__H__

#include "Utilities/Singleton.h"
#include "Graphics/Resource/Texture.h"
#include "Graphics/DescriptorHeap.h"

namespace DSM {

	
	class TextureManager : public Singleton<TextureManager>
	{
		friend class TextureRef;
	protected:
		class ManagedTexture : public Texture
		{
		public:
			ManagedTexture() : m_IsLoaded(true) {};
			virtual ~ManagedTexture() { Destroy(); };

			void Create(const std::string& filename, bool forceSRGB);
			void Create(const std::string& name, const TextureDesc& texDesc, const void* data);
			
			void WaitForLoad() const noexcept;
			virtual void Destroy() override;

			void Unload();

			bool IsValid() const noexcept { return m_IsValid; };

			D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept { return m_Descriptor; };

		private:
			std::string m_Name{};
			DescriptorHandle m_Descriptor{};
			std::atomic<bool> m_IsLoaded{false};
			bool m_IsValid = false;
		};
	
	public:
		TextureRef LoadTextureFromFile(const std::string& fileName, bool forceSRGB = false);
		TextureRef LoadTextureFromMemory(const std::string& name, const TextureDesc& texDesc, const void* data);

		void DestroyTexture(const std::string& name);

		size_t GetTextureCount() const noexcept;

	protected:
		friend class Singleton<TextureManager>;
		TextureManager() = default;
		virtual ~TextureManager() = default;

	protected:
		std::mutex m_Mutex;
		std::unordered_map<std::string, std::shared_ptr<ManagedTexture>> m_Textures;
	};

#define g_TexManager (TextureManager::GetInstance())


	class TextureRef
	{
	public:
		TextureRef(std::shared_ptr<TextureManager::ManagedTexture> tex = nullptr) : m_Texture(tex) {}
		~TextureRef();
		
		bool IsValid() const noexcept { return m_Texture != nullptr && m_Texture->IsValid(); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept;
		const Texture* Get() const noexcept { return m_Texture.get(); }
		const Texture* operator->() const { ASSERT(m_Texture != nullptr); return m_Texture.get(); }
		
	private:
		std::shared_ptr<TextureManager::ManagedTexture> m_Texture = nullptr;
	};
}

#endif