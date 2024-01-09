#include "gui.hpp"
#include "scenes/puma.hpp"

#include <glm/gtx/vector_angle.hpp>

namespace mini {
	constexpr auto AXIS_X = glm::vec3{ 1.0f, 0.0f, 0.0f };
	constexpr auto AXIS_Y = glm::vec3{ 0.0f, 1.0f, 0.0f };
	constexpr auto AXIS_Z = glm::vec3{ 0.0f, 0.0f, 1.0f };
	constexpr auto PI = glm::pi<float>();
	constexpr auto HPI = 0.5f * PI;

	inline float deg_to_rad(float deg) {
		constexpr auto pi = glm::pi<float>();
		return (deg / 180.0f) * pi;
	}

	inline float rad_to_deg(float rad) {
		constexpr auto pi = glm::pi<float>();
		return (rad / pi) * 180.0f;
	}

	puma_scene::puma_scene(application_base& app) : 
		scene_base(app), 
		m_context1(app.get_context()),
		m_context2(video_mode_t(600, 400)), 
		m_viewport1(app, m_context1, "Config Interp."),
		m_viewport2(app, m_context2, "Effector Interp."),
		m_effector1{ 0.0f, 0.0f, 0.0f },
		m_effector2{ 0.0f, 0.0f, 0.0f },
		m_distance(10.0f),
		m_manual_control(false),
		m_follow_effector(false),
		m_loop_animation(false),
		m_flashlight(false),
		m_debug_points(false),
		m_begin_changed(false),
		m_end_changed(false),
		m_anim_time(0.0f),
		m_anim_speed(0.5f),
		m_anim_active(false),
		m_anim_paused(false) {

		m_context1.set_clear_color({ 0.75f, 0.75f, 0.9f });
		m_context2.set_clear_color({ 0.75f, 0.75f, 0.9f });

		auto grid_shader = get_app().get_store().get_shader("grid_xz");
		auto puma_shader = get_app().get_store().get_shader("puma");
		auto point_shader = get_app().get_store().get_shader("point");
		auto line_shader = get_app().get_store().get_shader("line");

		if (grid_shader) {
			m_grid = std::make_shared<grid_object>(grid_shader);
		}

		if (line_shader) {
			m_debug_lines1 = m_make_debug_mesh(line_shader);
			m_debug_lines2 = m_make_debug_mesh(line_shader);
		}

		if (point_shader) {
			m_point_object = std::make_shared<billboard_object>(point_shader);
			m_point_object->set_color_tint({ 1.0f, 0.0f, 0.0f, 1.0f });
			m_point_object->set_size({ 16.0f, 16.0f });
		}

		if (puma_shader) {
			m_effector_mesh = m_make_effector_mesh();
			m_arm_mesh = triangle_mesh::make_cylinder(0.3f, 1.0f, 50, 20);
			m_joint_mesh = triangle_mesh::make_cylinder(0.5f, 0.7f, 50, 20);

			m_effector_model_x = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_y = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_effector_model_z = std::make_shared<model_object>(m_effector_mesh, puma_shader);
			m_arm_model = std::make_shared<model_object>(m_arm_mesh, puma_shader);
			m_joint_model = std::make_shared<model_object>(m_joint_mesh, puma_shader);

			m_effector_model_x->set_surface_color({ 1.0f, 0.0f, 0.0f, 1.0f });
			m_effector_model_y->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_effector_model_z->set_surface_color({ 0.0f, 0.0f, 1.0f, 1.0f });
			m_arm_model->set_surface_color({ 0.0f, 1.0f, 0.0f, 1.0f });
			m_joint_model->set_surface_color({0.0f, 0.0f, 1.0f, 1.0f});

			m_debug_mesh = triangle_mesh::make_cube_mesh();
			m_debug_model = std::make_shared<model_object>(m_debug_mesh, puma_shader);
			m_debug_model->set_surface_color({ 0.0f, 0.0f, 1.0f, 1.0f });
		}

		// no uninitialized solutions
		m_solve_ik(m_start_config, m_start_meta, m_puma_start, ik_mode_t::default_solution);
		m_solve_ik(m_end_config, m_end_meta, m_puma_end, ik_mode_t::default_solution);
		m_solve_ik(m_config1, m_meta1, m_puma_start, ik_mode_t::default_solution);
		m_solve_ik(m_config2, m_meta2, m_puma_start, ik_mode_t::default_solution);

		auto camera = std::make_unique<default_camera>();
		camera->video_mode_change(get_app().get_context().get_video_mode());
		get_app().get_context().set_camera(std::move(camera));
	}

