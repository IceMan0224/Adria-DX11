#pragma once
#include <memory>
#include "../Core/Engine.h"
#include "GUI.h"
#include "../tecs/registry.h"
#include "../Rendering/RendererSettings.h"
#include "../Graphics/ProfilerSettings.h"
#include "../ImGui/imgui_internal.h"
#include "../ImGui/ImGuizmo.h"

namespace adria
{
    struct Material;
    enum class EMaterialTextureType;
    struct ImGuiLogger;

	struct editor_init_t
	{
		engine_init_t engine_init;
	};
	class Editor
	{
	public:

        explicit Editor(editor_init_t const& init);

        ~Editor();

        void HandleWindowMessage(window_message_t const& msg_data);

        void Run();

	private:
        std::unique_ptr<Engine> engine;
		std::unique_ptr<GUI> gui;
        std::unique_ptr<ImGuiLogger> editor_log;
        tecs::entity selected_entity = tecs::null_entity;
        bool gizmo_enabled = false;
        bool scene_focused = false;
        ImGuizmo::OPERATION gizmo_op = ImGuizmo::TRANSLATE;
        RendererSettings renderer_settings{};
        ProfilerSettings profiler_settings{};
        SceneViewport scene_viewport_data;
	private:

        void SetStyle();
        void HandleInput();
        
        void MenuBar();
        void TerrainSettings();
        void OceanSettings();
        void SkySettings();
        void ParticleSettings();
        void ListEntities();
        void Properties();
        void Camera();
        void Scene();
        void Log();
        void RendererSettings();
        void StatsAndProfiling();

        void OpenMaterialFileDialog(Material* mat, EMaterialTextureType type);
	};
}


