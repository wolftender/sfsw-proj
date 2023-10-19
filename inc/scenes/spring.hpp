#pragma once
#include "scene.hpp"
#include "function.hpp"
#include "curve.hpp"
#include "grid.hpp"

namespace mini {
	class spring_scene : public scene_base {
		public:
			static constexpr std::size_t MAX_DATA_POINTS = 2000;

		private:
			float m_friction_coefficient; // k
			float m_spring_coefficient; // c
			float m_mass; // m
			float m_time; // t

			float m_k0, m_c0, m_m0;

			float m_x0, m_dx0, m_ddx0; // initial values
			float m_x, m_dx, m_ddx; // position, veloctiy, accel

			float m_distance;
			float m_spring_length;

			f_func m_fw, m_fh;
			int m_last_vp_width, m_last_vp_height;

			bool m_paused;

			std::string m_w_expression;
			std::string m_h_expression;

			bool m_is_w_error;
			bool m_is_h_error;

			std::string m_w_error;
			std::string m_h_error;

			std::vector<float> m_t_data;
			std::vector<float> m_f_data;
			std::vector<float> m_g_data;
			std::vector<float> m_h_data;

			std::size_t m_num_data_points;

			// drawable objects
			std::shared_ptr<grid_object> m_grid;
			std::shared_ptr<curve> m_spring_curve;
			std::shared_ptr<curve> m_mass_object;

		public:
			spring_scene(application_base& app);

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;
			virtual void on_scroll(double offset_x, double offset_y) override;

		private:
			std::shared_ptr<curve> m_make_wire_simplex(std::shared_ptr<shader_program> shader) const;
			std::shared_ptr<curve> m_make_helix_curve(std::shared_ptr<shader_program> shader) const;

			void m_gui_graph();
			void m_gui_settings();
			void m_gui_viewport();

			void m_start_simulation();
			void m_push_data_point(float t, float f, float g, float h);
	};
}