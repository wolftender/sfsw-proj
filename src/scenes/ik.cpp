#include <iostream>
#include <deque>
#include <glm/gtc/matrix_transform.hpp>

#include "gui.hpp"
#include "camera.hpp"
#include "scenes/ik.hpp"

namespace mini {
	inline bool intersect_segment(
		float x1, float y1,
		float x2, float y2,
		float x3, float y3,
		float x4, float y4,
		float& t, float& u) {

		float den = ((x1 - x2) * (y3 - y4)) - ((y1 - y2) * (x3 - x4));

		if (den == 0.0f) {
			t = u = 0.0f;
			return false;
		}

		float num_t = ((x1 - x3) * (y3 - y4)) - ((y1 - y3) * (x3 - x4));
		float num_u = ((x1 - x3) * (y1 - y2)) - ((y1 - y3) * (x1 - x2));

		// 0 < num_t/den < 1
		if (num_t * den < 0.0f || num_u * den < 0.0f) {
			t = u = 0.0f;
			return false;
		}

		num_u = fabsf(num_u);
		num_t = fabsf(num_t);
		den = fabsf(den);

		if (num_u > den || num_t > den) {
			t = u = 0.0f;
			return false;
		}

		t = num_t / den;
		u = num_u / den;

		return true;
	}

	inline bool intersect_segment(
		const glm::vec2& s1,
		const glm::vec2& e1,
		const glm::vec2& s2,
		const glm::vec2& e2,
		glm::vec2& p
	) {
		float t, u;
		if (!intersect_segment(s1.x, s1.y, e1.x, e1.y, s2.x, s2.y, e2.x, e2.y, t, u)) {
			return false;
		}

		p = glm::mix(s1, e1, t);
		return true;
	}

	ik_scene::configuration_space_t::configuration_space_t(int rx, int ry) {
		res_x = rx;
		res_y = ry;

		data.resize(rx * ry * 3);
		std::fill(data.begin(), data.end(), 0);

		texture = std::make_shared<mini::texture>(res_x, res_y, data.data(), GL_RGB);
	}

	bool ik_scene::configuration_space_t::is_collision(int x, int y) const {
		int index = y * res_x + x;
		int base = 3 * index;

		return (data[base + 0] != 0);
	}

	void ik_scene::configuration_space_t::set_collision(int x, int y, bool collision) {
		int index = y * res_x + x;
		int base = 3 * index;

		unsigned char val = (collision) ? 255 : 0;
		data[base + 0] = val;
	}

	void ik_scene::configuration_space_t::update_texture() {
		texture->update(data.data());
	}

	void ik_scene::configuration_space_t::clear_texture() {
		std::fill(data.begin(), data.end(), 0);
	}

