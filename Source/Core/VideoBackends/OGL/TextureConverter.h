// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/GL/GLUtil.h"

#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/VideoCommon.h"

namespace OGL
{
// Converts textures between formats using shaders
// TODO: support multiple texture formats
namespace TextureConverter
{
void Init();
void Shutdown();

// returns size of the encoded data (in bytes)
void EncodeToRamFromTexture(u8* dest_ptr, u32 format, u32 native_width, u32 bytes_per_row,
                            u32 num_blocks_y, u32 memory_stride, PEControl::PixelFormat srcFormat,
                            bool bIsIntensityFmt, int bScaleByHalf, const EFBRectangle& source);
}

}  // namespace OGL
