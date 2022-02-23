// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/physics.h"
#include "engine/scene.h"

namespace hg {

std::vector<NodeRef> GetNodeRefsInContact(NodeRef with, const NodePairContacts &contacts) {
	const auto i = contacts.find(with);
	if (i == std::end(contacts))
		return {};

	std::vector<NodeRef> node_refs;
	node_refs.reserve(i->second.size());

	for (auto j : i->second)
		node_refs.push_back(j.first);

	return node_refs;
}

std::vector<Contact> GetNodeRefPairContacts(NodeRef first, NodeRef second, const NodePairContacts &contacts) {
	const auto i = contacts.find(first);
	if (i == std::end(contacts))
		return {};

	const auto j = i->second.find(second);
	if (j == std::end(i->second))
		return {};

	return j->second;
}

std::vector<Node> GetNodesInContact(const Scene &scene, const Node with, const NodePairContacts &contacts) {
	const auto refs = GetNodeRefsInContact(with.ref, contacts);

	std::vector<Node> nodes;
	nodes.reserve(refs.size());

	for (auto ref : refs)
		nodes.push_back(scene.GetNode(ref));

	return nodes;
}

std::vector<Contact> GetNodePairContacts(const Node first, const Node second, const NodePairContacts &contacts) {
	return GetNodeRefPairContacts(first.ref, second.ref, contacts);
}

} // namespace hg
