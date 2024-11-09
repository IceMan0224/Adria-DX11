#pragma once
#include <string>
#include <array>
#include <unordered_map>
#include "Graphics/GfxDevice.h"
#include "Graphics/GfxView.h"
#include "Utilities/Singleton.h"

namespace adria
{
	using TextureHandle = Uint64;
	inline constexpr TextureHandle const INVALID_TEXTURE_HANDLE = Uint64(-1);

	class TextureManager : public Singleton<TextureManager>
	{
		friend class Singleton<TextureManager>;

	public:
		void Initialize(GfxDevice* gfx);
		void Destroy();

		ADRIA_NODISCARD TextureHandle LoadTexture(std::wstring const& name);
		ADRIA_NODISCARD TextureHandle LoadTexture(std::string const& name);
		ADRIA_NODISCARD TextureHandle LoadCubeMap(std::wstring const& name);
		ADRIA_NODISCARD TextureHandle LoadCubeMap(std::array<std::string, 6> const& cubemap_textures);

		GfxShaderResourceRO GetTextureView(TextureHandle tex_handle) const;
		void SetMipMaps(Bool mipmaps);

	private:
		GfxDevice* gfx;
		Bool mipmaps = true;
		TextureHandle handle = INVALID_TEXTURE_HANDLE;
		std::unordered_map<TextureHandle, GfxShaderResourceRORef> texture_map{};
		std::unordered_map<std::wstring, TextureHandle> loaded_textures{};

	private:
		TextureManager() = default;
		TextureManager(TextureManager const&) = delete;
		TextureManager& operator=(TextureManager const&) = delete;
		~TextureManager() = default;

		TextureHandle LoadDDSTexture(std::wstring const& name);
		TextureHandle LoadWICTexture(std::wstring const& name);
		TextureHandle LoadTexture_HDR_TGA_PIC(std::string const& name);
	};
	#define g_TextureManager TextureManager::Get()
}