#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "context.h"
#include "resource.h"
#include "game.h"

#include "GL/glew.h"

// #############################################################################
//          SCENE MAP

/// Local function. Initialize the scene map
void SceneMap_init( Scene *scene, WorldTile *tile ) {
    static const int tx_off = 2 * lmap_width * lmap_height/2 * 4;
    static const int ti_off = 2 * lmap_width * lmap_height/2 * 6;

    // buffer positions ( 4 vertices for each map tile; 9 world tiles )
    vec2 map_pos[9*4*lmap_size];
    // buffer tex coordinates ( 4 for each tile )
    vec2 map_tcs[9*4*lmap_size];
    // drawing indices ( 2 triangles(3verts) for each tile )
    u32  map_indices[9*6*lmap_size];

    // vertex position offsets
    int x_offset, y_offset;

    // Scene Tile offset (absolute offset to given tile)
    int stx_offset;
    int sty_offset;

    // arrays indices offsets ( xi and yj for pos and tcs offsets,
    int xi, yj, ii, ij;      // ii and ij for indices               )
    int xt, yt, it, jt;      // xt and yt for the tiles (x,y)


    // create map mesh data for 3x3 world tiles
    for( int y = 0; y < 3; ++y ) {
        sty_offset = (tile->location.y + y) * lmap_height/2 * tile_h;
        yt = y * 3 * tx_off;
        jt = y * 3 * ti_off;
        for( int x = 0; x < 3; ++x ) {
            stx_offset = (tile->location.x + x) * lmap_width * tile_w;
            xt = x * tx_off;
            it = x * ti_off;
            for( int j = 0; j < lmap_height/2; ++j )
                for( int i = 0; i < 2*lmap_width; ++i ) {
                    x_offset = stx_offset + i * tile_hw;
                    y_offset = sty_offset + j * tile_h + (i%2==0? 0.f : tile_hh);

                    // i * 4 : 4 vertices for each tile
                    xi = xt + i * 4;
                    // j * 2 : each row index is a col of 2 tiles
                    // LOCAL_MAP_WIDTH * 4 : 4 vertices for a complete row of tiles
                    yj = yt + j * 2 * lmap_width * 4;
                    map_pos[yj+xi+0].x = x_offset + tile_hw;
                    map_pos[yj+xi+0].y = y_offset + tile_h;
                    map_pos[yj+xi+1].x = x_offset;
                    map_pos[yj+xi+1].y = y_offset + tile_hh;
                    map_pos[yj+xi+2].x = x_offset + tile_hw;
                    map_pos[yj+xi+2].y = y_offset;
                    map_pos[yj+xi+3].x = x_offset + tile_w;
                    map_pos[yj+xi+3].y = y_offset + tile_hh;

                    map_tcs[yj+xi+0].x = 0.5f;     map_tcs[yj+xi+0].y = 1.f;
                    map_tcs[yj+xi+1].x = 0.f;     map_tcs[yj+xi+1].y = 0.5f;
                    map_tcs[yj+xi+2].x = 0.5f;     map_tcs[yj+xi+2].y = 0.f;
                    map_tcs[yj+xi+3].x = 1.f;     map_tcs[yj+xi+3].y = 0.5f;

                    ii = it + i * 6;
                    ij = jt + j * 2 * lmap_width * 6;
                    map_indices[ij+ii+0] = yj+xi+0;
                    map_indices[ij+ii+1] = yj+xi+2;
                    map_indices[ij+ii+2] = yj+xi+1;
                    map_indices[ij+ii+3] = yj+xi+0;
                    map_indices[ij+ii+4] = yj+xi+3;
                    map_indices[ij+ii+5] = yj+xi+2;
                }
        }
    }

    // create mesh
    if( 0 == scene->map.mesh )
        scene->map.mesh = Renderer_createDynamicMesh( GL_TRIANGLES );

    Renderer_setDynamicMeshData( scene->map.mesh, (f32*)map_pos, sizeof(map_pos), (f32*)map_tcs, sizeof(map_tcs), map_indices, sizeof(map_indices) );


    // get shader and texture
    scene->map.texture = ResourceManager_get( "pierre.png" );
}
/*
void SceneMap_redTile( Scene *scene, const vec2i *tile, bool red ) {
    if( tile->x >= lmap_width*2 || tile->y >= lmap_height/2 )
        return;

    Mesh *m = Renderer_getMesh( tile->map.mesh );
    u32 tcs_offset = m->vertex_count * 2;
    int i_offset = tile->x * 8,
        j_offset = tile->y * 2 * lmap_width * 8;


    // we change only on even indices, only the X coord, not Y
    m->data[tcs_offset+i_offset+j_offset+0] = red ? 0.75f : 0.25f;
    m->data[tcs_offset+i_offset+j_offset+2] = red ? 0.5f : 0.0f;
    m->data[tcs_offset+i_offset+j_offset+4] = red ? 0.75f : 0.25f;
    m->data[tcs_offset+i_offset+j_offset+6] = red ? 1.f : 0.5f;

    // rebuild map mesh
    Mesh_build( m, EUpdateVbo, false );
}
*/
inline vec2 Scene_screenToGlobal( Scene *scene, const vec2i *local ) {
    vec2 ret = { local->x, local->y};
    ret = vec2_mul( &ret, scene->camera->mZoom );
    ret = vec2_add( &ret, &scene->camera->global_position );

    return ret;
}

