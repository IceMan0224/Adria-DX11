#pragma once
#include <string>
#include <array>
#include <unordered_map>
#include <d3d11.h>
#include "GfxMacros.h"
#include "Utilities/Singleton.h"

namespace adria
{
	struct Timestamp
	{
		std::string name;
		Float time_in_ms;
	};

	class GfxDevice;
	class GfxCommandContext;
	class GfxQuery;

	class GfxProfiler : public Singleton<GfxProfiler>
	{
		friend class Singleton<GfxProfiler>;

		static constexpr Uint64 FRAME_COUNT = GFX_BACKBUFFER_COUNT;
		static constexpr Uint64 MAX_QUERIES = 256;
		struct QueryData
		{
			std::unique_ptr<GfxQuery> disjoint_query;
			std::unique_ptr<GfxQuery> timestamp_query_start;
			std::unique_ptr<GfxQuery> timestamp_query_end;
			Bool begin_called = false, end_called = false;
		};

	public:
		void Initialize(GfxDevice* gfx);
		void Destroy();
		void NewFrame();

		void BeginProfileScope(GfxCommandContext* context, Char const* name);
		void EndProfileScope(GfxCommandContext* context, Char const* name);
		std::vector<Timestamp> GetProfilingResults();

	private:
		GfxDevice* gfx = nullptr;
		Uint64 current_frame = 0;
		std::array<std::array<QueryData, MAX_QUERIES>, FRAME_COUNT> queries;
		std::unordered_map<std::string, Uint32> name_to_index_map;
		Uint32 scope_counter = 0;

	private:
		GfxProfiler();
		~GfxProfiler();
	};
	#define g_GfxProfiler GfxProfiler::Get()

#if GFX_PROFILING
	struct GfxProfileScope
	{
		GfxProfileScope(GfxCommandContext* context, Char const* name, Bool active = true)
			: name{ name }, context{ context }, active{ active }
		{
			if (active) g_GfxProfiler.BeginProfileScope(context, name);
		}

		~GfxProfileScope()
		{
			if (active)
				g_GfxProfiler.EndProfileScope(context, name);
		}

		GfxCommandContext* context;
		Char const* name;
		Bool active;
	};
	#define AdriaGfxProfileScope(context, name) GfxProfileScope ADRIA_CONCAT(gfx_profile, __COUNTER__)(context, name)
	#define AdriaGfxProfileCondScope(context, name, cond) GfxProfileScope ADRIA_CONCAT(gfx_profile, __COUNTER__)(context, name, cond)
	#define AdriaGfxProfileScopeOld(context, name) 
	#define AdriaGfxProfileCondScopeOld(context, name, active) 
#else
	#define AdriaGfxProfileScope(context, name) 
	#define AdriaGfxProfileCondScope(context, name, cond) 
	#define AdriaGfxProfileScopeOld(context, name) 
	#define AdriaGfxProfileCondScopeOld(context, name, active) 
#endif
}