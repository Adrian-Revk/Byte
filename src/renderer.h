#ifndef BYTE_RENDERER_HPP
#define BYTE_RENDERER_HPP

#include "common.h"

typedef struct s_Renderer Renderer;

/// Initialize a renderer and returns it
bool Renderer_init();

/// Destroy a given renderer
void Renderer_destroy();

/// Begin a new VAO in the given Renderer
/// @return : the indice in the Renderer VAO array of the created VAO
int  Renderer_beginVao();

/// End the current VAO in the current Renderer
void Renderer_endVao();

/// Bind a Vao given its index
void Renderer_bindVao( u32 pIndex );

#endif // BYTE_RENDERER

