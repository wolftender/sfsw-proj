#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app.hpp"
#include "gui.hpp"

#include "scenes/spring.hpp"
#include "scenes/top.hpp"
#include "scenes/slerp.hpp"
#include "scenes/gel.hpp"
#include "scenes/ik.hpp"
#include "scenes/puma.hpp"
#include "scenes/flywheel.hpp"
#include "scenes/blackhole.hpp"
	
namespace mini {
	// ignore pedantic warnings
	inline constexpr int operator| (ImGuiDockNodeFlags_ f1, ImGuiDockNodeFlagsPrivate_ f2) {
		return static_cast<int>(f1) | static_cast<int>(f2);
	};

	application::application() : 
		application_base(1200, 800, "physics application"),
		m_context(video_mode_t(1200, 800)) {

		m_layout_ready = false;

		// load some basic shaders
		m_store.load_shader("basic", "shaders/vs_basic.glsl", "shaders/fs_basic.glsl");
		m_store.load_shader("grid_xz", "shaders/vs_grid.glsl", "shaders/fs_grid_xz.glsl");
		m_store.load_shader("grid_xy", "shaders/vs_grid.glsl", "shaders/fs_grid_xy.glsl");
		m_store.load_shader("billboard", "shaders/vs_billboard.glsl", "shaders/fs_billboard.glsl");
		m_store.load_shader("billboard_s", "shaders/vs_billboard_s.glsl", "shaders/fs_billboard.glsl");
		m_store.load_shader("line", "shaders/vs_basic.glsl", "shaders/fs_solidcolor.glsl", "shaders/gs_lines.glsl");
		m_store.load_shader("cube", "shaders/vs_shaded.glsl", "shaders/fs_shaded.glsl");
		m_store.load_shader("room", "shaders/vs_shaded.glsl", "shaders/fs_shaded_room.glsl");
		m_store.load_shader("gizmo", "shaders/vs_position.glsl", "shaders/fs_solidcolor.glsl");
		m_store.load_shader("point", "shaders/vs_billboard_s.glsl", "shaders/fs_point.glsl");
		m_store.load_shader("gelcube", "shaders/vs_gelcube.glsl", "shaders/fs_gelcube.glsl", 
			"shaders/tcs_gelcube.glsl", "shaders/tes_gelcube.glsl");
		m_store.load_shader("obstacle", "shaders/vs_basic_tex.glsl", "shaders/fs_solidcolor.glsl");
		m_store.load_shader("bezier_model", "shaders/vs_beziermodel.glsl", "shaders/fs_beziermodel.glsl");
		m_store.load_shader("puma", "shaders/vs_shaded.glsl", "shaders/fs_shaded.glsl");
		m_store.load_shader("blackhole", "shaders/vs_blackhole.glsl", "shaders/fs_blackhole.glsl");

		// load textures
		m_store.load_texture("slime_albedo", "textures/slime_albedo.png");
		m_store.load_texture("slime_normal", "textures/slime_normal.png");
		m_store.load_texture("duck_albedo", "textures/duck.png");

		m_scene = std::make_unique<black_hole_scene>(*this);
	}

	scene_base& application::get_scene() {
		return *m_scene.get();
	}

	app_context& application::get_context() {
		return m_context;
	}

	const app_context& application::get_context() const {
		return m_context;
	}

	resource_store& application::get_store() {
		return m_store;
	}

	void application::t_integrate(float delta_time) {
		if (m_scene) {
			m_scene->integrate(delta_time);
		}

		app_window::t_integrate(delta_time);
	}

	void application::t_render() {
		if (m_scene) {
			m_scene->render(get_context());
		}

		m_context.display(false, true);

		m_draw_main_window();

		if (m_scene) {
			m_scene->gui();
		}
	}

	void application::t_on_character(unsigned int code) {
		if (m_scene) {
			m_scene->on_character(code);
		}

		app_window::t_on_character(code);
	}

	void application::t_on_cursor_pos(double posx, double posy) {
		if (m_scene) {
			m_scene->on_cursor_pos(posx, posy);
		}

		app_window::t_on_cursor_pos(posx, posy);
	}

	void application::t_on_mouse_button(int button, int action, int mods) {
		if (m_scene) {
			m_scene->on_mouse_button(button, action, mods);
		}

		app_window::t_on_mouse_button(button, action, mods);
	}

