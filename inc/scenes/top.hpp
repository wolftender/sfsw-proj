#pragma once
#include "scene.hpp"
#include "function.hpp"
#include "curve.hpp"
#include "grid.hpp"
#include "viewport.hpp"
#include "cube.hpp"

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace mini {
	class top_scene : public scene_base {
		private:
			static constexpr std::size_t MAX_DATA_POINTS = 5000;

			struct simulation_parameters_t {
				float diagonal_length;
				float cube_density;
				float cube_deviation;
				float angular_velocity;
				float int_step;

				simulation_parameters_t() {
					diagonal_length = 1.0f;
					cube_density = 1.0f;
					cube_deviation = 0.0f;
					angular_velocity = 2.0f;
					int_step = 0.001f;
				}
			};

			struct simulation_state_t {
				simulation_parameters_t parameters;

				glm::mat3x3 inertia_tensor;

				glm::vec3 angular_velocity;
				glm::quat rotation;

				simulation_state_t(const simulation_parameters_t & parameters);
				void integrate(float delta_time);
			};

			bool m_display_cube;
			bool m_display_diagonal;
			bool m_display_grid;
			bool m_display_plane;
			bool m_display_path;

			simulation_parameters_t m_start_params;
			simulation_state_t m_state;

			std::size_t m_max_data_points;
			std::size_t m_num_data_points;
			std::array<glm::vec3, MAX_DATA_POINTS> m_path_points;

			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<cube_object> m_cube;
			std::shared_ptr<curve> m_curve;

			viewport_window m_viewport;

		public:
			top_scene(application_base & app);
			~top_scene();

			top_scene(const top_scene&) = delete;
			top_scene& operator=(const top_scene&) = delete;

			void layout(ImGuiID dockspace_id) override;
			void integrate(float delta_time) override;
			void render(app_context& context) override;
			void gui() override;
			void menu() override;

			virtual void on_scroll(double offset_x, double offset_y) override;

		private:
			void m_export_data();
			void m_gui_settings();
			void m_gui_viewport();
	};
}