	void puma_scene::layout(ImGuiID dockspace_id) {
		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.4f, nullptr, &dockspace_id);
		auto dock_id_top_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);

		ImGui::DockBuilderDockWindow("Config Interp.", dockspace_id);
		ImGui::DockBuilderDockWindow("Effector Interp.", dock_id_top_left);
		ImGui::DockBuilderDockWindow("Simulation Settings", dock_id_bottom);
	}

	inline float angle_lerp(float start, float end, float t) {
		float d = end - start;

		if (abs(d) > PI) {
			if (start > 0.0f) {
				end = 2.0f * PI + end;
			} else {
				end = -2.0f * PI + end;
			}
		}

		return glm::mix(start, end, t);
	}

	void puma_scene::integrate(float delta_time) {
		m_viewport1.update(delta_time);
		m_viewport2.update(delta_time);

		m_viewport1.set_distance(m_distance);
		m_viewport2.set_distance(m_distance);

		if (m_viewport1.is_camera_moved()) {
			m_viewport2.set_cam_pitch(m_viewport1.get_cam_pitch());
			m_viewport2.set_cam_yaw(m_viewport1.get_cam_yaw());
		} else if (m_viewport2.is_camera_moved()) {
			m_viewport1.set_cam_pitch(m_viewport2.get_cam_pitch());
			m_viewport1.set_cam_yaw(m_viewport2.get_cam_yaw());
		}

		if (m_follow_effector) {
			m_viewport1.set_camera_target(m_effector1);
			m_viewport2.set_camera_target(m_effector2);
		} else {
			m_viewport1.set_camera_target({ 0.0f, 0.0f, 0.0f });
			m_viewport2.set_camera_target({ 0.0f, 0.0f, 0.0f });
		}

		if (!m_manual_control) {
			if (m_anim_active) {
				if (delta_time > 0.1f) {
					delta_time = 0.1f;
				}

				if (!m_anim_paused) {
					m_anim_time = m_anim_time + m_anim_speed * delta_time;
					if (m_anim_time > 1.0f) {
						if (m_loop_animation) {
							m_anim_time = 0.0f;
						} else {
							m_anim_time = 1.0f;
						}
					}
				}

				// simply interpolate the angles for animation 1
				m_config1.q1 = angle_lerp(m_start_config.q1, m_end_config.q1, m_anim_time);
				m_config1.q2 = angle_lerp(m_start_config.q2, m_end_config.q2, m_anim_time);
				m_config1.q3 = glm::mix(m_start_config.q3, m_end_config.q3, m_anim_time);    /// this is not an angle! ;)
				m_config1.q4 = angle_lerp(m_start_config.q4, m_end_config.q4, m_anim_time);
				m_config1.q5 = angle_lerp(m_start_config.q5, m_end_config.q5, m_anim_time);
				m_config1.q6 = angle_lerp(m_start_config.q6, m_end_config.q6, m_anim_time);

				// interpolate the end effector for animation 2
				m_current_target.position = glm::mix(m_puma_start.position, m_puma_end.position, m_anim_time);
				m_current_target.rotation = glm::slerp(m_puma_start.rotation, m_puma_end.rotation, m_anim_time);

				// use closest distance mode to avoid snapping
				m_solve_ik(m_config2, m_meta2, m_current_target, ik_mode_t::closest_distance);
			}
		} else {
			m_anim_active = false;
			m_config2 = m_config1;
		}
	}

	inline glm::mat4x4 rotation_mat(const glm::vec3& axis, float angle) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::rotate(id, angle, axis);
	}

	inline glm::mat4x4 translation_mat(const glm::vec3& vec) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::translate(id, vec);
	}

	inline glm::mat4x4 scale_mat(const glm::vec3& vec) {
		constexpr glm::mat4x4 id(1.0f);
		return glm::scale(id, vec);
	}

	inline float oriented_angle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& n) {
		return atan2f(glm::dot(glm::cross(a, b), n), glm::dot(a, b));
	}

	void puma_scene::m_solve_ik(puma_config_t& config, puma_solution_meta_t& meta, 
		const puma_target_t& target, ik_mode_t mode) const {

		auto p0 = glm::vec3{ 0.0f, 0.0f, 0.0f };
		auto p1 = glm::vec3{ 0.0f, 0.0f, config.l1 };
		auto p4 = target.position;

		auto xdir = glm::vec3(target.build_matrix_raw() * glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f });
		auto ydir = glm::vec3(target.build_matrix_raw() * glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f });
		auto p3 = target.position - xdir * config.l3;

		auto p_ = p4 - p3;
		auto n = glm::cross(p1 - p3, p1 - p0);

		bool is_colinear = (glm::length2(n) < 1e-7f);
		if (!is_colinear) {
			n = glm::normalize(n);
		} else {
			// solve the colinear case
			n = glm::normalize(p_);

			auto lp2 = meta.p2 - p3;
			auto q = lp2 - n * glm::dot(n, lp2); // project onto plane

			// previous point is at the centre of the circle
			// this probably should never happen
			if (glm::length2(q) < 1e-7f) [[unlikely]] { 
				q = glm::normalize(glm::cross(n, glm::vec3{1.0f, 0.0f, 0.0f}));
			}

			auto r = config.l2 * glm::normalize(q);
			auto p2 = r + p3;

			meta.p1 = p1;
			meta.p2 = p2;
			meta.p3 = p3;
			meta.p4 = p4;

			auto v01 = p1;
			auto v12 = p2 - p1;
			auto v23 = p3 - p2;

			auto world_up = glm::vec3{0.0f, 0.0f, 1.0f};
			float dotx = glm::abs(glm::dot(xdir, world_up));

			if (dotx > 0.9f) { 
				// special case 1: effector is facing directly up/down
				float l13 = glm::distance(p1, p3);
				float a = atan2f(config.l2, l13);

				if (p3.z >= p1.z) {
					config.q1 = atan2f(-p2.y, -p2.x);
					config.q2 = a + HPI;
					config.q3 = glm::distance(p1, p2);
					config.q4 = a;
				} else {
					config.q1 = atan2f(-p2.y, -p2.x);
					config.q2 = -HPI - a;
					config.q3 = glm::distance(p1, p2);
					config.q4 = PI - a;
				}
				
				auto b = oriented_angle(ydir, glm::normalize(v23), world_up);
				if (p4.z > p3.z) {
				        // xdir == world_up
					config.q5 = 0.0f;
					config.q6 = -HPI - b;
				} else {
				        // xdir == -world_up
					config.q5 = PI;
					config.q6 = b - HPI;
				}
			} else { 
				// special case 2: effector is in the plane xy
				auto n = glm::normalize(p4 - p3);
				float a = oriented_angle(glm::normalize(v01), glm::normalize(v12), n);
				float b = oriented_angle(glm::normalize(v12), glm::normalize(v23), n);
				float c = oriented_angle(glm::normalize(p2 - p3), ydir, n);
				
				config.q1 = atan2f(-p4.y, -p4.x) + HPI;
				config.q2 = HPI - a;
				config.q3 = glm::distance(p1, p2);
				config.q4 = b - HPI;
				config.q5 = HPI;
				config.q6 = c + HPI;
			}

			return;
		}

		// this is the "regular" case, so not any edge case
		const auto regular_case_p2 = [&](bool sign) -> glm::vec3 {
			assert(p_.z != 0.0f);

			auto num = config.l2 * config.l2;
			auto den = 1.0f;
			den += (n.x * n.x) / (n.y * n.y);

			float xv, yv, zv;

			auto m = (p_.x - p_.y * n.x / n.y) / p_.z;
			den += m * m;

			// + or -
			if (sign) {
				xv = glm::sqrt(num / den);
			} else {
				xv = -glm::sqrt(num / den);
			}

			zv = -m * xv;
			yv = -n.x * xv / n.y;

			return { xv, yv, zv };
		};

		const auto select_from_two = [&](const glm::vec3 & sol1, const glm::vec3 & sol2) -> glm::vec3 {
			auto d1 = glm::distance2(meta.p2, sol1);
			auto d2 = glm::distance2(meta.p2, sol2);

			switch (mode) {
				case ik_mode_t::default_solution:
					return sol1;

				case ik_mode_t::alter_solution:
					return sol2;

				case ik_mode_t::closest_distance:
					if (d1 <= d2) {
						return sol1;
					} else {
						return sol2;
					}

				default:
					return sol1;
			}
		};

		// this lambda returns the "best" p2, given previous state of config
		const auto get_best_p2 = [&]() -> glm::vec3 {
			if (p_.z != 0.0f) {
				auto sol1 = p3 + regular_case_p2(true);
				auto sol2 = p3 + regular_case_p2(false);

				return select_from_two(sol1, sol2);
			} else {
				auto m = (p_.x - p_.y * n.x / n.y);
				float xv = 0.0f;
				float yv = 0.0f;
				float zv = config.l2;

				auto sol1 = glm::vec3{ p3.x + xv, p3.y + yv, p3.z + zv };
				auto sol2 = glm::vec3{ p3.x + xv, p3.y + yv, p3.z - zv };

				return select_from_two(sol1, sol2);
			}
		};

		// v = P2 - P3
		// P2 = v + P3
		auto p2 = get_best_p2();
		auto v01 = p1;
		auto v12 = p2 - p1;
		auto v23 = p3 - p2;
		// auto v34 = p4 - p3;

		config.q1 = atan2f(p3.y, p3.x);
		config.q2 = HPI - oriented_angle(glm::normalize(v01), glm::normalize(v12), n);
		config.q3 = glm::distance(p1, p2);
		//config.q4 = atan2f(v23.z, glm::length(glm::vec2{ v23.x, v23.y })) + config.q2 + HPI;

		float a4 = oriented_angle(glm::normalize(v12), glm::normalize(v23), n);
		config.q4 = a4 - HPI;

		glm::vec3 fwd2 = rotation_mat(n, config.q4) * glm::vec4(glm::normalize(v12), 0.0f);
		config.q5 = oriented_angle(glm::normalize(fwd2), xdir, -glm::normalize(v23));

		glm::vec3 left4 = rotation_mat(glm::normalize(v23), -config.q5) * glm::vec4(n, 0.0f);
		config.q6 = oriented_angle(left4, ydir, xdir);

		meta.p1 = p1;
		meta.p2 = p2;
		meta.p3 = p3;
		meta.p4 = p4;
	}

	void puma_scene::m_draw_puma(app_context& context, const puma_config_t& config, glm::vec3& effector_pos) const {
		std::array<glm::mat4x4, 4> arm_matrix;
		std::array<glm::mat4x4, 5> joint_matrix;
		glm::mat4x4 effector_matrix;

		std::fill(arm_matrix.begin(), arm_matrix.end(), glm::mat4x4(1.0f));
		std::fill(joint_matrix.begin(), joint_matrix.end(), glm::mat4x4(1.0f));
		effector_matrix = glm::mat4x4(1.0f);

		// forward kinematics
		joint_matrix[0] = rotation_mat(AXIS_X, HPI) * rotation_mat(AXIS_Z, -config.q1);
		arm_matrix[0] = rotation_mat(AXIS_X, HPI) * rotation_mat(AXIS_Z, -config.q1);

		joint_matrix[1] = joint_matrix[0] * translation_mat({ 0.0f, 0.0f, config.l1 }) * 
			rotation_mat(AXIS_Y, -HPI + config.q2);
		arm_matrix[1] = arm_matrix[0] * translation_mat({0.0f, 0.0f, config.l1}) * 
			rotation_mat(AXIS_Y, -HPI + config.q2);

		joint_matrix[2] = joint_matrix[1] * translation_mat({0.0f, 0.0f, config.q3 }) * 
			rotation_mat(AXIS_Y, -HPI - config.q4);
		arm_matrix[2] = arm_matrix[1] * translation_mat({ 0.0f, 0.0f, config.q3 }) *
			rotation_mat(AXIS_Y, -HPI - config.q4) * rotation_mat(AXIS_Z, config.q5);

		joint_matrix[3] = joint_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) * 
			rotation_mat(AXIS_Z, config.q5) * rotation_mat(AXIS_Y, HPI);
		arm_matrix[3] = arm_matrix[2] * translation_mat({ 0.0f, 0.0f, config.l2 }) *
			rotation_mat(AXIS_Y, HPI);

		joint_matrix[4] = joint_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 });

		joint_matrix[1] = joint_matrix[1] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[2] = joint_matrix[2] * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[3] = joint_matrix[3] * rotation_mat(AXIS_Y, HPI) * translation_mat({ 0.0f, 0.0f, -0.35f });
		joint_matrix[4] = joint_matrix[4] * translation_mat({ 0.0f, 0.0f, -0.35f });

		constexpr glm::mat4x4 EFFECTOR_TO_WORLD = {
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		effector_matrix = arm_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 }) * rotation_mat(AXIS_Z, -config.q6) * EFFECTOR_TO_WORLD;
		effector_pos = arm_matrix[3] * translation_mat({ 0.0f, 0.0f, config.l3 }) * rotation_mat(AXIS_Z, -config.q6) * EFFECTOR_TO_WORLD * 
			glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

		arm_matrix[0] = arm_matrix[0] * scale_mat({ 1.0f, 1.0f, config.l1 });
		arm_matrix[1] = arm_matrix[1] * scale_mat({ 1.0f, 1.0f, config.q3 });
		arm_matrix[2] = arm_matrix[2] * scale_mat({ 1.0f, 1.0f, config.l2 });
		arm_matrix[3] = arm_matrix[3] * scale_mat({ 1.0f, 1.0f, config.l3 });

		for (const auto& mat : joint_matrix) {
			context.draw(m_joint_model, mat);
		}

		for (const auto& mat : arm_matrix) {
			context.draw(m_arm_model, mat);
		}

		m_draw_frame(context, effector_matrix);
	}

	void puma_scene::m_draw_frame(app_context& context, const glm::mat4x4& transform) const {
		context.draw(m_effector_model_y, transform * translation_mat({ 0.0f, 0.8f, 0.0f }));
		context.draw(m_effector_model_z, transform * rotation_mat(AXIS_X, HPI) * translation_mat({ 0.0f, 0.8f, 0.0f }));
		context.draw(m_effector_model_x, transform * rotation_mat(AXIS_Z, -HPI) * translation_mat({ 0.0f, 0.8f, 0.0f }));
	}

	void puma_scene::m_draw_debug(app_context& context, const glm::vec3& position) const {
		context.draw(m_point_object, CONVERT_MTX * translation_mat(position));
	}

	void puma_scene::m_draw_debug_mesh(app_context& context, std::shared_ptr<segments_array> mesh, 
		puma_solution_meta_t& meta) const {

		mesh->update_point(0, { 0.0f, 0.0f, 0.0f });
		mesh->update_point(1, meta.p1);
		mesh->update_point(2, meta.p2);
		mesh->update_point(3, meta.p3);
		mesh->update_point(4, meta.p4);
		mesh->update_point(5, meta.p1 + glm::normalize(glm::cross(meta.p1 - meta.p3, meta.p1)));
		mesh->rebuild_buffers();

		context.draw(mesh, CONVERT_MTX);
	}

	void puma_scene::m_setup_light(app_context& context) const {
		context.clear_lights();
		auto& light = context.get_light(0);

		light.color = { 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.att_const = 1.0f;

		if (m_flashlight) {
			light.position = context.get_camera().get_position();
		} else {
			light.position = { -5.0f, -5.0f, -5.0f };
		}
	}

	void puma_scene::render(app_context& context) {
		m_setup_light(m_context1);
		m_setup_light(m_context2);

		if (m_grid) {
			auto grid_model = glm::mat4x4(1.0f);
			m_context1.draw(m_grid, grid_model);
			m_context2.draw(m_grid, grid_model);
		}

		if (m_arm_mesh) {
			m_draw_puma(m_context1, m_config1, m_effector1);
			m_draw_puma(m_context2, m_config2, m_effector2);

			m_draw_frame(m_context1, m_puma_start.build_matrix());
			m_draw_frame(m_context1, m_puma_end.build_matrix());

			m_draw_frame(m_context2, m_puma_start.build_matrix());
			m_draw_frame(m_context2, m_puma_end.build_matrix());
		}

		if (m_debug_points) {
			if (!m_anim_active) {
				m_draw_debug_mesh(m_context1, m_debug_lines1, m_meta1);

				m_draw_debug(m_context1, m_meta1.p1);
				m_draw_debug(m_context1, m_meta1.p2);
				m_draw_debug(m_context1, m_meta1.p3);
				m_draw_debug(m_context1, m_meta1.p4);
			}

			m_draw_debug_mesh(m_context2, m_debug_lines2, m_meta2);

			m_draw_debug(m_context2, m_meta2.p1);
			m_draw_debug(m_context2, m_meta2.p2);
			m_draw_debug(m_context2, m_meta2.p3);
			m_draw_debug(m_context2, m_meta2.p4);
		}

		m_context2.display(false, true);
	}

	template<typename... Args>
	inline void padded_text(const char* fmt, const ImVec2& padding, Args... args) {
		auto cursor_pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor_pos.x + padding.x, cursor_pos.y + padding.y));
		ImGui::Text(fmt, args ...);
	}

	void puma_scene::gui() {
		const auto overlay_handler = [](const puma_config_t& config, const puma_solution_meta_t& meta) {
			if (ImGui::CollapsingHeader("Configuration")) {
				ImGui::SetWindowSize(ImVec2(150, 180));
				padded_text("q1 = %.4f", ImVec2(8.0f, 0.0f), config.q1);
				padded_text("q2 = %.4f", ImVec2(8.0f, 0.0f), config.q2);
				padded_text("q3 = %.4f", ImVec2(8.0f, 0.0f), config.q3);
				padded_text("q4 = %.4f", ImVec2(8.0f, 0.0f), config.q4);
				padded_text("q5 = %.4f", ImVec2(8.0f, 0.0f), config.q5);
				padded_text("q6 = %.4f", ImVec2(8.0f, 0.0f), config.q6);

				ImGui::Dummy(ImVec2(0.0f, 5.0f));
			}
		};

		m_gui_settings();

		m_viewport1.display([&]() {
			overlay_handler(m_config1, m_meta1);
		});

		m_viewport2.display([&]() {
			overlay_handler(m_config2, m_meta2);
		});
	}

	inline void angle_clamp(glm::vec3& euler) {
		if (euler.x > 180.0f) {
			euler.x = -180.0f + (euler.x - floorf(euler.x / 180.0f) * 180.0f);
		}

		if (euler.y > 180.0f) {
			euler.y = -180.0f + (euler.y - floorf(euler.y / 180.0f) * 180.0f);
		}

		if (euler.z > 180.0f) {
			euler.z = -180.0f + (euler.z - floorf(euler.z / 180.0f) * 180.0f);
		}

		if (euler.x < -180.0f) {
			euler.x = 180.0f + (euler.x + floorf(fabsf(euler.x) / 180.0f) * 180.0f);
		}

		if (euler.y < -180.0f) {
			euler.y = 180.0f + (euler.y + floorf(fabsf(euler.y) / 180.0f) * 180.0f);
		}

		if (euler.z < -180.0f) {
			euler.z = 180.0f + (euler.z + floorf(fabsf(euler.z) / 180.0f) * 180.0f);
		}
	}

	bool vector_editor_slider(const std::string& label, glm::vec3& vector) {
		const std::string label_x = "##" + label + "_x";
		const std::string label_y = "##" + label + "_y";
		const std::string label_z = "##" + label + "_z";

		bool changed = false;

		gui::prefix_label("X: ", 100.0f);
		changed = ImGui::DragFloat(label_x.c_str(), &vector[0], 0.1f, -10.0f, 10.0f) || changed;

		gui::prefix_label("Y: ", 100.0f);
		changed = ImGui::DragFloat(label_y.c_str(), &vector[1], 0.1f, -10.0f, 10.0f) || changed;

		gui::prefix_label("Z: ", 100.0f);
		changed = ImGui::DragFloat(label_z.c_str(), &vector[2], 0.1f, -10.0f, 10.0f) || changed;

		return changed;
	}

	inline bool joint_rotation_editor(const std::string_view id, glm::quat& q, glm::vec3& e, bool& quat_mode) {
		std::string id_checkbox = std::format("##_{}_checkbox", id);
		std::string id_quat_x = std::format("##_{}_qx", id);
		std::string id_quat_y = std::format("##_{}_qy", id);
		std::string id_quat_z = std::format("##_{}_qz", id);
		std::string id_quat_w = std::format("##_{}_qw", id);

		gui::prefix_label("Quaternion Input: ", 250.0f);
		ImGui::Checkbox(id_checkbox.c_str(), &quat_mode);

		bool changed = false;

		if (quat_mode) {
			gui::prefix_label("w: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_w.c_str(), &q.w, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_x.c_str(), &q.x, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_y.c_str(), &q.y, 0.01f, 0.0f, 1.0f) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_z.c_str(), &q.z, 0.01f, 0.0f, 1.0f) || changed;

			// convert quat to euler
			if (changed) {
				q = glm::normalize(q);
				e = glm::eulerAngles(q);

				e[0] = rad_to_deg(e[0]);
				e[1] = rad_to_deg(e[1]);
				e[2] = rad_to_deg(e[2]);
			}
		} else {
			gui::prefix_label("x: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_x.c_str(), &e.x, 1.0f, -90.0f, 90.0f) || changed;

			gui::prefix_label("y: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_y.c_str(), &e.y, 1.0f, -90.0f, 90.0f) || changed;

			gui::prefix_label("z: ", 250.0f);
			changed = ImGui::DragFloat(id_quat_z.c_str(), &e.z, 1.0f, -90.0f, 90.0f) || changed;

			// convert euler to quat
			if (changed) {
				angle_clamp(e);

				glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

				rotation = rotation * glm::angleAxis(deg_to_rad(e[2]), glm::vec3{ 0.0f, 0.0f, 1.0f });
				rotation = rotation * glm::angleAxis(deg_to_rad(e[1]), glm::vec3{ 0.0f, 1.0f, 0.0f });
				rotation = rotation * glm::angleAxis(deg_to_rad(e[0]), glm::vec3{ 1.0f, 0.0f, 0.0f });

				q = rotation;
			}
		}

		return changed;
	}

	void puma_scene::m_gui_settings() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(270, 450));
		ImGui::Begin("Simulation Settings", NULL);
		ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(270, 450), ImGuiCond_Once);

		auto min = ImGui::GetWindowContentRegionMin();
		auto max = ImGui::GetWindowContentRegionMax();
		auto width = max.x - min.x;

		ImGui::BeginChild("Start Config", ImVec2(width * 0.33f, 0));

		m_begin_changed = false;
		m_end_changed = false;

		if (ImGui::CollapsingHeader("Start Config", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Rotation")) {
				m_begin_changed = joint_rotation_editor("start_rot", m_puma_start.rotation, m_puma_start.euler_angles, m_puma_start.quat_mode) || m_begin_changed;
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Position")) {
				m_begin_changed = vector_editor_slider("start_pos", m_puma_start.position) || m_begin_changed;
				ImGui::TreePop();
			}

			if (ImGui::Button("Reset", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_start = puma_target_t();
				m_begin_changed = true;
			}

			ImGui::SameLine();
			if (ImGui::Button("Copy", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_end = m_puma_start;
				m_end_changed = true;
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("End Config", ImVec2(width * 0.33f, 0));

		if (ImGui::CollapsingHeader("End Config", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Rotation")) {
				m_end_changed = joint_rotation_editor("start_rot", m_puma_end.rotation, m_puma_end.euler_angles, m_puma_end.quat_mode) || m_end_changed;
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Position")) {
				m_end_changed = vector_editor_slider("start_pos", m_puma_end.position) || m_end_changed;
				ImGui::TreePop();
			}

			if (ImGui::Button("Reset", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_end = puma_target_t();
				m_end_changed = true;
			}

			ImGui::SameLine();
			if (ImGui::Button("Copy", ImVec2(glm::min(width * 0.3f, 150.0f), 0.0f))) {
				m_puma_start = m_puma_end;
				m_begin_changed = true;
			}
		}

		if (m_end_changed || m_begin_changed) {
			m_anim_time = 0.0f;
			m_anim_active = false;
		}

		if (m_begin_changed || m_end_changed) {
			puma_config_t config = m_start_config;
			puma_solution_meta_t meta = m_start_meta;

			m_solve_ik(config, meta, m_puma_start, ik_mode_t::default_solution);

			m_start_config = config;
			m_start_meta = meta;

			m_solve_ik(config, meta, m_puma_end, ik_mode_t::closest_distance);

			m_end_config = config;
			m_end_meta = meta;
		}

		if (m_begin_changed) {
			m_config1 = m_config2 = m_start_config;
			m_meta1 = m_meta2 = m_start_meta;
		}

		if (m_end_changed) {
			m_config1 = m_config2 = m_end_config;
			m_meta1 = m_meta2 = m_end_meta;
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Simulation Control", ImVec2(width * 0.33f, 0));

		if (ImGui::CollapsingHeader("Simulation Control", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label("Manual Mode: ", 100.0f);
			ImGui::Checkbox("##puma_manual", &m_manual_control);

			gui::prefix_label("Flashlight: ", 100.0f);
			ImGui::Checkbox("##puma_flashlight", &m_flashlight);

			gui::prefix_label("IK Debug: ", 100.0f);
			ImGui::Checkbox("##puma_debug", &m_debug_points);

			gui::prefix_label("Follow Effector: ", 100.0f);
			ImGui::Checkbox("##puma_cinematic", &m_follow_effector);

			if (m_manual_control) {
				gui::prefix_label("q1: ", 100.0f);
				ImGui::SliderFloat("##puma_q1", &m_config1.q1, -PI, PI);

				gui::prefix_label("q2: ", 100.0f);
				ImGui::SliderFloat("##puma_q2", &m_config1.q2, -PI, PI);

				gui::prefix_label("q3: ", 100.0f);
				ImGui::SliderFloat("##puma_q3", &m_config1.q3, 0.0f, 10.0f);

				gui::prefix_label("q4: ", 100.0f);
				ImGui::SliderFloat("##puma_q4", &m_config1.q4, -PI, PI);

				gui::prefix_label("q5: ", 100.0f);
				ImGui::SliderFloat("##puma_q5", &m_config1.q5, -PI, PI);

				gui::prefix_label("q6: ", 100.0f);
				ImGui::SliderFloat("##puma_q6", &m_config1.q6, -PI, PI);
			} else {
				gui::prefix_label("Loop Animation: ", 100.0f);
				ImGui::Checkbox("##puma_loop", &m_loop_animation);

				gui::prefix_label("Anim. Speed: ", 100.0f);
				ImGui::SliderFloat("##puma_anim_speed", &m_anim_speed, 0.1f, 5.0f);

				gui::prefix_label("Anim. Time: ", 100.0f);
				if (ImGui::SliderFloat("##puma_anim_time", &m_anim_time, 0.0f, 1.0f)) {
					if (!m_anim_active) {
						m_config1 = m_config2 = m_start_config;
						m_meta1 = m_meta2 = m_start_meta;
					}

					m_anim_active = true;
					m_anim_paused = true;
				}

				if (ImGui::Button("Play", ImVec2(width * 0.1f, 25.0f))) {
					if (!m_anim_active) {
						m_config1 = m_config2 = m_start_config;
						m_meta1 = m_meta2 = m_start_meta;
					}

					m_anim_active = true;
					m_anim_paused = false;

					if (!m_loop_animation && m_anim_time >= 1.0f) {
						m_anim_time = 0.0f;
						m_config1 = m_config2 = m_start_config;
						m_meta1 = m_meta2 = m_start_meta;
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Pause", ImVec2(width * 0.1f, 25.0f))) {
					m_anim_paused = true;
				}

				ImGui::SameLine();

				if (ImGui::Button("Reset", ImVec2(width * 0.1f, 25.0f))) {
					m_anim_paused = false;
					m_anim_time = 0.0f;
					m_config1 = m_config2 = m_start_config;
					m_meta1 = m_meta2 = m_start_meta;
				}
			}
		}

		ImGui::EndChild();

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void puma_scene::menu() {
		if (ImGui::BeginMenu("File", false)) {
			ImGui::EndMenu();
		}
	}

	void puma_scene::on_scroll(double offset_x, double offset_y) {
		bool vp1_focus = m_viewport1.is_viewport_focused() && m_viewport1.is_mouse_in_viewport();
		bool vp2_focus = m_viewport2.is_viewport_focused() && m_viewport2.is_mouse_in_viewport();

		if (vp1_focus || vp2_focus) {
			m_distance = m_distance - (static_cast<float> (offset_y) / 2.0f);
		}

		m_distance = glm::clamp(m_distance, 1.0f, 30.0f);
	}

	std::shared_ptr<triangle_mesh> puma_scene::m_make_effector_mesh() {
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> uvs;

		std::vector<GLuint> indices;

		const int res = 20;
		const float r1 = 0.25f;
		const float r2 = 0.18f;
		const float r3 = 0.18f;
		const float h = 0.5f;

		gizmo::make_gizmo_verts(positions, indices, res, r1, r2, r3, h);

		int num_vertices = positions.size() / 3;
		normals.resize(positions.size());
		uvs.resize(num_vertices * 2);

		std::fill(uvs.begin(), uvs.end(), 0.0f);

		// automatically calculate normals
		std::vector<glm::vec3> sums;
		sums.resize(num_vertices);
		std::fill(sums.begin(), sums.end(), glm::vec3{0.0f, 0.0f, 0.0f});

		int num_faces = indices.size() / 3;

		for (int face = 0; face < num_faces; ++face) {
			int base = face * 3;
			int i1 = indices[base + 0];
			int i2 = indices[base + 1];
			int i3 = indices[base + 2];

			int b1 = 3 * i1;
			int b2 = 3 * i2;
			int b3 = 3 * i3;

			auto p1 = glm::vec3{ positions[b1 + 0], positions[b1 + 1], positions[b1 + 2] };
			auto p2 = glm::vec3{ positions[b2 + 0], positions[b2 + 1], positions[b2 + 2] };
			auto p3 = glm::vec3{ positions[b3 + 0], positions[b3 + 1], positions[b3 + 2] };

			auto n1 = glm::cross(p2 - p1, p3 - p1);
			auto n2 = glm::cross(p1 - p3, p3 - p1);
			auto n3 = glm::cross(p1 - p3, p2 - p3);

			sums[i1] += n1;
			sums[i2] += n2;
			sums[i3] += n3;
		}

		for (int i = 0; i < num_vertices; ++i) {
			sums[i] = glm::normalize(sums[i]);
			normals[3 * i + 0] = sums[i].x;
			normals[3 * i + 1] = sums[i].y;
			normals[3 * i + 2] = sums[i].z;
		}

		return std::make_shared<triangle_mesh>(positions, normals, uvs, indices);
	}

	std::shared_ptr<segments_array> puma_scene::m_make_debug_mesh(std::shared_ptr<shader_program> line_shader) {
		auto mesh = std::make_shared<segments_array>(line_shader, 6);
		mesh->set_color({ 0.950f, 0.388f, 0.0380f, 1.0f });
		mesh->add_segment(0, 1);
		mesh->add_segment(1, 2);
		mesh->add_segment(2, 3);
		mesh->add_segment(3, 4);
		mesh->add_segment(1, 5);
		mesh->set_ignore_depth(true);

		return mesh;
	}
}
