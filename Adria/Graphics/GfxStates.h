#pragma once
#include <d3d11.h>

namespace adria
{
	enum class GfxPrimitiveTopology : Uint8
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
	enum class GfxComparisonFunc : Uint8
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

	enum class GfxDepthWriteMask : Uint8
	{
		Zero,
		All,
	};
	enum class GfxStencilOp : Uint8
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
	enum class GfxBlend : Uint8
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
	enum class GfxBlendOp : Uint8
	{
		Add,
		Subtract,
		RevSubtract,
		Min,
		Max,
	};
	enum class GfxFillMode : Uint8
	{
		Wireframe,
		Solid,
	};
	enum class GfxCullMode : Uint8
	{
		None,
		Front,
		Back,
	};
	enum class GfxColorWrite : Uint8
	{
		Disable = 0,
		EnableRed = 1 << 0,
		EnableGreen = 1 << 1,
		EnableBlue = 1 << 2,
		EnableAlpha = 1 << 3,
		EnableAll = EnableRed | EnableGreen | EnableBlue | EnableAlpha,
	};

	struct GfxRasterizerStateDesc
	{
		GfxFillMode fill_mode = GfxFillMode::Solid;
		GfxCullMode cull_mode = GfxCullMode::Back;
		Bool front_counter_clockwise = false;
		Sint32 depth_bias = 0;
		Float depth_bias_clamp = 0.0f;
		Float slope_scaled_depth_bias = 0.0f;
		Bool depth_clip_enable = true;
		Bool multisample_enable = false;
		Bool antialiased_line_enable = false;
		Bool conservative_rasterization_enable = false;
		Uint32 forced_sample_count = 0;
	};
	struct GfxDepthStencilStateDesc
	{
		Bool depth_enable = true;
		GfxDepthWriteMask depth_write_mask = GfxDepthWriteMask::All;
		GfxComparisonFunc depth_func = GfxComparisonFunc::Less;
		Bool stencil_enable = false;
		Uint8 stencil_read_mask = 0xff;
		Uint8 stencil_write_mask = 0xff;
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
	struct GfxBlendStateDesc
	{
		Bool alpha_to_coverage_enable = false;
		Bool independent_blend_enable = false;
		struct GfxRenderTargetBlendState
		{
			Bool blend_enable = false;
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

	class GfxDevice;

	class GfxRasterizerState
	{
		friend class GfxCommandContext;
	public:
		GfxRasterizerState(GfxDevice* gfx, GfxRasterizerStateDesc const& desc);

	private:
		Ref<ID3D11RasterizerState> rasterizer_state;

	private:
		operator ID3D11RasterizerState* () const
		{
			return rasterizer_state.Get();
		}
	};
	class GfxDepthStencilState
	{
		friend class GfxCommandContext;
	public:
		GfxDepthStencilState(GfxDevice* gfx, GfxDepthStencilStateDesc const& desc);

	private:
		Ref<ID3D11DepthStencilState> depth_stencil_state;

	private:
		operator ID3D11DepthStencilState* () const
		{
			return depth_stencil_state.Get();
		}
	};
	class GfxBlendState
	{
		friend class GfxCommandContext;
	public:
		GfxBlendState(GfxDevice* gfx, GfxBlendStateDesc const& desc);

	private:
		Ref<ID3D11BlendState> blend_state;

	private:
		operator ID3D11BlendState* () const
		{
			return blend_state.Get();
		}
	};

	enum class GfxTextureAddressMode
	{
		Wrap,
		Mirror,
		Clamp,
		Border,
	};
	enum class GfxFilter : Uint32
	{
		MIN_MAG_MIP_POINT,
		MIN_MAG_POINT_MIP_LINEAR,
		MIN_POINT_MAG_LINEAR_MIP_POINT,
		MIN_POINT_MAG_MIP_LINEAR,
		MIN_LINEAR_MAG_MIP_POINT,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MIN_MAG_LINEAR_MIP_POINT,
		MIN_MAG_MIP_LINEAR,
		ANISOTROPIC,
		COMPARISON_MIN_MAG_MIP_POINT,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_MAG_MIP_LINEAR,
		COMPARISON_ANISOTROPIC,
		MINIMUM_MIN_MAG_MIP_POINT,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_MAG_MIP_LINEAR,
		MINIMUM_ANISOTROPIC,
		MAXIMUM_MIN_MAG_MIP_POINT,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_MAG_MIP_LINEAR,
		MAXIMUM_ANISOTROPIC,
	};

	struct GfxSamplerDesc
	{
		GfxFilter filter = GfxFilter::MIN_MAG_MIP_POINT;
		GfxTextureAddressMode addressU = GfxTextureAddressMode::Clamp;
		GfxTextureAddressMode addressV = GfxTextureAddressMode::Clamp;
		GfxTextureAddressMode addressW = GfxTextureAddressMode::Clamp;
		Float mip_lod_bias = 0.0f;
		Uint32 max_anisotropy = 0;
		GfxComparisonFunc comparison_func = GfxComparisonFunc::Never;
		Float border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		Float min_lod = 0.0f;
		Float max_lod = FLT_MAX;
	};
	class GfxSampler
	{
		friend class GfxCommandContext;
	public:
		GfxSampler(GfxDevice* gfx, GfxSamplerDesc const& desc);

		operator ID3D11SamplerState* const() const
		{
			return sampler.Get();
		}
	private:
		Ref<ID3D11SamplerState> sampler;
	};

