#pragma once
#include <array>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "grid.hpp"
#include "scene.hpp"
#include "viewport.hpp"
#include "billboard.hpp"
#include "segments.hpp"

namespace mini {
	class gel_scene : public scene_base {
		private:
			struct simulation_settings_t {
				float gravity;
				float mass;
				float spring_length;
				float spring_friction;
				float spring_coefficient;
				float frame_coefficient;
				float integration_step;
				float frame_length;
				bool enable_gravity;

				simulation_settings_t() :
					gravity(9.807f),
					mass(1.0f),
					spring_length(1.0f),
					spring_friction(1.0f),
					spring_coefficient(1.0f),
					frame_coefficient(2.0f),
					integration_step(0.017f),
					frame_length(3.2f),
					enable_gravity(false) { }
			};

			struct point_mass_t {
				glm::vec3 x;
				glm::vec3 dx;
				glm::vec3 ddx;

				point_mass_t(const glm::vec3& x);
			};

			struct spring_t {
				float length;
				bool enabled;
			};

			struct simulation_state_t {
				std::vector<point_mass_t> point_masses;
				std::vector<glm::vec3> force_sums;
				std::vector<spring_t> springs;
				std::vector<std::size_t> active_springs;

				glm::vec3 frame_offset;
				glm::quat frame_rotation;

				simulation_settings_t settings;
				float time;
				float mass_inv;
				float frame_spring_len;

				simulation_state_t(const simulation_settings_t& settings);

				void integrate(float delta_time);
				void reset(const simulation_settings_t& settings);
			};

			simulation_settings_t m_settings;
			simulation_state_t m_state;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<billboard_object> m_point_object;
			std::shared_ptr<segments_array> m_springs_object;
			std::shared_ptr<segments_array> m_cube_object;

			viewport_window m_viewport;
			glm::vec3 m_frame_euler;

		public:
			gel_scene(application_base& app);
			~gel_scene();

			gel_scene(const gel_scene&) = delete;
			gel_scene& operator=(const gel_scene&) = delete;

			void layout(ImGuiID dockspace_id) override;
			void integrate(float delta_time) override;
			void render(app_context& context) override;
			void gui() override;
			void menu() override;

			virtual void on_scroll(double offset_x, double offset_y) override;

		private:
			void m_gui_viewport();
			void m_gui_settings();

			void m_build_cube_object(std::shared_ptr<shader_program> line_shader);
			void m_reset_spring_array();
	};
}