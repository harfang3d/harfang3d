// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/audio_stream_interface.h"

namespace hg {

IAudioStreamer MakeOGGFileStreamer();
IAudioStreamer MakeOGGAssetStreamer();

} // namespace hg
