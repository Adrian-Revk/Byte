#ifndef BYTE_SCENE_HPP
#define BYTE_SCENE_HPP

#include "common/common.h"
#include "common/handlemanager.h"
#include "common/actor.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "sprite.h"

// Forward declaration
typedef struct s_Scene Scene;


/// Create and returns a new scene instance
Scene *Scene_new();

/// Destroy and free the given scene
void Scene_destroy( Scene *pScene );

/// Update the scene (camera, etc)
void Scene_update( Scene *pScene );

/// Render all sprites & texts in the scene
void Scene_render();

// ##########################################################################3
//      Map
    

// ##########################################################################3
//      Sprite Array
    /// Add a sprite to be rendered each frame in the scene
    /// @param pMesh : The mesh handle the sprite use
    /// @param pTexture : The texture handle the sprite use
    /// @param pMM : The ModelMatrix used to orient the sprite
    /// @return : The handle to the given sprite
    int  Scene_addSprite( Scene *pScene, u32 pMesh, u32 pTexture, mat3 *pMM );

    /// Add a sprite to be rendered each frame in the scene
    /// @param pActor : the Actor that must be drawn as a sprite
    /// @return : The handle to the given sprite 
    int  Scene_addSpriteFromActor( Scene *pScene, Actor *pActor );

    /// Modify one attribute of a Sprite (given by its handle)
    /// @param pAttrib : the attribute type we want to change (see sprite.h)
    /// @param pData   : data set as the attrib
    void Scene_modifySprite( Scene *pScene, u32 pHandle, SpriteAttrib pAttrib, void *pData );

    /// Transform a sprite matrix with a given one. (just matrix multiplication)
    void Scene_transformSprite( Scene *pScene, u32 pHandle, mat3 *pTransform );

    /// Remove a sprite from the scene rendered sprites (by its handle)
    void Scene_removeSprite( Scene *pScene, u32 pIndex );

    /// Clears the whole sprite array, to set a new scene
    void Scene_clearSprites( Scene *pScene );

// ##########################################################################3
//      Texts Array
    /// Add a text to be rendered each frame
    /// @return : a handle to the given registered text;
    int Scene_addText( Scene *pScene, const Font *pFont, Color pColor );

    /// Modify one attribute of a Text (given by its handle)
    /// @param pAttrib : the attribute type we want to change (see text.h)
    /// @param pData   : data set as the attrib
    void Scene_modifyText( Scene *pScene, u32 pHandle, TextAttrib pAttrib, void *pData );

    /// Remove a text from the scene (by its handle).
    void Scene_removeText( Scene *pScene, u32 pText );

    /// Clears the whole text array, to set a new scene
    void Scene_clearTexts( Scene *pScene );



#endif // BYTE_SCENE
