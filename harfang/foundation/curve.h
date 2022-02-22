// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cmath>
#include <vector>

#include <foundation/math.h>

namespace hg {

enum CatmullRomParametrization { CatmullRomUniform, CatmullRomChordal, CatmullRomCentripetal };

template <typename Vec> void CubicCatmullRom(const Vec p[4], const float s[4], float t, Vec &position, Vec &derivative) {
	Vec l01 = ((s[1] - t) * p[0] + (t - s[0]) * p[1]) / (s[1] - s[0]);
	Vec l12 = ((s[2] - t) * p[1] + (t - s[1]) * p[2]) / (s[2] - s[1]);
	Vec l23 = ((s[3] - t) * p[2] + (t - s[2]) * p[3]) / (s[3] - s[2]);

	Vec dl01 = (p[1] - p[0]) / (s[1] - s[0]);
	Vec dl12 = (p[2] - p[1]) / (s[2] - s[1]);
	Vec dl23 = (p[3] - p[2]) / (s[3] - s[2]);

	Vec l012 = ((s[2] - t) * l01 + (t - s[0]) * l12) / (s[2] - s[0]);
	Vec l123 = ((s[3] - t) * l12 + (t - s[1]) * l23) / (s[3] - s[1]);

	Vec dl012 = ((s[2] - t) * dl01 + (t - s[0]) * dl12 + l12 - l01) / (s[2] - s[0]);
	Vec dl123 = ((s[3] - t) * dl12 + (t - s[1]) * dl23 + l23 - l12) / (s[3] - s[1]);

	position = ((s[2] - t) * l012 + (t - s[1]) * l123) / (s[2] - s[1]);
	derivative = ((s[2] - t) * dl012 + (t - s[1]) * dl123 + l123 - l012) / (s[2] - s[1]);
}

template <typename Vec> void CubicCatmullRom(const Vec p[4], const float s[4], float t, Vec &position, Vec &derivative, Vec &second_derivative) {
	Vec l01 = ((s[1] - t) * p[0] + (t - s[0]) * p[1]) / (s[1] - s[0]);
	Vec l12 = ((s[2] - t) * p[1] + (t - s[1]) * p[2]) / (s[2] - s[1]);
	Vec l23 = ((s[3] - t) * p[2] + (t - s[2]) * p[3]) / (s[3] - s[2]);

	Vec dl01 = (p[1] - p[0]) / (s[1] - s[0]);
	Vec dl12 = (p[2] - p[1]) / (s[2] - s[1]);
	Vec dl23 = (p[3] - p[2]) / (s[3] - s[2]);

	Vec l012 = ((s[2] - t) * l01 + (t - s[0]) * l12) / (s[2] - s[0]);
	Vec l123 = ((s[3] - t) * l12 + (t - s[1]) * l23) / (s[3] - s[1]);

	Vec dl012 = ((s[2] - t) * dl01 + (t - s[0]) * dl12 + l12 - l01) / (s[2] - s[0]);
	Vec dl123 = ((s[3] - t) * dl12 + (t - s[1]) * dl23 + l23 - l12) / (s[3] - s[1]);

	Vec ddl012 = 2.f * (dl12 - dl01) / (s[2] - s[0]);
	Vec ddl123 = 2.f * (dl23 - dl12) / (s[3] - s[1]);

	position = ((s[2] - t) * l012 + (t - s[1]) * l123) / (s[2] - s[1]);
	derivative = ((s[2] - t) * dl012 + (t - s[1]) * dl123 + l123 - l012) / (s[2] - s[1]);
	second_derivative = ((s[2] - t) * ddl012 + (t - s[1]) * ddl123 + 2.f * l123 - 2.f * l012) / (s[2] - s[1]);
}

template <typename Vec> void ComputeCubicCatmullRomParameters(const Vec p[4], CatmullRomParametrization param, float s[4]) {
	if (param == CatmullRomUniform) {
		for (int i = 0; i < 4; i++) {
			s[i] = float(i);
		}
	} else {
		s[0] = 0;
		float acc = 0;
		for (int i = 1; i < 4; i++) {
			float d = Dist(p[i - 1], p[i]);
			acc += (param == CatmullRomCentripetal) ? sqrt(d) : d;
			s[i] = acc;
		}
	}
}

template <typename Vec> void LinearInterpolate(const Vec &p0, const Vec &p1, size_t steps, std::vector<Vec> &position, std::vector<Vec> &derivative) {
	Vec p = p0;
	Vec d = p1 - p0;
	position.resize(steps);
	derivative.resize(steps);
	for (size_t i = 0; i < steps; i++) {
		float t = float(i) / float(steps - 1);
		position[i] = p0 + t * d;
		derivative[i] = d;
	}
}

template <typename Vec>
void CubicCatmullRomInterpolate(
	const std::vector<Vec> &p, CatmullRomParametrization param, size_t steps, std::vector<Vec> &position, std::vector<Vec> &derivative, bool loop = false) {
	position.clear();
	derivative.clear();
	if (p.size() < 3) {
		if (p.size() == 2) {
			LinearInterpolate(p[0], p[1], steps, position, derivative);
		} else if (p.size()) {
			position.push_back(p[0]);
			derivative.push_back(Vec());
		}
		return;
	}

	size_t total = p.size() * steps - (loop ? 0 : (steps - 1));
	position.resize(total);
	derivative.resize(total);

	size_t i, j, k;
	float s[4];
	Vec dummy[4] = {loop ? p.back() : (2.f * p[0] - p[1]), p[0], p[1], p[2]};
	ComputeCubicCatmullRomParameters(dummy, param, s);
	for (k = 0; k < steps; k++) {
		float t = s[1] + (k * (s[2] - s[1])) / (float)steps;
		CubicCatmullRom(dummy, s, t, position[k], derivative[k]);
	}

	size_t end = p.size() - 3;
	for (i = 0; i < end; i++) {
		ComputeCubicCatmullRomParameters(&p[i], param, s);
		for (j = 0; j < steps; j++, k++) {
			float t = s[1] + (j * (s[2] - s[1])) / (float)steps;
			CubicCatmullRom(&p[i], s, t, position[k], derivative[k]);
		}
	}

	if (loop) {
		dummy[0] = p[i];
		dummy[1] = p[i + 1];
		dummy[2] = p[i + 2];
		dummy[3] = p[0];
		ComputeCubicCatmullRomParameters(dummy, param, s);
		for (j = 0; j < steps; j++, k++) {
			float t = s[1] + (j * (s[2] - s[1])) / (float)steps;
			CubicCatmullRom(dummy, s, t, position[k], derivative[k]);
		}

		dummy[0] = p[i + 1];
		dummy[1] = p[i + 2];
		dummy[2] = p[0];
		dummy[3] = p[1];
		ComputeCubicCatmullRomParameters(dummy, param, s);
		for (j = 0; j < steps; j++, k++) {
			float t = s[1] + (j * (s[2] - s[1])) / (float)steps;
			CubicCatmullRom(dummy, s, t, position[k], derivative[k]);
		}
	} else {
		dummy[0] = p[i];
		dummy[1] = p[i + 1];
		dummy[2] = p[i + 2];
		dummy[3] = 2.f * p[i + 2] - p[i + 1];

		ComputeCubicCatmullRomParameters(dummy, param, s);
		for (j = 0; j <= steps; j++, k++) {
			float t = s[1] + (j * (s[2] - s[1])) / (float)steps;
			CubicCatmullRom(dummy, s, t, position[k], derivative[k]);
		}
	}
}

template <typename Vec>
void CubicCatmullRom(
	const std::vector<Vec> &p, CatmullRomParametrization param, size_t count, std::vector<Vec> &position, std::vector<Vec> &derivative, bool loop = false) {
	position.clear();
	derivative.clear();
	if (count < 2) {
		return;
	}
	if (p.size() < 3) {
		if (p.size() == 2) {
			LinearInterpolate(p[0], p[1], count, position, derivative);
		} else if (p.size()) {
			position.push_back(p[0]);
			derivative.push_back(Vec());
		}
		return;
	}

	position.resize(count);
	derivative.resize(count);

	size_t i;
	std::vector<float> strength;
	strength.resize(p.size() + 2);

	Vec first = loop ? p.back() : (2.f * p[0] - p[1]);
	Vec last = loop ? p[0] : (2.f * p[p.size() - 1] - p[p.size() - 2]);
	Vec prev;

	strength[0] = 0.f;
	if (param == CatmullRomUniform) {
		for (i = 0; i < strength.size(); i++) {
			strength[i] = float(i);
		}
	} else {
		float acc = 0.f;
		strength[0] = 0.f;
		prev = first;
		for (i = 0; i < p.size(); i++) {
			acc += sqrt(Dist(prev, p[i]));
			strength[i + 1] = acc;
			prev = p[i];
		}
		acc += sqrt(Dist(p[i - 1], last));
		strength[i + 1] = acc;
	}

	float dt = (strength[p.size() - 1] - strength[1]) / float(count - 1);

	size_t j;
	float t = strength[1];
	prev = first;
	for (i = 0, j = 0; (i < count) && (j < (p.size() - 1)); prev = p[j], j++) {
		float t_max = strength[j + 2];
		float t_0 = strength[j];

		Vec pt[4] = {prev, p[j], p[j + 1], (j < (p.size() - 2)) ? p[j + 2] : last};
		float s[4] = {0.f, strength[j + 1] - strength[j], strength[j + 2] - strength[j], strength[j + 3] - strength[j]};

		for (; (t < t_max) && (i < count); i++, t += dt) {
			CubicCatmullRom(pt, s, t - t_0, position[i], derivative[i]);
		}
	}
}

template <typename Vec>
void CubicCatmullComputeStrength(const std::vector<Vec> &p, const Vec &first, const Vec &last, CatmullRomParametrization param, std::vector<float> &strength) {
	size_t i;
	Vec prev;

	strength.resize(p.size() + 2);
	strength[0] = 0.f;
	if (param == CatmullRomUniform) {
		for (i = 0; i < strength.size(); i++) {
			strength[i] = float(i);
		}
	} else {
		float d;
		float acc = 0.f;
		strength[0] = 0.f;
		prev = first;
		for (i = 0; i < p.size(); i++) {
			d = Dist(prev, p[i]);
			acc += (param == CatmullRomCentripetal) ? sqrt(d) : d;
			;
			strength[i + 1] = acc;
			prev = p[i];
		}
		d = Dist(p[i - 1], last);
		acc += (param == CatmullRomCentripetal) ? sqrt(d) : d;
		;
		strength[i + 1] = acc;
	}
}

template <typename Vec>
void CubicCatmullRomEq(
	const std::vector<Vec> &p, CatmullRomParametrization param, float distance, std::vector<Vec> &position, std::vector<Vec> &derivative, bool loop = false) {
	position.clear();
	derivative.clear();
	if (p.size() < 3) {
		if (p.size() == 2) {
			size_t count = size_t(Dist(p[0], p[1]) / distance);
			if (count > 1) {
				LinearInterpolate(p[0], p[1], count, position, derivative);
			}
		} else if (p.size()) {
			position.push_back(p[0]);
			derivative.push_back(Vec());
		}
		return;
	}

	Vec first = loop ? p.back() : (2.f * p[0] - p[1]);
	Vec last = loop ? p[0] : (2.f * p[p.size() - 1] - p[p.size() - 2]);

	std::vector<float> strength;
	CubicCatmullComputeStrength(p, first, last, param, strength);

	float guess;
	if (param == CatmullRomUniform) {
		guess = 0.25f;
	} else {
		guess = ((param == CatmullRomCentripetal) ? sqrt(distance) : distance) / 2.f;
	}

	size_t count = p.size() - 1;
	float t = strength[1];
	position.push_back(p[0]);

	for (size_t i = 1; i <= count;) {
		int k;
		const Vec &start = position.back();
		Vec current = start;
		t += (i < count) ? guess : (strength[i + 1] - t) / 2.f;

		size_t j;
		for (k = 0; k < 64; k++) {
			for (j = i; j <= count; j++) {
				if ((t > strength[j]) && (t <= strength[j + 1])) {
					break;
				}
			}

			if (j > count) {
				break;
			}

			float t0 = strength[j - 1];
			const Vec &begin = (j > 1) ? p[j - 2] : first;
			const Vec &end = (j < count) ? p[j + 1] : last;

			Vec pt[4] = {begin, p[j - 1], p[j], end};
			float s[4] = {0.f, strength[j] - t0, strength[j + 1] - t0, strength[j + 2] - t0};

			// Hailley
			Vec dp;
			Vec d2p;

			CubicCatmullRom(pt, s, t - t0, current, dp, d2p);
			Vec delta = current - start;

			float f = Dot(delta, delta) - (distance * distance);
			float df = 2.f * Dot(delta, dp);
			float d2f = 2.f * (Dot(delta, d2p) + Dot(dp, dp));
			float d = 2.f * f * df / (2.f * df * df - f * d2f);

			if (Abs(d) < 0.0001) {
				position.push_back(current);
				derivative.push_back(dp);
				break;
			}
			if ((t - d) > strength[j]) {
				t -= d;
			} else {
				t += guess;
			}
		}
		i = j;
	}
}

} // namespace hg