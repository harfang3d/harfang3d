// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/debugger.h"
#include "engine/scene.h"

#include "foundation/clock.h"
#include "foundation/format.h"

#include "imgui/imgui.h"

namespace hg {

static std::string FormatGenRef(gen_ref ref) { return format("%1.%2").arg(ref.idx).arg(ref.gen); }

static std::string FormatAnimRef(const Scene &scene, AnimRef ref) {
	return scene.IsValidAnim(ref) ? format("AnimRef %1.%2").arg(ref.gen).arg(ref.idx) : format("AnimRef %1.%2: InvalidRef").arg(ref.idx).arg(ref.gen);
}

static std::string FormatNodeRef(const Scene &scene, NodeRef ref) {
	return format("NodeRef %1.%2: %3").arg(ref.idx).arg(ref.gen).arg(scene.IsValidNodeRef(ref) ? scene.GetNodeName(ref).c_str() : "InvalidRef");
}

static std::string FormatSceneAnimRef(const Scene &scene, SceneAnimRef ref) {
	return format("SceneAnimRef %1.%2: %3").arg(ref.idx).arg(ref.gen).arg(scene.IsValidSceneAnim(ref) ? scene.GetSceneAnim(ref)->name.c_str() : "InvalidRef");
}

template <typename T> void DebugAnimTrack(AnimTrackT<T> &track) {
	if (ImGui::TreeNode("Track (target: %s)", track.target.c_str())) {
		ImGui::Text("%d key(s)", track.keys.size());
		ImGui::TreePop();
	}
}

template <typename T> void DebugAnimTrack(AnimTrackHermiteT<T> &track) {
	if (ImGui::TreeNode("Track Hermite (target: %s)", track.target.c_str())) {
		ImGui::Text("%d key(s)", track.keys.size());
		ImGui::TreePop();
	}
}

template <typename Track> void DebugAnimTracks(const char *type, std::vector<Track> &tracks) {
	if (!tracks.empty())
		if (ImGui::TreeNode(type)) {
			for (auto &track : tracks)
				DebugAnimTrack(track);
			ImGui::TreePop();
		}
}

static void DebugAnim(Scene &scene, AnimRef ref) {
	if (ImGui::TreeNode(FormatAnimRef(scene, ref).c_str())) {
		if (auto anim = scene.GetAnim(ref)) {
			DebugAnimTracks("Bool", anim->bool_tracks);
			DebugAnimTracks("Int", anim->int_tracks);
			DebugAnimTracks("Float", anim->float_tracks);
			DebugAnimTracks("Vec2", anim->vec2_tracks);
			DebugAnimTracks("Vec3", anim->vec3_tracks);
			DebugAnimTracks("Vec4", anim->vec4_tracks);
			DebugAnimTracks("Color", anim->color_tracks);
		}
		ImGui::TreePop();
	}
}

static void DebugSceneAnim(Scene &scene, SceneAnimRef ref) {
	if (ImGui::TreeNode(FormatSceneAnimRef(scene, ref).c_str())) {
		if (auto anim = scene.GetSceneAnim(ref)) {
			if (ImGui::TreeNode("Scene animation")) {
				DebugAnim(scene, anim->scene_anim);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Node animations")) {
				for (auto node_anim : anim->node_anims)
					if (ImGui::TreeNode(FormatNodeRef(scene, node_anim.node).c_str())) {
						ImGui::PushID(node_anim.node.idx);
						DebugAnim(scene, node_anim.anim);
						ImGui::PopID();
						ImGui::TreePop();
					}

				ImGui::TreePop();
			}
		} else {
			ImGui::Text("Invalid ref");
		}
		ImGui::TreePop();
	}
}

static void DebugNode(Scene &scene, NodeRef node_ref, const NodesChildren &nodes_children) {
	ImGui::PushID(node_ref.idx);

	if (ImGui::TreeNode(FormatNodeRef(scene, node_ref).c_str())) {
		auto node = scene.GetNode(node_ref);

		if (ImGui::CollapsingHeader("Flags")) {
			bool is_disabled = node.GetFlags() & NF_Disabled;
			ImGui::Checkbox("NF_Disabled", &is_disabled);

			ImGui::Separator();

			bool is_instantiated = node.GetFlags() & NF_Instantiated;
			ImGui::Checkbox("NF_Instantiated", &is_instantiated);
			ImGui::SameLine();
			bool is_instance_disabled = node.GetFlags() & NF_InstanceDisabled;
			ImGui::Checkbox("NF_InstanceDisabled", &is_instance_disabled);
		}

		const auto children = nodes_children.GetChildren(node.ref);
		if (!children.empty())
			if (ImGui::CollapsingHeader("Children"))
				for (auto child : children)
					DebugNode(scene, child, nodes_children);

		if (node.HasTransform())
			if (ImGui::CollapsingHeader("Transform")) {
				const auto trs = node.GetTransform();
				auto pos = trs.GetPos(), rot = trs.GetRot(), scl = trs.GetScale();
				ImGui::InputFloat3("Pos", &pos.x);
				ImGui::InputFloat3("Rot", &rot.x);
				ImGui::InputFloat3("Scl", &scl.x);
			}

		if (node.HasInstance())
			if (ImGui::CollapsingHeader("Instance"))
				if (ImGui::TreeNode(node.GetInstance().GetPath().c_str())) {
					auto view = node.GetInstanceSceneView();

					if (ImGui::TreeNode("Nodes")) {
						for (auto ref : view.nodes)
							DebugNode(scene, ref, nodes_children);
						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Animations")) {
						for (auto ref : view.anims)
							DebugAnim(scene, ref);
						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Scene animations")) {
						for (auto ref : view.scene_anims)
							DebugSceneAnim(scene, ref);
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

		ImGui::TreePop();
	}
	ImGui::PopID();
}

static bool g_pause_clock = false;

void DebugSceneExplorer(Scene &scene, const char *name, bool *open) {
	if (ImGui::Begin(name, open)) {
		if (ImGui::Button("Pause/Resume Clock")) {
			g_pause_clock = !g_pause_clock;
			g_pause_clock ? set_clock_scale(0.f) : set_clock_scale(1.f);
		}

		ImGui::Separator();
		const auto nodes_children = scene.BuildNodesChildren();

		for (auto &node : scene.GetNodes())
			if (!node.GetTransform().GetParentNode().IsValid())
				DebugNode(scene, node.ref, nodes_children);
	}
	ImGui::End();
}

} // namespace hg
