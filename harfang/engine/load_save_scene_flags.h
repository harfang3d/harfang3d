// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>

namespace hg {

// features
static const uint32_t LSSF_Nodes = 0x1;
static const uint32_t LSSF_Scene = 0x2;
static const uint32_t LSSF_Anims = 0x4;
static const uint32_t LSSF_KeyValues = 0x8;
static const uint32_t LSSF_Physics = 0x10;
static const uint32_t LSSF_Scripts = 0x20;
static const uint32_t LSSF_All = 0xffff;

static const uint32_t LSSF_FeaturesMask = 0xffff;
static const uint32_t LSSF_AllNodeFeatures = LSSF_Nodes | LSSF_Anims | LSSF_Physics | LSSF_Scripts;

// options
static const uint32_t LSSF_QueueTextureLoads = 0x10000;
static const uint32_t LSSF_FreezeMatrixToTransformOnSave = 0x20000; // freeze current matrix to the transform TRS (useful to save exact physics state)
static const uint32_t LSSF_DoNotLoadResources = 0x40000; // skip loading any resources
static const uint32_t LSSF_QueueModelLoads = 0x80000;
static const uint32_t LSSF_DoNotChangeCurrentCameraIfValid = 0xa0000; // default behavior when loading a scene is to set the current camera if specified, when
																	  // this flag is raised this will only be done if the current camera is invalid
static const uint32_t LSSF_Silent = 0xb0000; // do not log errors

static const uint32_t LSSF_QueueResourceLoads = LSSF_QueueTextureLoads | LSSF_QueueModelLoads;

static const uint32_t LSSF_OptionsMask = 0xffff0000;

} // namespace hg