	bool ik_scene::configuration_space_t::find_path(
		const robot_configuration_t& start, 
		const robot_configuration_t& end, 
		std::vector<robot_configuration_t>& path) {
		
		if (dist.size() != static_cast<std::size_t>(res_x * res_y)) {
			dist.resize(res_x * res_y);
		}

		constexpr int INF_DIST = 999999;
		std::fill(dist.begin(), dist.end(), INF_DIST);

		const auto config_to_coords = [&](const robot_configuration_t & config) -> std::pair<int, int> {
			constexpr float pi = glm::pi<float>();
			float alpha = 0.5f * (config.theta1 + pi) / pi;
			float beta = 0.5f * (config.theta2 + pi) / pi;

			int x = static_cast<int>(roundf(alpha * res_x));
			int y = static_cast<int>(roundf(beta * res_y));

			return {x,y};
		};

		const auto add_wrapped = [&](const std::pair<int, int>& cell, int dx, int dy) -> std::pair<int, int> {
			std::pair<int,int> val = cell;

			val.first += dx;
			val.second += dy;

			if (val.first >= res_x) {
				val.first = val.first % res_x;
			} else if (val.first < 0) {
				val.first = res_x + (val.first % res_x);
			}

			if (val.second >= res_y) {
				val.second = val.second % res_y;
			} else if (val.second < 0) {
				val.second = res_y + (val.second % res_y);
			}

			return val;
		};

		auto start_cell = config_to_coords(start);
		auto end_cell = config_to_coords(end);

		std::deque<std::pair<int,int>> s;
		s.push_back(start_cell);

		int idx_start = start_cell.second * res_x + start_cell.first;
		//int idx_end = end_cell.second * res_x + end_cell.first;

		dist[idx_start] = 0;

		const auto handle_neighbor = [&](const std::pair<int, int>& cell, const std::pair<int, int>& neighbor) -> void {
			if (is_collision(neighbor.first, neighbor.second)) {
				return;
			}

			int idx1 = cell.second * res_x + cell.first;
			int idx2 = neighbor.second * res_x + neighbor.first;

			int d = dist[idx1] + 1;
			if (dist[idx2] > d) {
				dist[idx2] = d;
				s.push_front(neighbor);

				data[3*idx2 + 1] = glm::min(d,255);
			}
		};

		while (!s.empty()) {
			auto current = s.back();
			s.pop_back();

			auto up = add_wrapped(current, 0, -1);
			auto down = add_wrapped(current, 0, 1);
			auto left = add_wrapped(current, -1, 0);
			auto right = add_wrapped(current, 1, 0);

			handle_neighbor(current, up);
			handle_neighbor(current, down);
			handle_neighbor(current, left);
			handle_neighbor(current, right);
		}

		const auto convert_to_config = [&](const std::pair<int, int>& cell) -> robot_configuration_t {
			constexpr float pi = glm::pi<float>();
			const float rx = static_cast<float>(res_x);
			const float ry = static_cast<float>(res_y);
			const float fx = static_cast<float>(cell.first);
			const float fy = static_cast<float>(cell.second);
			const float alpha = ((fx / rx) * 2.0f * pi) - pi;
			const float beta = ((fy / ry) * 2.0f * pi) - pi;

			return {alpha, beta};
		};

		std::vector<std::pair<int,int>> inv_path;
		auto curr = end_cell;

		inv_path.push_back(curr);
		while (curr != start_cell) {
			int idx = curr.second * res_x + curr.first;
			if (dist[idx] >= INF_DIST) {
				return false;
			}

			auto up = add_wrapped(curr, 0, -1);
			auto down = add_wrapped(curr, 0, 1);
			auto left = add_wrapped(curr, -1, 0);
			auto right = add_wrapped(curr, 1, 0);

			int up_idx = up.second * res_x + up.first;
			int down_idx = down.second * res_x + down.first;
			int left_idx = left.second * res_x + left.first;
			int right_idx = right.second * res_x + right.first;

			int d = dist[idx];
			if (dist[up_idx] == d - 1) {
				curr = up;
				data[3 * up_idx + 2] = 255;
				inv_path.push_back(curr);
			} else if (dist[down_idx] == d - 1) {
				curr = down;
				data[3 * down_idx + 2] = 255;
				inv_path.push_back(curr);
			} else if (dist[left_idx] == d - 1) {
				curr = left;
				data[3 * left_idx + 2] = 255;
				inv_path.push_back(curr);
			} else if (dist[right_idx] == d - 1) {
				curr = right;
				data[3 * right_idx + 2] = 255;
				inv_path.push_back(curr);
			} else {
				return false;
			}
		}

		path.resize(inv_path.size());
		int i = 0;
		for (auto it = inv_path.rbegin(); it != inv_path.rend(); ++it) {
			path[i++] = convert_to_config(*it);
		}

		update_texture();
		return true;
	}