inline vec2i Scene_screenToIso( Scene *scene, const vec2i *local ) {
    // get global mouse position (not depending on camera zoom or pan)
    vec2 global = Scene_screenToGlobal( scene, local );

    return Map_globalToIso( &global );
}

void Scene_setLocation( Scene *scene, int x, int y ) {
    // Clamp our 3x3 window frame
    x = Clamp( x, 0, wmap_width-3 );
    y = Clamp( y, 0, wmap_height-3 );


    if( scene->map.location.x >=0 && !vec2i_eq( &scene->map.location, &(vec2i){x,y} ) ) {
        // check if deplacement on X
        int dx = (int)x - scene->map.location.x;
        if( dx ) {
            for( int i = 0; i < 3; ++i ) {
                // find removed col : - if delta is >0, rem 1 to val, if <0, add 3 to it
                //                    ==> remove left row if going right, and inversely
                //                    - then iterate on the unchanged axis
                int tx = x + (dx>0? -1:3);
                int ty = y + i;

                WorldTile *t = World_getTile( game->world, tx, ty );

                for( u32 i = 0; i < t->agents->mCount; ++i ) {
                    Agent *a = World_getGlobalAgent( game->world, HandleManager_getHandle( t->agents, i ) );
                    if( a->sprite.used_sprite >= 0 ) {
                        Scene_removeSprite( scene, a->sprite.used_sprite );
                        a->sprite.used_sprite = -1;
                    }
                }

                // find added col : -if delta is >0, add 2 to val, else, we are in added row
                //                  -iterate on the unchanged axis
                tx = x + (dx>0? 2:0);
                ty = y + i;

                t = World_getTile( game->world, tx, ty );

                for( u32 i = 0; i < t->agents->mCount; ++i ) {
                    Agent *a = World_getGlobalAgent( game->world, HandleManager_getHandle( t->agents, i ) );

                    Scene_addAgentSprite( scene, a );
                }
            }
        }

        // check if deplacement on Y
        int dy = (int)y - scene->map.location.y;
        if( dy ) {
            for( int i = 0; i < 3; ++i ) {
                // find removed row : - if delta is >0, rem 1 to val, if <0, add 3 to it
                //                    ==> remove left row if going right, and inversely
                //                    - then iterate on the unchanged axis
                int tx = x + i;
                int ty = y + (dy>0? -1:3);

                WorldTile *t = World_getTile( game->world, tx, ty );

                for( u32 i = 0; i < t->agents->mCount; ++i ) {
                    Agent *a = World_getGlobalAgent( game->world, HandleManager_getHandle( t->agents, i ) );
                    if( a->sprite.used_sprite >= 0 ) {
                        Scene_removeSprite( scene, a->sprite.used_sprite );
                        a->sprite.used_sprite = -1;
                    }
                }

                // find added row : -if delta is >0, add 2 to val, else, we are in added row
                //                  -iterate on the unchanged axis
                tx = x + i;
                ty = y + (dy>0? 2:0);

                t = World_getTile( game->world, tx, ty );

                for( u32 i = 0; i < t->agents->mCount; ++i ) {
                    Agent *a = World_getGlobalAgent( game->world, HandleManager_getHandle( t->agents, i ) );

                    Scene_addAgentSprite( scene, a );
                }
            }
        }
    } else if( scene->map.location.x == -1 ){ // initialization case
        // if initialization, just add the agents on the 3x3 tiles at (x,y)
        for( int j = 0; j < 3; ++j )
            for( int i = 0; i < 3; ++i ) {
                WorldTile *t = World_getTile( game->world, x+i, y+j );
                for( u32 i = 0; i < t->agents->mCount; ++i ) {
                    Agent *a = World_getGlobalAgent( game->world, HandleManager_getHandle( t->agents, i ) );

                    Scene_addAgentSprite( scene, a );
                }

            }
    }


    // Recreate the scene map geometry
    WorldTile *wt = World_getTile( game->world, x, y );
    SceneMap_init( scene, wt );


    // set location and global location of newly loaded frame
    scene->map.location = (vec2i){ x,y };
    scene->map.global_loc = (vec2i){ x*lmap_width*2, y*lmap_height/2 };
}


