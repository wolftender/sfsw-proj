#pragma once
#include <memory>
#include <limits>
#include <optional>

#include <glm/glm.hpp>

namespace mini {
	constexpr float NOT_DIFFERENTIABLE = std::numeric_limits<float>::quiet_NaN();

	class f_base {
		private:
			bool m_differentiable;

		public:
			bool is_differentiable() const {
				return m_differentiable;
			}

			f_base(bool differentiable) : m_differentiable(differentiable) { }
			virtual ~f_base() { }

			virtual float derivative(float t) = 0;
			virtual float value(float t) = 0;
	};

	using f_func = std::unique_ptr<f_base>;

	class f_sum : public f_base {
		private:
			f_func m_f1, m_f2;

		public:
			f_sum(f_func && f1, f_func && f2) : 
				f_base(f1->is_differentiable() && f2->is_differentiable()),
				m_f1(std::move(f1)), m_f2(std::move(f2)) { }

			virtual float derivative(float t) override {
				return m_f1->derivative(t) + m_f2->derivative(t);
			}

			virtual float value(float t) override {
				return m_f1->value(t) + m_f2->value(t);
			}
	};

	class f_sub : public f_base {
		private:
			f_func m_f1, m_f2;

		public:
			f_sub(f_func&& f1, f_func&& f2) : 
				f_base(f1->is_differentiable() && f2->is_differentiable()),
				m_f1(std::move(f1)), m_f2(std::move(f2)) { }

			virtual float derivative(float t) override {
				return m_f1->derivative(t) - m_f2->derivative(t);
			}

			virtual float value(float t) override {
				return m_f1->value(t) - m_f2->value(t);
			}
	};

	class f_mul : public f_base {
		private:
			f_func m_f1, m_f2;

		public:
			f_mul(f_func&& f1, f_func&& f2) : 
				f_base(f1->is_differentiable() && f2->is_differentiable()),
				m_f1(std::move(f1)), m_f2(std::move(f2)) {}

			virtual float derivative(float t) override {
				return (m_f1->derivative(t) * m_f2->value(t)) + (m_f2->derivative(t) * m_f1->value(t));
			}

			virtual float value(float t) override {
				return m_f1->value(t) * m_f2->value(t);
			}
	};

	class f_frac : public f_base {
		private:
			f_func m_f1, m_f2;

		public:
			f_frac(f_func&& f1, f_func&& f2) : 
				f_base(f1->is_differentiable() && f2->is_differentiable()),
				m_f1(std::move(f1)), m_f2(std::move(f2)) {}

			virtual float derivative(float t) override {
				return ((m_f1->derivative(t) * m_f2->value(t)) - (m_f2->derivative(t) * m_f1->value(t))) / (m_f2->value(t) * m_f2->value(t));
			}

			virtual float value(float t) override {
				return m_f1->value(t) / m_f2->value(t);
			}
	};

	class f_comp : public f_base {
		private:
			f_func m_f1, m_f2;

		public:
			f_comp(f_func&& f1, f_func&& f2) : 
				f_base(f1->is_differentiable() && f2->is_differentiable()),
				m_f1(std::move(f1)), m_f2(std::move(f2)) {}

			virtual float derivative(float t) override {
				return m_f1->derivative(m_f2->value(t)) * m_f2->derivative(t);
			}

			virtual float value(float t) override {
				return m_f1->value(m_f2->value(t));
			}
	};

	class f_const : public f_base {
		private:
			float m_value;

		public:
			f_const(float value) : f_base(true), m_value(value) {}

			virtual float derivative(float t) override {
				return 0.0f;
			}

			virtual float value(float t) override {
				return m_value;
			}
	};

	class f_lin : public f_base {
		private:
			float m_slope;

		public:
			f_lin(float slope) : f_base(true), m_slope(slope) { }

			virtual float derivative(float t) override {
				return m_slope;
			}

			virtual float value(float t) override {
				return m_slope * t;
			}
	};

	class f_pow : public f_base {
		private:
			float m_pow;

		public:
			f_pow(float pow) : f_base(true), m_pow(pow) {}

			virtual float derivative(float t) override {
				return glm::pow(t, m_pow - 1.0f);
			}

			virtual float value(float t) override {
				return glm::pow(t, m_pow);
			}
	};

	class f_exp : public f_base {
		public:
			f_exp() : f_base(true) {}

			virtual float derivative(float t) override {
				return glm::exp(t);
			}

			virtual float value(float t) override {
				return glm::exp(t);
			}
	};

	class f_sin : public f_base {
		public:
			f_sin() : f_base(true) {}

			virtual float derivative(float t) override {
				return glm::cos(t);
			}

			virtual float value(float t) override {
				return glm::sin(t);
			}
	};

	class f_cos : public f_base {
		public:
			f_cos() : f_base(true) {}

			virtual float derivative(float t) override {
				return -glm::sin(t);
			}

			virtual float value(float t) override {
				return glm::cos(t);
			}
	};

	class f_sign : public f_base {
		public:
			f_sign() : f_base(false) {}

			virtual float derivative(float t) override {
				return NOT_DIFFERENTIABLE;
			}

			virtual float value(float t) override {
				return glm::sign(t);
			}
	};

	inline std::optional<float> diff(const f_func& f, const float t) {
		auto df = f->derivative(t);
		if (std::isnan(df)) {
			return {};
		}

		return df;
	}

	inline f_func mk_sum(f_func && f1, f_func && f2) {
		return std::make_unique<f_sum>(std::move(f1), std::move(f2));
	}

	inline f_func mk_mul(f_func&& f1, f_func&& f2) {
		return std::make_unique<f_mul>(std::move(f1), std::move(f2));
	}

	inline f_func mk_sub(f_func&& f1, f_func&& f2) {
		return std::make_unique<f_sub>(std::move(f1), std::move(f2));
	}

	inline f_func mk_frac(f_func&& f1, f_func&& f2) {
		return std::make_unique<f_frac>(std::move(f1), std::move(f2));
	}
 
	inline f_func mk_comp(f_func&& f1, f_func&& f2) {
		return std::make_unique<f_comp>(std::move(f1), std::move(f2));
	}

	inline f_func mk_const(float c) {
		return std::make_unique<f_const>(c);
	}

	inline f_func mk_lin(float slope) {
		return std::make_unique<f_lin>(slope);
	}
	
	inline f_func mk_pow(float p) {
		return std::make_unique<f_pow>(p);
	}

	inline f_func mk_exp() {
		return std::make_unique<f_exp>();
	}

	inline f_func mk_sin() {
		return std::make_unique<f_sin>();
	}

	inline f_func mk_cos() {
		return std::make_unique<f_cos>();
	}

	inline f_func mk_sgn() {
		return std::make_unique<f_sign>();
	}
}