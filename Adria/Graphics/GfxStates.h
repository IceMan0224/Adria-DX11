#pragma once
#include <d3d11.h>

namespace adria
{
	enum class GfxPrimitiveTopology : uint8
	{
		Undefined,
		TriangleList,
		TriangleStrip,
		PointList,
		LineList,
		LineStrip,
		PatchList1,
		PatchList2,
		PatchList3,
		PatchList4,
		PatchList5,
		PatchList6,
		PatchList7,
		PatchList8,
		PatchList9,
		PatchList10,
		PatchList11,
		PatchList12,
		PatchList13,
		PatchList14,
		PatchList15,
		PatchList16,
		PatchList17,
		PatchList18,
		PatchList19,
		PatchList20,
		PatchList21,
		PatchList22,
		PatchList23,
		PatchList24,
		PatchList25,
		PatchList26,
		PatchList27,
		PatchList28,
		PatchList29,
		PatchList30,
		PatchList31,
		PatchList32
	};
	enum class GfxComparisonFunc : uint8
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};
	inline constexpr D3D11_COMPARISON_FUNC ConvertComparisonFunc(GfxComparisonFunc value)
	{
		switch (value)
		{
		case GfxComparisonFunc::Never:
			return D3D11_COMPARISON_NEVER;
			break;
		case GfxComparisonFunc::Less:
			return D3D11_COMPARISON_LESS;
			break;
		case GfxComparisonFunc::Equal:
			return D3D11_COMPARISON_EQUAL;
			break;
		case GfxComparisonFunc::LessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
			break;
		case GfxComparisonFunc::Greater:
			return D3D11_COMPARISON_GREATER;
			break;
		case GfxComparisonFunc::NotEqual:
			return D3D11_COMPARISON_NOT_EQUAL;
			break;
		case GfxComparisonFunc::GreaterEqual:
			return D3D11_COMPARISON_GREATER_EQUAL;
			break;
		case GfxComparisonFunc::Always:
			return D3D11_COMPARISON_ALWAYS;
			break;
		default:
			break;
		}
		return D3D11_COMPARISON_NEVER;
	}

	enum class GfxDepthWriteMask : uint8
	{
		Zero,
		All,
	};
	enum class GfxStencilOp : uint8
	{
		Keep,
		Zero,
		Replace,
		IncrSat,
		DecrSat,
		Invert,
		Incr,
		Decr,
	};
	enum class GfxBlend : uint8
	{
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		SrcAlpha,
		InvSrcAlpha,
		DstAlpha,
		InvDstAlpha,
		DstColor,
		InvDstColor,
		SrcAlphaSat,
		BlendFactor,
		InvBlendFactor,
		Src1Color,
		InvSrc1Color,
		Src1Alpha,
		InvSrc1Alpha,
	};
	enum class GfxBlendOp : uint8
	{
		Add,
		Subtract,
		RevSubtract,
		Min,
		Max,
	};
	enum class GfxFillMode : uint8
	{
		Wireframe,
		Solid,
	};
	enum class GfxCullMode : uint8
	{
		None,
		Front,
		Back,
	};
	enum class GfxColorWrite
	{
		Disable = 0,
		EnableRed = 1 << 0,
		EnableGreen = 1 << 1,
		EnableBlue = 1 << 2,
		EnableAlpha = 1 << 3,
		EnableAll = ~0,
	};

	struct GfxRasterizerState
	{
		GfxFillMode fill_mode = GfxFillMode::Solid;
		GfxCullMode cull_mode = GfxCullMode::Back;
		bool front_counter_clockwise = false;
		int32 depth_bias = 0;
		float depth_bias_clamp = 0.0f;
		float slope_scaled_depth_bias = 0.0f;
		bool depth_clip_enable = true;
		bool multisample_enable = false;
		bool antialiased_line_enable = false;
		bool conservative_rasterization_enable = false;
		uint32 forced_sample_count = 0;
	};
	struct GfxDepthStencilState
	{
		bool depth_enable = true;
		GfxDepthWriteMask depth_write_mask = GfxDepthWriteMask::All;
		GfxComparisonFunc depth_func = GfxComparisonFunc::Less;
		bool stencil_enable = false;
		uint8 stencil_read_mask = 0xff;
		uint8 stencil_write_mask = 0xff;
		struct GfxDepthStencilOp
		{
			GfxStencilOp stencil_fail_op = GfxStencilOp::Keep;
			GfxStencilOp stencil_depth_fail_op = GfxStencilOp::Keep;
			GfxStencilOp stencil_pass_op = GfxStencilOp::Keep;
			GfxComparisonFunc stencil_func = GfxComparisonFunc::Always;
		};
		GfxDepthStencilOp front_face{};
		GfxDepthStencilOp back_face{};
	};
	struct GfxBlendState
	{
		bool alpha_to_coverage_enable = false;
		bool independent_blend_enable = false;
		struct GfxRenderTargetBlendState
		{
			bool blend_enable = false;
			GfxBlend src_blend = GfxBlend::One;
			GfxBlend dest_blend = GfxBlend::Zero;
			GfxBlendOp blend_op = GfxBlendOp::Add;
			GfxBlend src_blend_alpha = GfxBlend::One;
			GfxBlend dest_blend_alpha = GfxBlend::Zero;
			GfxBlendOp blend_op_alpha = GfxBlendOp::Add;
			GfxColorWrite render_target_write_mask = GfxColorWrite::EnableAll;
		};
		GfxRenderTargetBlendState render_target[8];
	};

	D3D11_RASTERIZER_DESC ConvertRasterizerDesc(GfxRasterizerState);
	D3D11_DEPTH_STENCIL_DESC ConvertDepthStencilDesc(GfxDepthStencilState);
	D3D11_BLEND_DESC ConvertBlendDesc(GfxBlendState);
	inline constexpr D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(GfxPrimitiveTopology topology)
	{
		switch (topology)
		{
		case GfxPrimitiveTopology::PointList:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case GfxPrimitiveTopology::LineList:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case GfxPrimitiveTopology::LineStrip:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case GfxPrimitiveTopology::TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case GfxPrimitiveTopology::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			if (topology >= GfxPrimitiveTopology::PatchList1 && topology <= GfxPrimitiveTopology::PatchList32)
				return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + ((uint32)topology - (uint32)GfxPrimitiveTopology::PatchList1));
			else return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}
}