// #############################################################################
//          SCENE
// Event Listener for window resizing
/*
    void sceneWindowResizing( const Event *event, void *data ) {
        Scene *s = (Scene*)data;

        // update all texts with new window size
        for( u32 i = 0; i < s->texts->mMaxIndex; ++i ) {
            if( HandleManager_isUsed( s->texts->mUsed, i ) )
                Text_setString( s->texts->mMeshes[i], s->texts->mFonts[i], s->texts->mStrings[i] );
        }
    }
*/

bool Scene_init( Scene **sp ) {
    *sp = byte_alloc( sizeof( Scene ) );
    check_mem( *sp );

    Scene *s = *sp;

    // sprites
    // create array of entities (initial size = 50)
    s->sprites = SpriteArray_init( 50 );

    // init default sprite shader
    int ss = ResourceManager_get( "sprite_shader.json" );
    check( ss >= 0, "Sprite shader creation error!\n" );

    s->sprite_shader = ss;

    // texts
    // create array of texts (intial size = 50)
    s->texts = TextArray_init( 50 );

    // init default text shader
    int ts = ResourceManager_get( "text_shader.json" );
    check( ts >= 0, "Text shader creation error!\n" );

    s->text_shader = ts;


    // widgets
    s->widgets = WidgetArray_init(50);

    // default ui shader
    int ws = ResourceManager_get( "ui_shader.json" );
    check( ws >= 0, "UI Shader creation error!\n" );

    s->ui_shader = ws;

    // map shader
    s->map_shader = ResourceManager_get( "map_shader.json" );

    // camera
        s->camera = Camera_new();
        check_mem( s->camera );


    // 2D proj matrix (just ortho2D camera with no zoom or pan)
    vec2i winsize = Context_getSize();
    mat3_ortho( &s->proj_matrix_2d, 0.f, winsize.x, winsize.y, 0.f );

    // Lights
    s->ambient_color = (Color){ 0.05f, 0.05f, 0.05f, 1.f };

    Renderer_useShader( s->map_shader );
    Shader_sendColor( "amb_color", &s->ambient_color );
    Shader_sendFloat( "light_power", 1.f );
    Renderer_useShader( s->sprite_shader );
    Shader_sendColor( "amb_color", &s->ambient_color );
    Shader_sendFloat( "light_power", 1.f );

    // zero the light array
    memset( s->lights, 0, 8 * sizeof(Light*) );
    s->used_lights = 0;


    // Static Objects
    //s->walls = NULL;
    //s->wall_objs = StaticObjectArray_init( 100 );

    // map init
    s->map.location = (vec2i){ -1, -1 };


    return true;

error:
    Scene_destroy( s );
    return false;
}

