#pragma once
#include <d3d11.h>

namespace adria
{
	using GfxColorDescriptor	 = ID3D11RenderTargetView*;
	using GfxDepthDescriptor	 = ID3D11DepthStencilView*;
	using GfxReadOnlyDescriptor  = ID3D11ShaderResourceView*;
	using GfxReadWriteDescriptor = ID3D11UnorderedAccessView*;

	using GfxArcColorDescriptor		= ArcPtr<ID3D11RenderTargetView>;
	using GfxArcDepthDescriptor		= ArcPtr<ID3D11DepthStencilView>;
	using GfxArcReadOnlyDescriptor	= ArcPtr<ID3D11ShaderResourceView>;
	using GfxArcReadWriteDescriptor = ArcPtr<ID3D11UnorderedAccessView>;
}