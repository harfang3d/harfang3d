/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file  fbx_optimizegraph.cpp
 *  @brief Adapted from Assimp OptimizeGraph.cpp, Implementation of the fbx scenegraph step
 */

#include "fbx_optimizegraph.h"
#include "PostProcessing/ProcessHelper.h"
#include "PostProcessing/ConvertToLHProcess.h"
#include <assimp/Exceptional.h>
#include <assimp/SceneCombiner.h>
#include <stdio.h>
#include <functional>

using namespace Assimp;

#define AI_RESERVED_NODE_NAME "$Reserved_And_Evil"

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
FbxOptimizeGraphProcess::FbxOptimizeGraphProcess() :
		mScene(),
		nodes_in(),
		nodes_out(),
		count_merged() {
	// empty
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
FbxOptimizeGraphProcess::~FbxOptimizeGraphProcess() {
	// empty
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool FbxOptimizeGraphProcess::IsActive(unsigned int pFlags) const {
	return true;
}

// ------------------------------------------------------------------------------------------------
// Setup properties for the post-processing step
void FbxOptimizeGraphProcess::SetupProperties(const Importer *pImp) {
}

// ------------------------------------------------------------------------------------------------
// Collect new children
void FbxOptimizeGraphProcess::CollectNewChildren(aiNode *nd, std::list<aiNode *> &nodes) {
	nodes_in += nd->mNumChildren;

	// Process children
	std::list<aiNode *> child_nodes;
	for (unsigned int i = 0; i < nd->mNumChildren; ++i) {
		CollectNewChildren(nd->mChildren[i], child_nodes);
		nd->mChildren[i] = nullptr;
	}

	// collapse $AssimpFbx$ nodes
	if (strstr(nd->mName.C_Str(), "$AssimpFbx$") != nullptr) {
		for (std::list<aiNode *>::iterator it = child_nodes.begin(); it != child_nodes.end();) {

			(*it)->mTransformation = nd->mTransformation * (*it)->mTransformation;
			nodes.push_back(*it);

			it = child_nodes.erase(it);
		}
	} else {
		nodes.push_back(nd);
	}
	// reassign children if something changed
	if (child_nodes.empty() || child_nodes.size() > nd->mNumChildren) {

		delete[] nd->mChildren;

		if (!child_nodes.empty()) {
			nd->mChildren = new aiNode *[child_nodes.size()];
		} else
			nd->mChildren = nullptr;
	}

	nd->mNumChildren = static_cast<unsigned int>(child_nodes.size());

	if (nd->mChildren) {
		aiNode **tmp = nd->mChildren;
		for (std::list<aiNode *>::iterator it = child_nodes.begin(); it != child_nodes.end(); ++it) {
			aiNode *node = *tmp++ = *it;
			node->mParent = nd;
		}
	}

	nodes_out += static_cast<unsigned int>(child_nodes.size());
}

// ------------------------------------------------------------------------------------------------
// Execute the post-processing step on the given scene
void FbxOptimizeGraphProcess::Execute(aiScene *pScene) {
	ASSIMP_LOG_DEBUG("FbxOptimizeGraphProcess begin");
	nodes_in = nodes_out = count_merged = 0;
	mScene = pScene;

	// Insert a dummy master node and make it read-only
	aiNode *dummy_root = new aiNode(AI_RESERVED_NODE_NAME);

	const aiString prev = pScene->mRootNode->mName;
	pScene->mRootNode->mParent = dummy_root;

	dummy_root->mChildren = new aiNode *[dummy_root->mNumChildren = 1];
	dummy_root->mChildren[0] = pScene->mRootNode;

	// Do our recursive processing of scenegraph nodes. For each node collect
	// a fully new list of children and allow their children to place themselves
	// on the same hierarchy layer as their parents.
	std::list<aiNode *> nodes;
	CollectNewChildren(dummy_root, nodes);

	ai_assert(nodes.size() == 1);

	if (dummy_root->mNumChildren == 0) {
		pScene->mRootNode = nullptr;
		throw DeadlyImportError("After optimizing the scene graph, no data remains");
	}

	if (dummy_root->mNumChildren > 1) {
		pScene->mRootNode = dummy_root;

		// Keep the dummy node but assign the name of the old root node to it
		pScene->mRootNode->mName = prev;
	} else {

		// Remove the dummy root node again.
		pScene->mRootNode = dummy_root->mChildren[0];

		dummy_root->mChildren[0] = nullptr;
		delete dummy_root;
	}

	pScene->mRootNode->mParent = nullptr;
	if (!DefaultLogger::isNullLogger()) {
		if (nodes_in != nodes_out) {
			ASSIMP_LOG_INFO("FbxOptimizeGraphProcess finished; Input nodes: ", nodes_in, ", Output nodes: ", nodes_out);
		} else {
			ASSIMP_LOG_DEBUG("FbxOptimizeGraphProcess finished");
		}
	}
}

