#pragma once
#include "scene.hpp"
#include "function.hpp"

namespace mini {
	class spring_scene : public scene_base {
		public:
			static constexpr std::size_t MAX_DATA_POINTS = 2000;

		private:
			float m_friction_coefficient; // k
			float m_spring_coefficient; // c
			float m_mass; // m
			float m_time; // t

			float m_x0, m_dx0, m_ddx0; // initial values
			float m_x, m_dx, m_ddx; // position, veloctiy, accel

			float m_w, m_dw; // equilibrium pos
			float m_h, m_dh; // external forces

			f_func m_fw, m_fh;

			std::vector<float> m_t_data;
			std::vector<float> m_f_data;
			std::vector<float> m_g_data;
			std::vector<float> m_h_data;

			std::size_t m_num_data_points;

		public:
			spring_scene(application_base& app);

			virtual void layout(ImGuiID dockspace_id) override;
			virtual void integrate(float delta_time) override;
			virtual void render(app_context& context) override;
			virtual void gui() override;

		private:
			void m_gui_graph();
			void m_gui_settings();
			void m_gui_viewport();

			void m_push_data_point(float t, float f, float g, float h);
	};
}