void Scene_destroy( Scene *scene ) {
    if( scene ) {
        SpriteArray_destroy( scene->sprites );
        TextArray_destroy( scene->texts );
        WidgetArray_destroy( scene->widgets );
        Camera_destroy( scene->camera );

        //if( scene->walls )
        //    Mesh_destroy( scene->walls );
        //StaticObjectArray_destroy( scene->wall_objs );

        DEL_PTR( scene );
    }
}


inline void Scene_receiveEvent( Scene *scene, const Event *evt ) {
    switch( evt->type ) {
        case EMouseWheelMoved :
            Camera_zoom( scene->camera, evt->i );
        default:
            break;
    }
}


void Scene_updateAnimations( Scene *scene, f32 frame_time );
inline void Scene_updateAnimations( Scene *scene, f32 frame_time ) {
    for( u32 i = 0; i < scene->sprites->mMaxIndex; ++i )
        if( HandleManager_isUsed( scene->sprites->mUsed, i ) && scene->sprites->anims[i].frame_n >= 0 ) {
            if( Anim_update( &scene->sprites->anims[i], frame_time ) ) {
                scene->sprites->mAttributes[i].x[6] = scene->sprites->anims[i].frames[scene->sprites->anims[i].curr_n].x;
                scene->sprites->mAttributes[i].x[7] = scene->sprites->anims[i].frames[scene->sprites->anims[i].curr_n].y;
            }
        }
}

inline void Scene_updateShadersProjMatrix( Scene *pScene ) {
    Renderer_updateProjectionMatrix( ECamera, &pScene->camera->mProjectionMatrix );
    Renderer_updateProjectionMatrix( EGui, &pScene->proj_matrix_2d );
}

inline void Scene_update( Scene *scene, f32 frame_time, GameMode mode ) {
    Widget_update( scene, root->widget );
    switch( mode ) {
        case EGame:
            Scene_updateAnimations( scene, frame_time );
            Camera_update( scene->camera );
            break;
        case EEditor:
            Camera_update( scene->camera );
            break;
        case EMenu:
            break;
    }
}

static f32 light_powers[5] = { 0.9f, 0.95f, 1.f, 0.88f, 1.f };

void Scene_render( Scene *pScene ) {
    static f32 change_power = 0.f;
    static int power_index = 0;
    if( pScene ) {
        const f32 tmp = Game_getElapsedTime();
        glDisable( GL_CULL_FACE );

        // ##################################################
        //      RENDER MAP
        Renderer_useShader( pScene->map_shader );
        Shader_sendInt( "Depth", 9 );

        if( change_power > 0.09f )
            Shader_sendFloat( "light_power", light_powers[power_index] );

        // loop on all scene tiles
        //for( int i = 0; i < 9; ++i ) {
            Renderer_useTexture( pScene->map.texture, 0 );
            mat3 m;
            mat3_identity( &m );
            Shader_sendMat3( "ModelMatrix", &m );
            Renderer_renderMesh( pScene->map.mesh );
        //}


        // ##################################################
        //      RENDER SPRITES
        Renderer_useShader( pScene->sprite_shader );

        if( change_power > 0.09f ) {
            Shader_sendFloat( "light_power", light_powers[power_index] );
            change_power = 0.f;
            power_index = (power_index + 1) % 5;
        }

        for( u32 i = 0; i < pScene->sprites->mMaxIndex; ++i ) {
            if( HandleManager_isUsed( pScene->sprites->mUsed, i ) ) {
                // use sprites different textures (multi texturing)
                Renderer_useTexture( pScene->sprites->mTextures0[i], 0 );
                Renderer_useTexture( pScene->sprites->mTextures1[i], 1 );
                Shader_sendMat3( "AttrMatrix", &pScene->sprites->mAttributes[i] );
                Renderer_renderMesh( pScene->sprites->mMeshes[i] );
            }
        }


        // ##################################################
        //      RENDER WIDGETS
        Renderer_useShader( pScene->ui_shader );

        for( u32 i = 0; i < pScene->widgets->max_index; ++i ){
            if( HandleManager_isUsed( pScene->widgets->used, i ) ) {
                if( pScene->widgets->textures[i] >= 0 ){
                    Renderer_useTexture( pScene->widgets->textures[i], 0 );
                    Shader_sendVec2( "Position", &pScene->widgets->positions[i] );
                    Shader_sendVec2( "Scale", &pScene->widgets->scales[i] );
                    Shader_sendInt( "Depth", pScene->widgets->depths[i] );
                    Renderer_renderMesh( pScene->widgets->meshes[i] );
                }
            }
        }


        // ##################################################
        //      RENDER TEXTS
        Renderer_useShader( pScene->text_shader );

        for( u32 i = 0; i < pScene->texts->mMaxIndex; ++i ) {
            if( HandleManager_isUsed( pScene->texts->mUsed, i ) ) {
                if( pScene->texts->mVisible[i] ) {
                    Renderer_useTexture( pScene->texts->mFonts[i]->mTexture, 0 );
                    Shader_sendInt( "Depth", pScene->texts->mDepths[i] );
                    Shader_sendColor( "Color", &pScene->texts->mColors[i] );
                    Shader_sendVec2( "Position", &pScene->texts->mPositions[i] );
                    Renderer_renderMesh( pScene->texts->mMeshes[i] );
                }
            }
        }

        change_power += Game_getElapsedTime() - tmp;
    }
}

