#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "viewport.hpp"

namespace mini {
	viewport_window::viewport_window(application_base& app, const std::string_view& name) : 
		m_app(app),
		m_name(name),
		m_viewport_focus(false),
		m_mouse_in_viewport(false),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_distance(10.0f),
		m_cam_pitch(-0.7f),
		m_cam_yaw(0.0f),
		m_camera_target{ 0.0f, 0.0f, 0.0f } {
	}

	viewport_window::~viewport_window() {}

	bool viewport_window::is_viewport_focused() const {
		return m_viewport_focus;
	}

	bool viewport_window::is_mouse_in_viewport() const {
		return m_mouse_in_viewport;
	}

	const glm::vec3& viewport_window::get_camera_target() const {
		return m_camera_target;
	}

	void viewport_window::set_camera_target(const glm::vec3& target) {
		m_camera_target = target;
	}

	float viewport_window::get_distance() const {
		return m_distance;
	}

	void viewport_window::set_distance(float distance) {
		m_distance = glm::max(glm::min(distance, 20.0f), 0.0f);
	}

	float viewport_window::get_cam_pitch() const {
		return m_cam_pitch;
	}

	void viewport_window::set_cam_pitch(float pitch) {
		m_cam_pitch = pitch;
	}

	float viewport_window::get_cam_yaw() const {
		return m_cam_yaw;
	}

	void viewport_window::set_cam_yaw(float yaw) {
		m_cam_yaw = yaw;
	}

	void viewport_window::update(float delta_time) {
		// scene interactions
		if (m_app.is_left_click() && m_viewport_focus) {
			const offset_t& last_pos = m_app.get_last_mouse_offset();
			const offset_t& curr_pos = m_app.get_mouse_offset();

			int d_yaw = curr_pos.x - last_pos.x;
			int d_pitch = curr_pos.y - last_pos.y;

			float f_d_yaw = static_cast<float> (d_yaw);
			float f_d_pitch = static_cast<float> (d_pitch);

			f_d_yaw = f_d_yaw * 15.0f / static_cast<float> (m_app.get_width());
			f_d_pitch = f_d_pitch * 15.0f / static_cast<float> (m_app.get_height());

			m_cam_yaw = m_cam_yaw - f_d_yaw;
			m_cam_pitch = m_cam_pitch + f_d_pitch;
		}

		// camera uptading
		gui::clamp(m_distance, 1.0f, 30.0f);

		glm::vec4 cam_pos = { 0.0f, 0.0f, -m_distance, 1.0f };
		glm::mat4x4 cam_rotation(1.0f);

		cam_rotation = glm::translate(cam_rotation, m_camera_target);
		cam_rotation = glm::rotate(cam_rotation, m_cam_yaw, { 0.0f, 1.0f, 0.0f });
		cam_rotation = glm::rotate(cam_rotation, m_cam_pitch, { 1.0f, 0.0f, 0.0f });

		cam_pos = cam_rotation * cam_pos;

		m_app.get_context().get_camera().set_position(cam_pos);
		m_app.get_context().get_camera().set_target(m_camera_target);
	}

	void viewport_window::display() {
		auto& context = m_app.get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin(m_name.c_str(), NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_Once);

		if (ImGui::IsWindowFocused()) {
			m_viewport_focus = true;
		} else {
			m_viewport_focus = false;
		}

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto window_pos = ImGui::GetWindowPos();

		int width = static_cast<int>(max.x - min.x);
		int height = static_cast<int>(max.y - min.y);

		const offset_t& mouse_offset = m_app.get_mouse_offset();
		m_vp_mouse_offset.x = mouse_offset.x - static_cast<int> (min.x + window_pos.x);
		m_vp_mouse_offset.y = mouse_offset.y - static_cast<int> (min.y + window_pos.y);

		if (m_vp_mouse_offset.x < 0 || m_vp_mouse_offset.x > width || m_vp_mouse_offset.y < 0 || m_vp_mouse_offset.y > height) {
			m_mouse_in_viewport = false;
		} else {
			m_mouse_in_viewport = true;
		}

		if ((width != m_last_vp_width || height != m_last_vp_height) && width > 8 && height > 8) {
			video_mode_t mode(width, height);

			context.set_video_mode(mode);

			m_last_vp_width = width;
			m_last_vp_height = height;
		} else {
			ImGui::ImageButton(
				reinterpret_cast<ImTextureID>(context.get_front_buffer()),
				ImVec2(width, height), ImVec2(0, 0), ImVec2(1, 1), 0);
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}
}