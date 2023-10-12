#include <imgui.h>
#include <imgui_internal.h>

#include "gui.hpp"
#include "scenes/spring.hpp"

namespace mini {
	spring_scene::spring_scene(application_base& app) : scene_base(app),
		m_time(0.0f),
		m_position(0.0f),
		m_friction_coefficient(0.0f),
		m_spring_coefficient(0.0f),
		m_eq_position(0.0f) {

		m_t_data.resize(MAX_DATA_POINTS);
		m_f_data.resize(MAX_DATA_POINTS);
		m_g_data.resize(MAX_DATA_POINTS);
		m_h_data.resize(MAX_DATA_POINTS);

		m_num_data_points = 0;
	}

	void spring_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.45f, nullptr, &dockspace_id);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Spring", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
		ImGui::DockBuilderDockWindow("Simulation Graph", dock_id_bottom);
	}

	void spring_scene::integrate(float delta_time) {
		m_time += delta_time;
		m_push_data_point(m_time / 100.0f, glm::sin(m_time), glm::cos(m_time), 1.0f / glm::exp(m_time));
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

			ImPlot::PlotLine("f(t)", m_t_data.data(), m_f_data.data(), m_num_data_points);
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

			ImPlot::PlotLine("g(t)", m_t_data.data(), m_g_data.data(), m_num_data_points);
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

			ImPlot::PlotLine("h(t)", m_t_data.data(), m_h_data.data(), m_num_data_points);
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