	ik_scene::ik_scene(application_base& app) : 
		scene_base(app),
		m_conf(360, 360),
		m_arm1_len(5.0f),
		m_arm2_len(6.0f),
		m_mouse_tool_id(0),
		m_alt_solution_start(false),
		m_alt_solution_end(false),
		m_last_vp_width(0),
		m_last_vp_height(0),
		m_mouse_in_viewport(false),
		m_viewport_focus(false),
		m_is_start_ok(false),
		m_is_end_ok(false),
		m_is_start_collision(false),
		m_is_end_collision(false),
		m_show_path_error(false),
		m_loop_animation(false),
		m_vp_mouse_offset{0, 0},
		m_start_point{0.0f, 11.0f},
		m_end_point{11.0f, 0.0f},
		m_mouse_mode(mouse_mode_t::start_config),
		m_obstacle_start{0.0f, 0.0f},
		m_is_adding_obstacle(false),
		m_selected_obstacle(-1),
		m_animation_playing(false),
		m_animation_timer(0.0f) {
		
		app.get_context().set_clear_color({ 0.95f, 0.95f, 0.95f });

		auto xy_grid_shader = app.get_store().get_shader("grid_xy");
		auto line_shader = get_app().get_store().get_shader("line");
		auto billboard_shader = get_app().get_store().get_shader("obstacle");

		if (xy_grid_shader) {
			m_grid = std::make_shared<grid_object>(xy_grid_shader);
		}

		if (line_shader) {
			m_robot_arm_start = m_build_robot_arm(line_shader);
			m_robot_arm_end = m_build_robot_arm(line_shader);
			m_robot_arm_curr = m_build_robot_arm(line_shader);

			m_robot_arm_start->set_color({ 0.85f, 0.3f, 0.3f, 1.0f });
			m_robot_arm_end->set_color({ 0.3f, 0.3f, 0.85f, 1.0f });
		}

		if (billboard_shader) {
			m_billboard = std::make_shared<plane_object>(billboard_shader);
			m_billboard->set_color_tint(glm::vec4{0.0558, 0.595, 0.930, 1.0f});
		}

		constexpr auto pi = glm::pi<float>();
		m_start_config.theta1 = 0.25f * pi;
		m_start_config.theta2 = 0.25f * pi;

		m_end_config = m_start_config;
		m_current_config = m_start_config;

		auto camera = std::make_unique<ortho_camera>();
		camera->set_top(-12.0f);
		camera->set_bottom(12.0f);
		camera->set_left(-12.0f);
		camera->set_right(12.0f);

		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));

		m_solve_start_ik();
		m_solve_end_ik();

		m_rebuild_configuration();
	}

	std::shared_ptr<segments_array> ik_scene::m_build_robot_arm(std::shared_ptr<shader_program> line_shader) const {
		auto arm = std::make_shared<segments_array>(line_shader, 3);

		arm->add_segment(0, 1);
		arm->add_segment(1, 2);
		arm->add_segment(2, 3);

		arm->set_line_width(4.0f);
		arm->set_color({0.0f, 0.0f, 0.0f, 1.0f});

		return arm;
	}

	void ik_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Inverse Kinematics", dockspace_id);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_right);
		ImGui::DockBuilderDockWindow("Configuration Space", dock_id_right);
	}

	void ik_scene::integrate(float delta_time) {
		m_configure_robot_arm(m_robot_arm_start, m_start_config);
		m_configure_robot_arm(m_robot_arm_curr, m_current_config);
		m_configure_robot_arm(m_robot_arm_end, m_end_config);

		if (m_is_adding_obstacle) {
			if (!m_viewport_focus) {
				m_is_adding_obstacle = false;
			}

			auto mouse_world = m_get_mouse_world();

			float min_x = glm::min(mouse_world.x, m_obstacle_start.x);
			float min_y = glm::min(mouse_world.y, m_obstacle_start.y);

			float max_x = glm::max(mouse_world.x, m_obstacle_start.x);
			float max_y = glm::max(mouse_world.y, m_obstacle_start.y);

			float width = max_x - min_x;
			float height = max_y - min_y;

			m_curr_obstacle.position = {min_x, min_y};
			m_curr_obstacle.size = {width, height};
		}

		if (m_animation_playing) {
			if (m_path.size() == 0) {
				m_animation_playing = false;
			}

			m_animation_timer += 0.25f * delta_time;
			if (m_animation_timer >= 1.0f) {
				if (m_loop_animation) {
					m_animation_timer = 0.0f;
				} else {
					m_animation_playing = false;
					m_animation_timer = 1.0f;
				}
			}

			int anim_frame = static_cast<int>(m_animation_timer * static_cast<float>(m_path.size() - 1));
			m_current_config = m_path[anim_frame];
		}

		m_check_collisions();
	}

	void ik_scene::render(app_context& context) {
		auto& camera = get_app().get_context().get_camera();
		camera.set_position({ 0.0f, 0.0f, 20.0f });
		camera.set_target({ 0.0f, 0.0f, 0.0f });

		constexpr float pi = glm::pi<float>();

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			grid_model = glm::rotate(grid_model, pi * 0.5f, { 1.0f, 0.0f, 0.0f });

			context.draw(m_grid, grid_model);
		}

		const auto draw_obstacle = [&](const obstacle_t& obstacle) {
			auto obstacle_model = glm::mat4x4(1.0f);
			glm::vec3 translation = { obstacle.position.x, -obstacle.position.y, 0.0f };
			glm::vec3 scale = { obstacle.size.x * 0.5f, -obstacle.size.y * 0.5f, 1.0f };

			translation = translation + scale;

			obstacle_model = glm::translate(obstacle_model, translation);
			obstacle_model = glm::scale(obstacle_model, scale);

			context.draw(m_billboard, obstacle_model);
		};

		if (m_billboard) {
			if (m_is_adding_obstacle) {
				draw_obstacle(m_curr_obstacle);	
			}

			for (const auto& obstacle : m_obstacles) {
				draw_obstacle(obstacle);
			}
		}

		auto arm_model = glm::mat4x4(1.0f);
		context.draw(m_robot_arm_start, arm_model);
		context.draw(m_robot_arm_end, arm_model);
		
		if (m_animation_playing) {
			context.draw(m_robot_arm_curr, arm_model);
		}
	}

	void ik_scene::gui() {
		m_gui_settings();
		m_gui_parameters();
		m_gui_viewport();
	}

	void ik_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void ik_scene::on_mouse_button(int button, int action, int mods) {
		if (!m_viewport_focus || !m_mouse_in_viewport) {
			return;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			bool is_press = (action == GLFW_PRESS);
			bool is_release = (action == GLFW_RELEASE);

			auto world_pos = m_get_mouse_world();

			if (is_press) {
				m_handle_mouse_click(world_pos.x, world_pos.y);
			} else if (is_release) {
				m_handle_mouse_release(world_pos.x, world_pos.y);
			}
		}
	}

	void ik_scene::on_key_event(int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS && key == GLFW_KEY_LEFT_ALT) {
			m_mouse_tool_id = (m_mouse_tool_id + 1) % 3;
			m_mode_changed();
		}
	}

	void ik_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool length_changed = false;

			gui::prefix_label("Arm 1 Len. : ", 250.0f);
			length_changed = ImGui::InputFloat("##ik_arm1_len", &m_arm1_len) || length_changed;

			gui::prefix_label("Arm 2 Len. : ", 250.0f);
			length_changed = ImGui::InputFloat("##ik_arm2_len", &m_arm2_len) || length_changed;

			if (length_changed) {
				m_length_changed();
			}

			gui::prefix_label("Alt. Solution S. : ", 250.0f);
			if (ImGui::Checkbox("##ik_alt_sol_s", &m_alt_solution_start)) {
				m_solve_start_ik();
				m_solve_end_ik();
			}

			gui::prefix_label("Alt. Solution E. : ", 250.0f);
			if (ImGui::Checkbox("##ik_alt_sol_e", &m_alt_solution_end)) {
				m_solve_start_ik();
				m_solve_end_ik();
			}

			constexpr const char* mouse_modes[] = { "Select Start", "Select End", "Add Obstacle" };
			gui::prefix_label("Mouse Tool:", 250.0f);

			if (ImGui::Combo("##ik_tool", &m_mouse_tool_id, mouse_modes, 3)) {
				m_mode_changed();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Start Point")) {
				bool start_changed = false;

				if (!m_is_start_ok) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					ImGui::Text("This point is incorrect!");
					ImGui::PopStyleColor();
				}

				if (m_is_start_collision) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					ImGui::Text("This configuration is colliding!");
					ImGui::PopStyleColor();
				}

				gui::prefix_label("Start X:", 250.0f);
				start_changed = ImGui::InputFloat("##ik_start_x", &m_start_point.x) || start_changed;

				gui::prefix_label("Start Y:", 250.0f);
				start_changed = ImGui::InputFloat("##ik_start_y", &m_start_point.y) || start_changed;

				if (start_changed) {
					m_solve_start_ik();
				}

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("End Point")) {
				bool end_changed = false;

				if (!m_is_end_ok) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					ImGui::Text("This point is incorrect!");
					ImGui::PopStyleColor();
				}

				if (m_is_end_collision) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					ImGui::Text("This configuration is colliding!");
					ImGui::PopStyleColor();
				}

				gui::prefix_label("End X:", 250.0f);
				end_changed = ImGui::InputFloat("##ik_end_x", &m_end_point.x) || end_changed;

				gui::prefix_label("End Y:", 250.0f);
				end_changed = ImGui::InputFloat("##ik_end_y", &m_end_point.y) || end_changed;

				if (end_changed) {
					m_solve_end_ik();
				}

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Pathfinding")) {
				gui::prefix_label("Loop Anim. :", 250.0f);
				ImGui::Checkbox("##ik_loop_anim", &m_loop_animation);
				ImGui::NewLine();

				if (ImGui::Button("Find Path")) {
					m_rebuild_configuration();
					m_show_path_error = !m_conf.find_path(m_start_config, m_end_config, m_path);

					if (!m_show_path_error) {
						m_animation_playing = true;
						m_animation_timer = 0.0f;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Play Anim")) {
					if (m_path.size() > 0) {
						m_animation_playing = true;
						m_animation_timer = 0.0f;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Stop Anim")) {
					m_animation_playing = false;
				}

				ImGui::TreePop();
			}

			if (m_show_path_error) {
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::Text("There is no path between given configurations!");
				ImGui::PopStyleColor();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Obstacles")) {
				if (ImGui::BeginListBox("##objectlist", ImVec2(-1.0f, 200.0f))) {
					ImGuiListClipper clipper;
					clipper.Begin(m_obstacles.size());

					while (clipper.Step()) {
						int num_obstacles = static_cast<int>(m_obstacles.size());
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && i < num_obstacles; ++i) {
							std::string full_name;
							bool selected = (i == m_selected_obstacle);

							if (!selected) {
								full_name = "obstacle #" + std::to_string(i);
							} else {
								full_name = "*obstacle #" + std::to_string(i);
							}

							if (ImGui::Selectable(full_name.c_str(), &selected)) {
								m_selected_obstacle = i;
							}
						}
					}

					clipper.End();
					ImGui::EndListBox();
				}

				if (m_selected_obstacle < 0) {
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);					
				} else {
					auto& obstacle = m_obstacles[m_selected_obstacle];
					gui::prefix_label("Obstacle X:", 250.0f);
					ImGui::SliderFloat("##ik_obstacle_x", &obstacle.position.x, -20.0f, 20.0f);
					gui::prefix_label("Obstacle Y:", 250.0f);
					ImGui::SliderFloat("##ik_obstacle_y", &obstacle.position.y, -20.0f, 20.0f);
					gui::prefix_label("Obstacle Width:", 250.0f);
					ImGui::SliderFloat("##ik_obstacle_w", &obstacle.size.x, 0.1f, 20.0f);
					gui::prefix_label("Obstacle Height:", 250.0f);
					ImGui::SliderFloat("##ik_obstacle_h", &obstacle.size.y, 0.1f, 20.0f);

					obstacle.size.x = glm::max(0.1f, obstacle.size.x);
					obstacle.size.y = glm::max(0.1f, obstacle.size.y);
				}

				bool deleted = false;

				if (ImGui::Button("Delete", ImVec2(-1.0f, 24.0f))) {
					m_obstacles.erase(m_obstacles.begin() + m_selected_obstacle);
					m_rebuild_configuration();
					m_selected_obstacle = -1;
					deleted = true;
				}

				if (m_selected_obstacle < 0 && !deleted) {
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				ImGui::TreePop();
			}

			ImGui::NewLine();
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void ik_scene::m_gui_viewport() {
		auto& context = get_app().get_context();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(320, 240));
		ImGui::Begin("Inverse Kinematics", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_Once);

		if (ImGui::IsWindowFocused()) {
			m_viewport_focus = true;
		}
		else {
			m_viewport_focus = false;
		}

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto window_pos = ImGui::GetWindowPos();

		int width = static_cast<int>(max.x - min.x);
		int height = static_cast<int>(max.y - min.y);

		const offset_t& mouse_offset = get_app().get_mouse_offset();
		m_vp_mouse_offset.x = mouse_offset.x - static_cast<int> (min.x + window_pos.x);
		m_vp_mouse_offset.y = mouse_offset.y - static_cast<int> (min.y + window_pos.y);

		if (m_vp_mouse_offset.x < 0 || m_vp_mouse_offset.x > width || m_vp_mouse_offset.y < 0 || m_vp_mouse_offset.y > height) {
			m_mouse_in_viewport = false;
		}
		else {
			m_mouse_in_viewport = true;
		}

		if ((width != m_last_vp_width || height != m_last_vp_height) && width > 8 && height > 8) {
			video_mode_t mode(width, height);

			context.set_video_mode(mode);

			m_last_vp_width = width;
			m_last_vp_height = height;
		} else {
			ImGui::ImageButton(
				reinterpret_cast<ImTextureID>(context.get_front_buffer()),
				ImVec2(width, height), ImVec2(0, 0), ImVec2(1, 1), 0);
		}

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void ik_scene::m_gui_parameters() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Configuration Space", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();

		int width = static_cast<int> (max.x - min.x);
		//int height = static_cast<int> (max.y - min.y);

		ImGui::ImageButton(reinterpret_cast<ImTextureID> (
			m_conf.texture->get_handle()), ImVec2(width, width), ImVec2(0, 0), ImVec2(1, 1), 0);

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void ik_scene::m_handle_mouse_click(float x, float y) {
		switch (m_mouse_mode) {
			case mouse_mode_t::start_config:
				m_start_point = {x, y};
				m_solve_start_ik();
				break;

			case mouse_mode_t::end_config:
				m_end_point = { x, y };
				m_solve_end_ik();
				break;

			case mouse_mode_t::obstacle:
				m_is_adding_obstacle = true;
				m_curr_obstacle.position = {x, y};
				m_curr_obstacle.size = {0.0f, 0.0f};
				m_obstacle_start = {x, y};
				break;
		}
	}

	void ik_scene::m_handle_mouse_release(float x, float y) {
		if (m_is_adding_obstacle) {
			m_obstacles.push_back(m_curr_obstacle);
			m_is_adding_obstacle = false;

			m_rebuild_configuration();
		}
	}

	bool ik_scene::m_solve_arm_ik(robot_configuration_t& config, float x, float y, bool alt) const {
		float l1 = m_arm1_len;
		float l2 = m_arm2_len;

		float dsq = x * x + y * y;
		float l1sq = l1 * l1;
		float l2sq = l2 * l2;
		float d = glm::sqrt(dsq);

		float cx = (l1sq + dsq - l2sq) / (2.0f * d);
		float cysq = l1sq - cx * cx;

		if (cysq >= 0.0f) {
			float cy1 = +glm::sqrt(cysq);
			float cy2 = -glm::sqrt(cysq);

			glm::vec2 v1 = glm::normalize(glm::vec2{ x, y });
			glm::vec2 v2 = { -v1.y, v1.x };

			glm::vec2 x1_pos = (alt) ? 
				cx * v1 + cy2 * v2 : 
				cx * v1 + cy1 * v2;

			float alpha = atan2f(x1_pos.y, x1_pos.x);
			glm::mat2 M10 = glm::mat2{
				 cosf(-alpha), sinf(-alpha),
				-sinf(-alpha), cosf(-alpha)
			};
			glm::vec2 x2p = M10 * glm::vec2{ x - x1_pos.x, y - x1_pos.y };
			float beta = atan2f(x2p.y, x2p.x);

			config.theta1 = -alpha;
			config.theta2 = -beta;

			return true;
		}

		return false;
	}

	void ik_scene::m_configure_robot_arm(
		std::shared_ptr<segments_array>& arm, 
		const robot_configuration_t& config) {

		// calculate robot arms
		glm::vec3 origin{ 0.0f, 0.0f, 1.0f };
		const float a = config.theta1;
		const float b = config.theta2;

		const float c1 = glm::cos(a);
		const float s1 = glm::sin(a);
		const float c2 = glm::cos(b);
		const float s2 = glm::sin(b);
		const float l1 = m_arm1_len;
		const float l2 = m_arm2_len;

		glm::mat3 F01 = {
			 c1,      s1,      0.0f,
			-s1,	  c1,      0.0f,
			 l1 * c1, l1 * s1, 1.0f
		};

		glm::vec3 p1 = F01 * origin;

		glm::mat3 F12 = {
			 c2,      s2,      0.0f,
			-s2,	  c2,      0.0f,
			 l2 * c2, l2 * s2, 1.0f
		};

		glm::vec3 p2 = F01 * F12 * origin;
		p1.z = p2.z = origin.z = 0.0f;

		arm->update_point(0, origin);
		arm->update_point(1, p1);
		arm->update_point(2, p2);
		arm->rebuild_buffers();
	}

	glm::vec2 ik_scene::m_get_mouse_world() const {
		float mx = m_vp_mouse_offset.x;
		float my = m_vp_mouse_offset.y;

		float vp_width = m_last_vp_width;
		float vp_height = m_last_vp_height;

		float sx = (mx / vp_width) * 2.0f - 1.0f;
		float sy = 1.0f - (my / vp_height) * 2.0f;

		const auto& camera = get_app().get_context().get_camera();
		glm::vec4 screen_pos = { sx, sy, 1.0f, 1.0f };

		glm::mat4x4 view_proj_inv = camera.get_view_inverse() * camera.get_projection_inverse();
		glm::vec4 world_pos = view_proj_inv * screen_pos;

		return world_pos;
	}

	inline bool ik_scene::m_collides(float alpha, float beta) const {
		const float l1 = m_arm1_len;
		const float l2 = m_arm2_len;

		const glm::vec2 p0 = { 0.0f, 0.0f };
		const glm::vec2 p1 = { l1 * cosf(alpha), l1 * sinf(alpha) };
		const glm::vec2 p2 = {
			l2 * cosf(alpha + beta) + l1 * cosf(alpha),
			l2 * sinf(alpha + beta) + l1 * sinf(alpha),
		};

		bool collision = false;
		for (const auto& obstacle : m_obstacles) {
			const glm::vec2 q1 = { obstacle.position.x, -obstacle.position.y };
			const glm::vec2 q2 = { q1.x + obstacle.size.x, q1.y };
			const glm::vec2 q3 = { q1.x + obstacle.size.x, q1.y - obstacle.size.y };
			const glm::vec2 q4 = { q1.x, q1.y - obstacle.size.y };
			glm::vec2 _r;

			collision = collision || intersect_segment(p0, p1, q1, q2, _r);
			collision = collision || intersect_segment(p0, p1, q2, q3, _r);
			collision = collision || intersect_segment(p0, p1, q3, q4, _r);
			collision = collision || intersect_segment(p0, p1, q4, q1, _r);

			collision = collision || intersect_segment(p1, p2, q1, q2, _r);
			collision = collision || intersect_segment(p1, p2, q2, q3, _r);
			collision = collision || intersect_segment(p1, p2, q3, q4, _r);
			collision = collision || intersect_segment(p1, p2, q4, q1, _r);

			if (collision) {
				break;
			}
		}

		return collision;
	}

	void ik_scene::m_check_collisions() {
		m_is_start_collision = m_collides(m_start_config.theta1, m_start_config.theta2);
		m_is_end_collision = m_collides(m_end_config.theta1, m_end_config.theta2);
	}

	void ik_scene::m_rebuild_configuration() {
		m_conf.clear_texture();
		m_path.clear();

		m_check_collisions();

		constexpr float pi = glm::pi<float>();
		const float step_x = 2.0f * pi / static_cast<float>(m_conf.res_x);
		const float step_y = 2.0f * pi / static_cast<float>(m_conf.res_y);

		for (int x = 0; x < m_conf.res_x; ++x) {
			for (int y = 0; y < m_conf.res_y; ++y) {
				const float alpha = step_x * x - pi;
				const float beta = step_y * y - pi;

				auto collision = m_collides(alpha, beta);
				m_conf.set_collision(x, y, collision);
			}
		}

		m_conf.update_texture();
	}

	void ik_scene::m_mode_changed() {
		mouse_mode_t new_mode = mouse_mode_t::start_config;
		switch (m_mouse_tool_id) {
			case 0: new_mode = mouse_mode_t::start_config; break;
			case 1: new_mode = mouse_mode_t::end_config; break;
			case 2: new_mode = mouse_mode_t::obstacle; break;
		}

		m_mouse_mode = new_mode;
	}

	void ik_scene::m_length_changed() {
		const float l1 = m_arm1_len;
		const float l2 = m_arm2_len;

		const auto apply_changes = [&](const robot_configuration_t& config) -> glm::vec2 {
			const float a = config.theta1;
			const float b = config.theta2;

			const float c1 = glm::cos(a);
			const float s1 = glm::sin(a);
			const float c2 = glm::cos(b);
			const float s2 = glm::sin(b);

			glm::mat3 F01 = {
				 c1,      s1,      0.0f,
				-s1,	  c1,      0.0f,
				 l1 * c1, l1 * s1, 1.0f
			};

			glm::mat3 F12 = {
				 c2,      s2,      0.0f,
				-s2,	  c2,      0.0f,
				 l2 * c2, l2 * s2, 1.0f
			};

			glm::vec3 p2 = F01 * F12 * glm::vec3{ 0.0f, 0.0f, 1.0f };
			return {p2.x, -p2.y};
		};

		m_start_point = apply_changes(m_start_config);
		m_end_point = apply_changes(m_end_config);
		m_rebuild_configuration();
	}

	void ik_scene::m_solve_start_ik() {
		m_is_start_ok = m_solve_arm_ik(m_start_config, m_start_point.x, m_start_point.y, m_alt_solution_start);
		m_check_collisions();
	}

	void ik_scene::m_solve_end_ik() {
		m_is_end_ok = m_solve_arm_ik(m_end_config, m_end_point.x, m_end_point.y, m_alt_solution_end);
		m_check_collisions();
	}
}