	inline GfxBlendStateDesc OpaqueBlendStateDesc()
	{
		GfxBlendStateDesc desc{};
		desc.render_target[0].blend_enable = false;
		desc.render_target[0].src_blend = desc.render_target[0].src_blend_alpha = GfxBlend::One;
		desc.render_target[0].dest_blend = desc.render_target[0].dest_blend_alpha = GfxBlend::Zero;
		desc.render_target[0].blend_op = desc.render_target[0].blend_op_alpha = GfxBlendOp::Add;
		desc.render_target[0].render_target_write_mask = GfxColorWrite::EnableAll;
		return desc;
	}
	inline GfxBlendStateDesc AlphaBlendStateDesc()
	{
		GfxBlendStateDesc desc{};
		desc.render_target[0].blend_enable = true;
		desc.render_target[0].src_blend = desc.render_target[0].src_blend_alpha = GfxBlend::SrcAlpha;
		desc.render_target[0].dest_blend = desc.render_target[0].dest_blend_alpha = GfxBlend::InvSrcAlpha;
		desc.render_target[0].blend_op = desc.render_target[0].blend_op_alpha = GfxBlendOp::Add;
		desc.render_target[0].render_target_write_mask = GfxColorWrite::EnableAll;
		return desc;
	}
	inline GfxBlendStateDesc AdditiveBlendStateDesc()
	{
		GfxBlendStateDesc desc{};
		desc.render_target[0].blend_enable = true;
		desc.render_target[0].src_blend = desc.render_target[0].src_blend_alpha = GfxBlend::One;
		desc.render_target[0].dest_blend = desc.render_target[0].dest_blend_alpha = GfxBlend::One;
		desc.render_target[0].blend_op = desc.render_target[0].blend_op_alpha = GfxBlendOp::Add;
		desc.render_target[0].render_target_write_mask = GfxColorWrite::EnableAll;
		return desc;
	}

	inline GfxDepthStencilStateDesc NoneDepthDesc()
	{
		GfxDepthStencilStateDesc desc{};
		desc.depth_enable = false;
		desc.depth_write_mask = GfxDepthWriteMask::Zero;
		desc.depth_func = GfxComparisonFunc::LessEqual;
		desc.stencil_enable = false;
		desc.stencil_read_mask = 0xff;
		desc.stencil_write_mask = 0xff;

		desc.front_face.stencil_func = GfxComparisonFunc::Always;
		desc.front_face.stencil_pass_op = GfxStencilOp::Keep;
		desc.front_face.stencil_fail_op = GfxStencilOp::Keep;
		desc.front_face.stencil_depth_fail_op = GfxStencilOp::Keep;
		desc.back_face = desc.front_face;

		return desc;
	}
	inline GfxDepthStencilStateDesc DefaultDepthDesc()
	{
		GfxDepthStencilStateDesc desc{};
		desc.depth_enable = true;
		desc.depth_write_mask = GfxDepthWriteMask::All;
		desc.depth_func = GfxComparisonFunc::LessEqual;
		desc.stencil_enable = false;
		desc.stencil_read_mask = 0xff;
		desc.stencil_write_mask = 0xff;

		desc.front_face.stencil_func = GfxComparisonFunc::Always;
		desc.front_face.stencil_pass_op = GfxStencilOp::Keep;
		desc.front_face.stencil_fail_op = GfxStencilOp::Keep;
		desc.front_face.stencil_depth_fail_op = GfxStencilOp::Keep;
		desc.back_face = desc.front_face;

		return desc;
	}
	inline GfxDepthStencilStateDesc ReadDepthDesc()
	{
		GfxDepthStencilStateDesc desc{};
		desc.depth_enable = true;
		desc.depth_write_mask = GfxDepthWriteMask::Zero;
		desc.depth_func = GfxComparisonFunc::LessEqual;
		desc.stencil_enable = false;
		desc.stencil_read_mask = 0xff;
		desc.stencil_write_mask = 0xff;

		desc.front_face.stencil_func = GfxComparisonFunc::Always;
		desc.front_face.stencil_pass_op = GfxStencilOp::Keep;
		desc.front_face.stencil_fail_op = GfxStencilOp::Keep;
		desc.front_face.stencil_depth_fail_op = GfxStencilOp::Keep;
		desc.back_face = desc.front_face;

		return desc;
	}

	inline GfxRasterizerStateDesc CullNoneDesc()
	{
		GfxRasterizerStateDesc desc{};
		desc.cull_mode = GfxCullMode::None;
		desc.fill_mode = GfxFillMode::Solid;
		desc.depth_clip_enable = true;
		desc.multisample_enable = true;
		return desc;
	}
	inline GfxRasterizerStateDesc CullCWDesc()
	{
		GfxRasterizerStateDesc desc{};
		desc.cull_mode = GfxCullMode::Front;
		desc.fill_mode = GfxFillMode::Solid;
		desc.depth_clip_enable = true;
		desc.multisample_enable = true;
		return desc;
	}
	inline GfxRasterizerStateDesc CullCCWDesc()
	{
		GfxRasterizerStateDesc desc{};
		desc.cull_mode = GfxCullMode::Back;
		desc.fill_mode = GfxFillMode::Solid;
		desc.depth_clip_enable = true;
		desc.multisample_enable = true;
		return desc;
	}
	inline GfxRasterizerStateDesc WireframeDesc()
	{
		GfxRasterizerStateDesc desc{};
		desc.cull_mode = GfxCullMode::Back;
		desc.fill_mode = GfxFillMode::Wireframe;
		desc.depth_clip_enable = true;
		desc.multisample_enable = true;
		return desc;
	}

	inline GfxSamplerDesc SamplerDesc(GfxFilter filter, GfxTextureAddressMode mode)
	{
		GfxSamplerDesc desc{};
		desc.filter = filter;
		desc.addressU = mode;
		desc.addressV = mode;
		desc.addressW = mode;
		desc.max_anisotropy = 16;
		desc.max_lod = FLT_MAX;
		desc.comparison_func = GfxComparisonFunc::Never;
		return desc;
	}
}