	void application::t_on_key_event(int key, int scancode, int action, int mods) {
		bool lshift_down = is_key_down(GLFW_KEY_LEFT_SHIFT);
		bool rshift_down = is_key_down(GLFW_KEY_RIGHT_SHIFT);
		bool ctrl_down = is_key_down(GLFW_KEY_LEFT_CONTROL);

		if (ctrl_down && (lshift_down || rshift_down) && action == GLFW_PRESS) {
			switch (key) {
				case GLFW_KEY_F1:
					m_load_scene_spring();
					return;

				case GLFW_KEY_F2:
					m_load_scene_top();
					return;

				case GLFW_KEY_F3:
					m_load_scene_rotation();
					return;

				case GLFW_KEY_F4:
					m_load_scene_soft();
					return;

				case GLFW_KEY_F5:
					m_load_scene_ik();
					return;

				case GLFW_KEY_F6:
					m_load_scene_puma();
					return;
					
				case GLFW_KEY_F7:
					m_load_scene_flywheel();
					return;

				case GLFW_KEY_F8:
					m_load_scene_blackhole();
					return;

				default: 
					break;
			}
		}

		if (m_scene) {
			m_scene->on_key_event(key, scancode, action, mods);
		}

		app_window::t_on_key_event(key, scancode, action, mods);
	}

	void application::t_on_scroll(double offset_x, double offset_y) {
		if (m_scene) {
			m_scene->on_scroll(offset_x, offset_y);
		}

		app_window::t_on_scroll(offset_x, offset_y);
	}

	void application::t_on_resize(int width, int height) {
		if (m_scene) {
			m_scene->on_resize(width, height);
		}

		app_window::t_on_resize(width, height);
	}

	void application::m_draw_main_menu() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

		if (ImGui::BeginMenuBar()) {
			if (m_scene) {
				m_scene->menu();
			}
			
			if (ImGui::BeginMenu("Simulation")) {
				if (ImGui::MenuItem("Spring", "Ctrl + Shift + F1", nullptr, true)) {
					m_load_scene_spring();
				}

				if (ImGui::MenuItem("Spinning Top", "Ctrl + Shift + F2", nullptr, true)) {
					m_load_scene_top();
				}

				if (ImGui::MenuItem("Rotation Demo", "Ctrl + Shift + F3", nullptr, true)) {
					m_load_scene_rotation();
				}

				if (ImGui::MenuItem("Soft Body Demo", "Ctrl + Shift + F4", nullptr, true)) {
					m_load_scene_soft();
				}

				if (ImGui::MenuItem("IK Demo", "Ctrl + Shift + F5", nullptr, true)) {
					m_load_scene_ik();
				}

				if (ImGui::MenuItem("PUMA Demo", "Ctrl + Shift + F6", nullptr, true)) {
					m_load_scene_puma();
				}
				
				if (ImGui::MenuItem("Flywheel Demo", "Ctrl + Shift + F7", nullptr, true)) {
					m_load_scene_flywheel();
				}

				if (ImGui::MenuItem("Black Hole Demo", "Ctrl + Shift + F8", nullptr, true)) {
					m_load_scene_blackhole();
				}

				ImGui::EndMenu();
			}
		}

		ImGui::EndMenuBar();
		ImGui::PopStyleVar(1);
	}

	void application::m_draw_main_window() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		float vp_width = static_cast<float> (get_width());
		float vp_height = static_cast<float> (get_height());

		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Dockspace", nullptr,
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoResize);

		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(vp_width, vp_height));

		ImGuiID dockspace_id = ImGui::GetID("g_dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		//auto min = ImGui::GetWindowContentRegionMin();
		//auto max = ImGui::GetWindowContentRegionMax();

		if (!m_layout_ready) {
			m_layout_ready = true;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(vp_width, vp_height));

			if (m_scene) {
				m_scene->layout(dockspace_id);
			}

			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGui::PopStyleVar(3);
		m_draw_main_menu();

		ImGui::End();
	}

	void application::m_load_scene_spring() {
		m_scene = std::make_unique<spring_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_top() {
		m_scene = std::make_unique<top_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_rotation() {
		m_scene = std::make_unique<slerp_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_soft() {
		m_scene = std::make_unique<gel_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_ik() {
		m_scene = std::make_unique<ik_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_puma() {
		m_scene = std::make_unique<puma_scene>(*this);
		m_layout_ready = false;
	}
	
	void application::m_load_scene_flywheel() {
		m_scene = std::make_unique<flywheel_scene>(*this);
		m_layout_ready = false;
	}

	void application::m_load_scene_blackhole() {
		m_scene = std::make_unique<black_hole_scene>(*this);
		m_layout_ready = false;
	}
}
