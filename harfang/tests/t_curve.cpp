// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "gtest/gtest.h"

#include <foundation/vector2.h>
#include <foundation/vector3.h>
#include <foundation/curve.h>

#include <cstdlib>
#include <vector>

using namespace hg;

TEST(Curve, CatmullRomCentripetalVec2)
{
	std::vector<Vec2> position;
	std::vector<Vec2> derivative;
	static const float s = sqrt(2.f);
	std::vector<Vec2> keypoints = {
		{ 0.f, s  },
		{ 1.f, 1.f},
		{   s, 0.f},
		{ 1.f,-1.f},
		{ 0.f,-s  },
		{-1.f,-1.f},
		{  -s, 0.f},
		{-1.f, 1.f},
	};
	CubicCatmullRom(keypoints, CatmullRomCentripetal, 32, position, derivative, true);
	EXPECT_EQ(position.size(), 32);
	EXPECT_EQ(derivative.size(), 32);
}

TEST(Curve, CatmullRomEqCentripetalVec2)
{
	std::vector<Vec2> position;
	std::vector<Vec2> derivative;
	static const float s = sqrt(2.f);
	static const float distance = 0.25f;
	std::vector<Vec2> keypoints = {
		{ 0.f, s  },
		{ 1.f, 1.f},
		{   s, 0.f},
		{ 1.f,-1.f},
		{ 0.f,-s  },
		{-1.f,-1.f},
		{  -s, 0.f},
		{-1.f, 1.f},
	};
	CubicCatmullRomEq(keypoints, CatmullRomCentripetal, distance, position, derivative, true);
	for (size_t i = 0; i < position.size() - 1; i++) {
		float d = Dist(position[i], position[i + 1]);
		EXPECT_NEAR(distance, d, 0.0001f);
	}
}