//  =======================
int  Scene_addAgentSprite( Scene *scene, Agent *agent ) {
    int handle = -1;

    if( scene && agent ) {
        handle = SpriteArray_add( scene->sprites );

        if( handle >= 0 ) {
            Sprite *as = &agent->sprite;

            as->used_sprite = handle;
            scene->sprites->mMeshes[handle] = as->mesh_id;
            scene->sprites->mTextures0[handle] = as->tex_id[0];
            scene->sprites->mTextures1[handle] = as->tex_id[1];

            // reinit attribute matrix
            memset( &scene->sprites->mAttributes[handle], 0, 9 * sizeof(f32) );

            // position components
            scene->sprites->mAttributes[handle].x[0] = as->position.x;
            scene->sprites->mAttributes[handle].x[1] = as->position.y;

            // depth component
            scene->sprites->mAttributes[handle].x[2] = as->depth;

            // mesh size components
            scene->sprites->mAttributes[handle].x[3] = as->mesh_size.x;
            scene->sprites->mAttributes[handle].x[4] = as->mesh_size.y;

            // animation frame
            if( as->animation ) {
                scene->sprites->mAttributes[handle].x[6] = as->animation->frames[as->animation->curr_n].x;
                scene->sprites->mAttributes[handle].x[7] = as->animation->frames[as->animation->curr_n].y;

                // copy and restart sprite animation
                Anim_cpy( &scene->sprites->anims[handle], as->animation );
            } else {
                scene->sprites->mAttributes[handle].x[6] = 0;
                scene->sprites->mAttributes[handle].x[7] = 0;
                scene->sprites->anims[handle].frame_n = -1;
            }
        }
    }

    return handle;
}

void Scene_modifySprite( Scene *pScene, u32 pHandle, SpriteAttrib pAttrib, void *pData ) {
    if( pScene ) {
        if( HandleManager_isUsed( pScene->sprites->mUsed, pHandle ) ) {
            switch( pAttrib ) {
                case SA_Position :
                {
                    const vec2 *pos = (vec2*)pData;
                    pScene->sprites->mAttributes[pHandle].x[0] = pos->x;
                    pScene->sprites->mAttributes[pHandle].x[1] = pos->y;
                }
                break;
                case SA_Animation :
                {
                    // copy of anim
                    Anim_cpy( &pScene->sprites->anims[pHandle], (Anim*)pData );

                    // change current frame in Attribute matrix
                    const Anim *a = &pScene->sprites->anims[pHandle];
                    pScene->sprites->mAttributes[pHandle].x[6] = a->frames[a->curr_n].x;
                    pScene->sprites->mAttributes[pHandle].x[7] = a->frames[a->curr_n].y;
                }
                break;
                case SA_Texture0 :
                    pScene->sprites->mTextures0[pHandle] = *((u32*)pData);
                    break;
                case SA_Texture1 :
                    pScene->sprites->mTextures1[pHandle] = *((int*)pData);
                    break;
                case SA_Depth :
                    pScene->sprites->mAttributes[pHandle].x[2] = *((u32*)pData);
                    break;
            }
        }
    }
}

