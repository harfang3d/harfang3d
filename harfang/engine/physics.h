// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/node.h"

#include <array>
#include <limits>
#include <map>

namespace hg {

enum CollisionEventTrackingMode : uint8_t { CETM_EventOnly, CETM_EventAndContacts };

struct Contact {
	Vec3 P, N;
	uint32_t i;
	float d;
};

using NodeContacts = std::map<NodeRef, std::vector<Contact>>;
using NodeNodeContacts = std::map<NodeRef, NodeContacts>;

struct RaycastOut {
	Vec3 P{}, N{};
	Node node;
	float t{std::numeric_limits<float>::max()};
};

} // namespace hg
