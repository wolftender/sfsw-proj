#pragma once
#include <array>
#include <memory>

#include <glm/glm.hpp>

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
				float integration_step;

				simulation_settings_t() :
					gravity(0.98f),
					mass(1.0f),
					spring_length(1.0f),
					spring_friction(0.1f),
					spring_coefficient(1.0f),
					integration_step(0.001f) { }
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
				std::vector<spring_t> springs;

				simulation_settings_t settings;
				float time;

				simulation_state_t(const simulation_settings_t& settings);

				void integrate(float delta_time);
				void reset(const simulation_settings_t& settings);
			};

			simulation_settings_t m_settings;
			simulation_state_t m_state;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<billboard_object> m_point_object;
			std::shared_ptr<segments_array> m_springs_object;

			viewport_window m_viewport;

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

			void m_reset_spring_array();
	};
}