void Scene_removeSprite( Scene *pScene, u32 pIndex ) {
    if( pScene )
        SpriteArray_remove( pScene->sprites, pIndex );
}

void Scene_clearSprites( Scene *pScene ) {
    if( pScene )
        SpriteArray_clear( pScene->sprites );
}

//  =======================

int Scene_addText( Scene *pScene, const Font *pFont, Color pColor ) {
    int handle = -1;

    if( pScene ) {
        handle = TextArray_add( pScene->texts );

        if( handle >= 0 ) {
            pScene->texts->mFonts[handle] = pFont;
            pScene->texts->mColors[handle] = pColor;
            pScene->texts->mVisible[handle] = true;
        }
    }

    return handle;
}

void Scene_modifyText( Scene *pScene, u32 pHandle, TextAttrib pAttrib, void *pData ) {
    if( pScene ) {
        // check if the given handle is a used text
        if( HandleManager_isUsed( pScene->texts->mUsed, pHandle ) ) {
            switch( pAttrib ) {
                case TA_Position :
                    {
                        vec2 new_pos = vec2_vec2i( (vec2i*)pData );
                        pScene->texts->mPositions[pHandle] = new_pos;
                    }
                    break;
                case TA_Depth :
                    pScene->texts->mDepths[pHandle] = *((int*)pData);
                    break;
                case TA_String :
                    {
                        const char *s = (const char*)pData;

                        // recreate VBO
                        Text_setString( pScene->texts->mMeshes[pHandle], pScene->texts->mFonts[pHandle], s );

                        // copy string inside textarray to keep track of current string
                        pScene->texts->mStrings[pHandle] = byte_realloc( pScene->texts->mStrings[pHandle], strlen( s ) + 1 );
                        strcpy( pScene->texts->mStrings[pHandle], s );
                    }
                    break;
                case TA_Font :
                    pScene->texts->mFonts[pHandle] = (const Font*)pData;
                    break;
                case TA_Color:
                    pScene->texts->mColors[pHandle] = *((Color*)pData);
                    break;
                case TA_Visible :
                    pScene->texts->mVisible[pHandle] = *((bool*)pData);
            }
        }
    }
}

void Scene_removeText( Scene *pScene, u32 pIndex ) {
    if( pScene )
        TextArray_remove( pScene->texts, pIndex );
}

void Scene_clearTexts( Scene *pScene ) {
    if( pScene )
        TextArray_clear( pScene->texts );
}


//  =======================

void Scene_addWidget( Scene *scene, Widget *widget ) {
    int handle = -1;

    if( scene ){
        handle = WidgetArray_add( scene->widgets );
        if( handle >= 0 ) {
            widget->sceneIndex = handle;
            widget->visible = true;

            if( widget->assets.mesh >= 0 )
                scene->widgets->meshes[handle] = widget->assets.mesh;
            if( widget->assets.texture >= 0 )
                scene->widgets->textures[handle] = widget->assets.texture;
            if( widget->assets.text >= 0 ) {
                scene->widgets->texts[handle] = widget->assets.text;
                vec2i textPos = vec2i_add( &widget->position, &widget->textOffset );
                scene->widgets->textOffsets[handle] = vec2_vec2i(&widget->textOffset);
                Scene_modifyText( scene, widget->assets.text, TA_Position, &textPos );
                Scene_modifyText( scene, widget->assets.text, TA_Visible, &(bool){true} );
                int depth = widget->depth - 1;
                Scene_modifyText( scene, widget->assets.text, TA_Depth, &depth );
            }

            scene->widgets->scales[handle] = widget->scale;
            scene->widgets->depths[handle] = widget->depth;
            scene->widgets->positions[handle] = vec2_vec2i( &widget->position );
            for( u32 i = 0; i < widget->childrenCount; ++i ) {
                Scene_addWidget( scene, widget->children[i] );
            }
        }
    }
}

