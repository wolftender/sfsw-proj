#pragma once
#include <string_view>

#include "context.hpp"
#include "scene.hpp"

namespace mini {
	class viewport_window final {
		private:
			application_base& m_app;
			app_context& m_context;

			bool m_viewport_focus;
			bool m_mouse_in_viewport;
			bool m_camera_moved;

			float m_distance;
			float m_cam_pitch;
			float m_cam_yaw;

			int m_last_vp_width, m_last_vp_height;
			offset_t m_vp_mouse_offset;

			std::string m_name;
			glm::vec3 m_camera_target;
			
		public:
			viewport_window(application_base& app, const std::string_view& name);
			viewport_window(application_base& app, app_context& context, const std::string_view& name);
			~viewport_window();

			viewport_window(const viewport_window&) = delete;
			viewport_window& operator=(const viewport_window&) = delete;

			bool is_camera_moved() const;

			bool is_viewport_focused() const;
			bool is_mouse_in_viewport() const;

			const glm::vec3& get_camera_target() const;
			void set_camera_target(const glm::vec3& target);

			float get_distance() const;
			void set_distance(float distance);

			float get_cam_pitch() const;
			void set_cam_pitch(float pitch);

			float get_cam_yaw() const;
			void set_cam_yaw(float yaw);

			void update(float delta_time);
			void display();
			void display(std::optional<std::function<void()>> overlay);
			void configure();
	};
}