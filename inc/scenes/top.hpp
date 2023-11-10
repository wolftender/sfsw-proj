#pragma once
#include "scene.hpp"
#include "function.hpp"
#include "curve.hpp"
#include "grid.hpp"

#include <array>
#include <vector>

namespace mini {
	class top_scene : public scene_base {
		private:
			static constexpr std::size_t MAX_DATA_POINTS = 5000;

			struct simulation_parameters_t {
				float edge_length;
				float cube_density;
				float cube_deviation;
				float int_step;

				simulation_parameters_t() {
					edge_length = 1.0f;
					cube_density = 1.0f;
					cube_deviation = 0.0f;
					int_step = 0.001f;
				}
			};

			bool m_display_cube;
			bool m_display_diagonal;
			bool m_display_plane;
			bool m_display_path;

			int m_last_vp_width, m_last_vp_height;

			simulation_parameters_t m_start_params;
			simulation_parameters_t m_parameters;

			std::size_t m_max_data_points;
			std::size_t m_num_data_points;
			std::array<glm::vec3, MAX_DATA_POINTS> m_path_points;

			std::shared_ptr<curve> m_curve;

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

		private:
			void m_export_data();
			void m_gui_settings();
			void m_gui_viewport();
	};
}