void Scene_modifyWidget( Scene *scene, u32 handle, WidgetAttrib attrib, void *data ) {
    if( scene ) {
        if( HandleManager_isUsed( scene->widgets->used, handle ) ) {
            switch( attrib ) {
                case WA_Texture :
                    scene->widgets->textures[handle] = *((u32*)data);
                    break;
                case WA_Position :
                    scene->widgets->positions[handle] = vec2_vec2i( (vec2i*)data );
                    vec2i pos = vec2i_c( (int)scene->widgets->positions[handle].x, (int)scene->widgets->positions[handle].y );
                    vec2i off = vec2i_c( (int)scene->widgets->textOffsets[handle].x, (int)scene->widgets->textOffsets[handle].y );
                    vec2i textPos = vec2i_add( &pos, &off );
                    Scene_modifyText( scene, scene->widgets->texts[handle], TA_Position, &textPos );
                    break;
                case WA_Depth :
                    scene->widgets->depths[handle] = *((u32*)data);
                    int depth = scene->widgets->depths[handle] - 1;
                    Scene_modifyText( scene, scene->widgets->texts[handle], TA_Depth, &depth );
                    break;
                case WA_Scale :
                    scene->widgets->scales[handle] = *((vec2*)data);
                    break;
                default :
                    break;
            }
        }
    }
}



void Scene_removeWidget( Scene *scene, Widget* widget ) {
    if( scene ) {
        for( int i = widget->childrenCount - 1; i >= 0; --i )
            Scene_removeWidget( scene, widget->children[i] );
        WidgetArray_remove( scene->widgets, widget->sceneIndex );
        if( widget->assets.text >= 0 ) {
            Scene_modifyText( scene, widget->assets.text, TA_Visible, &(bool){false} );
        }

        widget->visible = false;
        widget->sceneIndex = -1;
    }
}

void Scene_clearWidgets( Scene *scene) {
    if( scene )
        WidgetArray_clear( scene->widgets );
}


void Scene_addLight( Scene *scene, Light *l ) {
    static str32 pname;

    if( scene && scene->used_lights < 8 ) {
        l->scene_id = scene->used_lights;
        scene->lights[scene->used_lights++] = l;

        // update shaders using lights with new light
        // map shader
        Renderer_useShader( scene->map_shader );

        snprintf( pname, 32, "light_color[%d]", l->scene_id );
        Shader_sendColor( pname, &l->diffuse );
        snprintf( pname, 32, "light_pos[%d]", l->scene_id );
        Shader_sendVec2( pname, &l->position );
        snprintf( pname, 32, "light_radius[%d]", l->scene_id );
        Shader_sendFloat( pname, l->radius );
        snprintf( pname, 32, "light_height[%d]", l->scene_id );
        Shader_sendFloat( pname, l->height );

        Shader_sendInt( "light_N", scene->used_lights );

        // sprite shader
        Renderer_useShader( scene->sprite_shader );

        snprintf( pname, 32, "light_color[%d]", l->scene_id );
        Shader_sendColor( pname, &l->diffuse );
        snprintf( pname, 32, "light_pos[%d]", l->scene_id );
        Shader_sendVec2( pname, &l->position );
        snprintf( pname, 32, "light_radius[%d]", l->scene_id );
        Shader_sendFloat( pname, l->radius );
        snprintf( pname, 32, "light_height[%d]", l->scene_id );
        Shader_sendFloat( pname, l->height );

        Shader_sendInt( "light_N", scene->used_lights );
    }
}

