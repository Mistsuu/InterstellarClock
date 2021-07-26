#pragma once

#include <Windows.h>
#include <cmath>

#define PI 3.14159265359

/*
	This one calculates the variable changes for X, Y
	when the screen size changes.
*/
#define scaleX(_x_) _x_ * width
#define scaleY(_y_) _y_ * height

static double _cache_quadratic_roots[2];
inline void solveQuadratic(double a, double b, double c)
{
	double delta = b * b - 4 * a * c;
	_cache_quadratic_roots[0] = (-b + sqrt(delta)) / (2 * a);
	_cache_quadratic_roots[1] = (-b - sqrt(delta)) / (2 * a);
}

inline double sqr(double x)
{
	return x * x;
}

inline double get12HourMarkOnClock(
	D2D1_POINT_2F& dot, D2D1_POINT_2F& center,
	double a, double b
)
{
	double m = (center.y - dot.y)                    / (center.x - dot.x);
	double n = (dot.y * center.x - center.y * dot.x) / (center.x - dot.x);

	solveQuadratic(
		b * b + sqr(a * m),
		2 * (a * a * m * (n - center.y) - b * b * center.x),
		sqr(b * center.x) + sqr(a * (n - center.y)) - sqr(a * b)
	);

	if (m * _cache_quadratic_roots[0] + n > m * _cache_quadratic_roots[1] + n)
		return -acos((_cache_quadratic_roots[1] - center.x) / a);
	return -acos((_cache_quadratic_roots[0] - center.x) / a);
}