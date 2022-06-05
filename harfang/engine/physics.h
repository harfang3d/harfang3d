// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/node.h"

#include <array>
#include <limits>
#include <map>

namespace hg {

enum CollisionEventTrackingMode : uint8_t { CETM_EventOnly, CETM_EventAndContacts };

/// Object containing the world space position, normal and depth of a contact as reported by the collision system.
struct Contact {
	Vec3 P, N;
	float d;
};

using NodeContacts = std::map<NodeRef, std::vector<Contact>>;
using NodePairContacts = std::map<NodeRef, NodeContacts>;

std::vector<NodeRef> GetNodeRefsInContact(NodeRef with, const NodePairContacts &contacts);
std::vector<Contact> GetNodeRefPairContacts(NodeRef first, NodeRef second, const NodePairContacts &contacts);

class Scene;

std::vector<Node> GetNodesInContact(const Scene &scene, const Node with, const NodePairContacts &contacts);
std::vector<Contact> GetNodePairContacts(const Node first, const Node second, const NodePairContacts &contacts);

struct RaycastOut {
	Vec3 P{}, N{};
	Node node;
	float t{std::numeric_limits<float>::max()};
};

} // namespace hg
