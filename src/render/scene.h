#ifndef BYTE_SCENE_H
#define BYTE_SCENE_H

#include "common/common.h"
#include "common/handlemanager.h"
#include "anim.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "sprite.h"
#include "widget.h"
#include "gui.h"
#include "light.h"
#include "camera.h"

/// Visual representation of the map in game
typedef struct {
    u32     mesh;
    u32     texture;
    u32     shader;
} SceneMap;

/// Scene structure. Keep all info about rendering in the current view
typedef struct {
    u32             sprite_shader;      ///< Shader used to render sprites
    SpriteArray     *sprites;           ///< Sprites in the scene

    u32             text_shader;        ///< Shader used to render texts
    TextArray       *texts;             ///< Texts in the scene

    u32             ui_shader;          ///< Shader used to render ui elements
    WidgetArray     *widgets;           ///< Widgets in the scene. 

    Camera          *camera;            ///< Camera of the scene

    mat3            proj_matrix_2d;     ///< 2D projection matrix (GUI & text)

    SceneMap        local_map;

    Light           light1;
    Color           ambient_color;

} Scene;


/// Create and returns a new scene instance
Scene *Scene_init();

/// Destroy and free the given scene
void Scene_destroy( Scene *pScene );

/// Update the scene (camera, etc)
void Scene_update( Scene *scene, f32 frame_time );

/// Render all sprites & texts in the scene
void Scene_render( Scene *pScene );

/// Update all shaders with the scene projection matrices
void Scene_updateShadersProjMatrix( Scene *pScene );

// ##########################################################################3
//      Scene Map
    /// Returns the global world coordinates of a vector, not depending of camera
    /// zoom or pan. (often you want this to translate mousepos from screen to world)
    vec2  Scene_localToGlobal( Scene *scene, const vec2i *local );

    /// Returns the map tile at a given local screen position
    vec2i Scene_screenToIso( Scene *scene, const vec2i *local );

    void SceneMap_redTile( Scene *scene, const vec2i *tile, bool red );
    

// ##########################################################################3
//      Sprite Array
    /// Add a sprite to be rendered each frame in the scene
    /// @param pActor : the Actor that must be drawn as a sprite
    /// @return : The handle to the given sprite 
    //int  Scene_addSpriteFromActor( Scene *pScene, Actor *pActor );

    /// Modify one attribute of a Sprite (given by its handle)
    /// @param pAttrib : the attribute type we want to change (see sprite.h)
    /// @param pData   : data set as the attrib
    void Scene_modifySprite( Scene *pScene, u32 pHandle, SpriteAttrib pAttrib, void *pData );

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

// ##########################################################################3
//      Widgets Array
    /// Add a widget (and recursively all its children) to be rendered
    int Scene_addWidget( Scene *scene, const Widget *widget );

    /// Modify one attribute of a Widget (given by its handle)
    /// @param attrib : the attribute type we want to change (see widget.h)
    /// @param data   : data set as the attrib
    void Scene_modifyWidget( Scene *scene, u32 handle, WidgetAttrib attrib, void *data );

    /// Remove a widget from the scene (by its handle).
    void Scene_removeWidget( Scene *scene, u32 widget );

    /// Clears the whole widget array, to set a new scene
    void Scene_clearWidgets( Scene *scene );



#endif // BYTE_SCENE
