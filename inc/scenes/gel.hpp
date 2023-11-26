#pragma once
#include <array>
#include <memory>

#include <glm/glm.hpp>

#include "scene.hpp"
#include "viewport.hpp"
#include "grid.hpp"

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

				const simulation_settings_t& settings;
				std::vector<point_mass_t*> springs;

				point_mass_t(const simulation_settings_t& settings, const glm::vec3& x);

				point_mass_t(const point_mass_t&) = delete;
				point_mass_t& operator=(const point_mass_t&) = delete;
			};

			struct simulation_state_t {
				std::vector<std::unique_ptr<point_mass_t>> point_masses;
				simulation_settings_t settings;
				float m_time;

				simulation_state_t(const simulation_settings_t& settings);

				void integrate(float delta_time);
				void reset(const simulation_settings_t& settings);

				simulation_state_t(const simulation_state_t&) = delete;
				simulation_state_t& operator=(const simulation_state_t&) = delete;
			};

			std::shared_ptr<grid_object> m_grid;
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
	};
}