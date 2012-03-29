#ifndef BYTE_DEVICE_H
#define BYTE_DEVICE_H

#include "common/common.h"
#include "camera.h"
#include "context.h"

#include "ft2build.h"
#include FT_FREETYPE_H

/// Initialize the engine Device.
/// @return : True if everything went well, false if error;
bool Device_init();

/// Destroy the engine Device and all engine parts
void Device_destroy();

/// Call functions that need to be at the begining of every rendering frame
void Device_beginFrame();

/// Call functions that need to be at the end of every rendering frame
void Device_endFrame();

/// Returns the frame time of the last frame
f32 Device_getFrameTime();

/// Sets the device active camera
void Device_setCamera( Camera *pCamera );

/// Returns a pointer to freetype lib
FT_Library *Device_getFreetype();

#endif // BYTE_DEVICE
