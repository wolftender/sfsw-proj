#include <imgui.h>
#include <imgui_internal.h>

#include "gui.hpp"
#include "scenes/spring.hpp"

namespace mini {
	spring_scene::spring_scene(application_base& app) : scene_base(app),
		m_time(0.0f),
		m_friction_coefficient(0.0f),
		m_spring_coefficient(0.0f),
		m_x0(0.0f), m_dx0(0.0f), m_ddx0(0.0f),
		m_x(0.0f), m_dx(0.0f), m_ddx(0.0f),
		m_w(0.0f), m_dw(0.0f),
		m_h(0.0f), m_dh(0.0f) {

		m_t_data.resize(MAX_DATA_POINTS);
		m_f_data.resize(MAX_DATA_POINTS);
		m_g_data.resize(MAX_DATA_POINTS);
		m_h_data.resize(MAX_DATA_POINTS);

		m_num_data_points = 0;

		// initialize functions
		m_fw = mk_const(5.0f);
		m_fh = mk_const(7.0f);
	}

	void spring_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.45f, nullptr, &dockspace_id);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spring", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
		ImGui::DockBuilderDockWindow("Simulation Graph", dock_id_bottom);
	}

	void spring_scene::integrate(float delta_time) {
		const float t0 = m_time;
		const float h = delta_time;
		const float m = m_mass;
		const float c = m_spring_coefficient;
		const float k = m_friction_coefficient;

		const float dw = m_fw->derivative(t0);
		const float dh = m_fh->derivative(t0);

		// euler method
		const float ddx1 = m_ddx + h * (c * dw - c * m_dx - k * m_dx + dh) / m;
		const float dx1 = m_dx + h * m_ddx;
		const float x1 = m_x + h * m_dx;

		// advance time step
		m_time += delta_time;

		// set new values
		m_ddx = ddx1;
		m_dx = dx1;
		m_x = x1;

		float a = m_fh->value(m_time);
		float b = m_fh->derivative(m_time);

		m_push_data_point(m_time / 100.0f, a, b, 1.0f);
	}

	void spring_scene::render(app_context& context) {
		
	}

	void spring_scene::gui() {
		m_gui_graph();
		m_gui_settings();
		m_gui_viewport();
	}

	void spring_scene::m_gui_graph() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Graph", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		auto width = max.x - min.x;
		auto height = max.y - min.y;

		// render plots
		if (ImPlot::BeginPlot("f(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < 0.2f) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + 0.2f);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("f(t)", m_t_data.data(), m_f_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::SameLine();
		if (ImPlot::BeginPlot("g(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < 0.2f) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + 0.2f);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("g(t)", m_t_data.data(), m_g_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::SameLine();
		if (ImPlot::BeginPlot("h(t)", ImVec2(width * 0.325f, height - 15.0f), ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs)) {
			if (m_t_data[m_num_data_points - 1] - m_t_data[0] < 0.2f) {
				ImPlot::SetupAxis(ImAxis_X1, "t");
				ImPlot::SetupAxisLimits(ImAxis_X1, m_t_data[0], m_t_data[0] + 0.2f);
			} else {
				ImPlot::SetupAxis(ImAxis_X1, "t", ImPlotAxisFlags_AutoFit);
			}

			ImPlot::SetupAxis(ImAxis_Y1, "##y", ImPlotAxisFlags_AutoFit);

			ImPlot::PlotLine("h(t)", m_t_data.data(), m_h_data.data(), static_cast<int>(m_num_data_points));
			ImPlot::EndPlot();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Button("dupa");
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_gui_viewport() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Spring", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Button("dupa");
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void spring_scene::m_push_data_point(float t, float f, float g, float h) {
		if (m_num_data_points < MAX_DATA_POINTS) {
			m_t_data[m_num_data_points] = t;
			m_f_data[m_num_data_points] = f;
			m_g_data[m_num_data_points] = g;
			m_h_data[m_num_data_points] = h;

			m_num_data_points++;
		} else {
			for (std::size_t i = 0; i < m_num_data_points - 1; ++i) {
				m_t_data[i] = m_t_data[i + 1];
				m_f_data[i] = m_f_data[i + 1];
				m_g_data[i] = m_g_data[i + 1];
				m_h_data[i] = m_h_data[i + 1];
			}

			m_t_data[m_num_data_points - 1] = t;
			m_f_data[m_num_data_points - 1] = f;
			m_g_data[m_num_data_points - 1] = g;
			m_h_data[m_num_data_points - 1] = h;
		}
	}
}