#pragma once
#include "scene.hpp"
#include "curve.hpp"
#include "grid.hpp"

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace mini {
	class flywheel_scene : public scene_base {
		private:
			struct simulation_state_t {
				float wheel_radius;
				float stick_length;
				float time;
				
				simulation_state_t() :
					wheel_radius(5.0f),
					stick_length(10.0f),
					time(0.0f) { }
					
				void integrate(float delta_time);
			};
			
			simulation_state_t m_state;
		
			int m_last_vp_width, m_last_vp_height;
			bool m_mouse_in_viewport, m_viewport_focus;
			
			offset_t m_vp_mouse_offset;
		
			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<curve> m_wheel_curve;
			std::shared_ptr<curve> m_square_curve;
			std::shared_ptr<curve> m_stick_curve;
	
		public:
			flywheel_scene(application_base & app);
			~flywheel_scene();

			flywheel_scene(const flywheel_scene&) = delete;
			flywheel_scene& operator=(const flywheel_scene&) = delete;

			void layout(ImGuiID dockspace_id) override;
			void integrate(float delta_time) override;
			void render(app_context& context) override;
			void gui() override;
			void menu() override;

			virtual void on_scroll(double offset_x, double offset_y) override;

		private:
			void m_gui_settings();
			void m_gui_viewport();
			
			std::shared_ptr<curve> m_make_square_curve(std::shared_ptr<shader_program> line_shader) const;
			std::shared_ptr<curve> m_make_wheel_curve(std::shared_ptr<shader_program> line_shader) const;
	};
}