void Scene_removeLight( Scene *scene, Light *l ) {
    static str32 pname;

    if( scene && l->scene_id >= 0 && l->scene_id < scene->used_lights ) {
        // move all lights right to the one removed, one step to the left
        for( int i = l->scene_id+1; i < scene->used_lights; ++i ) {
            scene->lights[i-1] = scene->lights[i];
        }

        scene->lights[--scene->used_lights] = NULL;

        // update shaders using lights with new arrangement of lights
        // map
        Renderer_useShader( scene->map_shader );

        for( int i = l->scene_id; i < scene->used_lights; ++i ) {
            snprintf( pname, 32, "light_color[%d]", i );
            Shader_sendColor( pname, &scene->lights[i]->diffuse );
            snprintf( pname, 32, "light_pos[%d]", i );
            Shader_sendVec2( pname, &scene->lights[i]->position );
            snprintf( pname, 32, "light_radius[%d]", i );
            Shader_sendFloat( pname, scene->lights[i]->radius );
            snprintf( pname, 32, "light_height[%d]", i );
            Shader_sendFloat( pname, scene->lights[i]->height );
        }

        Shader_sendInt( "light_N", scene->used_lights );

        // sprites
        Renderer_useShader( scene->sprite_shader );

        for( int i = l->scene_id; i < scene->used_lights; ++i ) {
            snprintf( pname, 32, "light_color[%d]", i );
            Shader_sendColor( pname, &scene->lights[i]->diffuse );
            snprintf( pname, 32, "light_pos[%d]", i );
            Shader_sendVec2( pname, &scene->lights[i]->position );
            snprintf( pname, 32, "light_radius[%d]", i );
            Shader_sendFloat( pname, scene->lights[i]->radius );
            snprintf( pname, 32, "light_height[%d]", i );
            Shader_sendFloat( pname, scene->lights[i]->height );
        }

        Shader_sendInt( "light_N", scene->used_lights );

        l->scene_id = -1;
    }
}

inline void Scene_clearLights( Scene *scene ) {
    if( scene ) {
        memset( scene->lights, 0, 8 * sizeof(Light*) );

        // update shaders using lights
        Renderer_useShader( scene->map_shader );
        Shader_sendInt( "light_N", 0 );

        Renderer_useShader( scene->sprite_shader );
        Shader_sendInt( "light_N", 0 );
    }
}

void Scene_modifyLight( Scene *scene, Light *l, LightAttribute la ) {
    if( l->scene_id >= 0 && l->scene_id < scene->used_lights ) {
        str32 pname;

        // switch on attribute type and send the new value to shaders
        switch( la ) {
            case LA_Position :
                snprintf( pname, 32, "light_pos[%d]", l->scene_id );
                Renderer_useShader( scene->map_shader );
                Shader_sendVec2( pname, &l->position );
                Renderer_useShader( scene->sprite_shader );
                Shader_sendVec2( pname, &l->position );
                break;
            case LA_Color :
                snprintf( pname, 32, "light_color[%d]", l->scene_id );
                Renderer_useShader( scene->map_shader );
                Shader_sendColor( pname, &l->diffuse );
                Renderer_useShader( scene->sprite_shader );
                Shader_sendColor( pname, &l->diffuse );
                break;
            case LA_Radius :
                snprintf( pname, 32, "light_radius[%d]", l->scene_id );
                Renderer_useShader( scene->map_shader );
                Shader_sendFloat( pname, l->radius );
                Renderer_useShader( scene->sprite_shader );
                Shader_sendFloat( pname, l->radius );
                break;
            case LA_Height :
                snprintf( pname, 32, "light_height[%d]", l->scene_id );
                Renderer_useShader( scene->map_shader );
                Shader_sendFloat( pname, l->height );
                Renderer_useShader( scene->sprite_shader );
                Shader_sendFloat( pname, l->height );
                break;
        }
    }
}
/*
void Scene_addStaticObject( Scene *scene, StaticObject *so ) {
    if( scene ){
        int handle = StaticObjectArray_add( scene->wall_objs );

        if( handle >= 0 ) {
            so->scene_id = handle;
            scene->wall_objs->objs[handle] = so;
        }
    }
}

void Scene_removeStaticObject( Scene *scene, int index ) {
    if( scene )
        StaticObjectArray_remove( scene->wall_objs, index );
}

void Scene_buildStaticObjects( Scene *scene ) {
}
*/
