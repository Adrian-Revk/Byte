#include "game.h"
#include "resource.h"
#include "renderer.h"
#include "context.h"

#ifdef USE_GLDL
#include "GL/gldl.h"
#else
#include "GL/glew.h"
#endif

#include "json/cJSON.h"

// #############################################################
//      CONFIG
const str32 config_file = "config.json";

bool LoadConfigItem( cJSON *root, cJSON **item, const char *param ) {
    *item = cJSON_GetObjectItem( root, param );
    check( *item, "Error while getting parameter %s from config file.\n", param );
    return true;
error:
    return false;
}

// Config loading function
bool LoadConfig() {
    if( !game ) return false;

    cJSON *root = NULL, *item = NULL;
    char *json_file = NULL;

    bool return_val = false;

    Byte_ReadFile( &json_file, config_file );
    check( json_file, "Error when loading config file!\n" );

    root = cJSON_Parse( json_file );
    check( root, "JSON Parse error [%s] before :\n%s\n", config_file, cJSON_GetErrorPtr() );

    // get fullscreen state
    if( !LoadConfigItem( root, &item, "bFullscreen" ) ) 
        game->config.fullscreen = 0;
    else 
        game->config.fullscreen = item->type > 0;

    // get window size
    if( !LoadConfigItem( root, &item, "iWindowWidth" ) ) 
        game->config.window_size.x = 800;
    else 
        game->config.window_size.x = (f32)item->valueint;

    if( !LoadConfigItem( root, &item, "iWindowHeight" ) ) 
        game->config.window_size.y = 600;
    else
        game->config.window_size.y = (f32)item->valueint;

    // get multisamples
    if( !LoadConfigItem( root, &item, "iMultiSamples" ) ) 
        game->config.multisamples = 0;
    else 
        game->config.multisamples = item->valueint;

    return_val = true;

error:
    DEL_PTR( json_file );
    if( root ) cJSON_Delete( root );
    return return_val;
}


// #############################################################
//      GAME

/// Game mouse listener
void Game_mouseListener( const Event *event, void *data ) {
    if( event->Type == EMouseMoved ) {
        str32 text;
        snprintf( text, 32, "X : %d, Y : %d", (int)event->v.x, (int)event->v.y );
        Scene_modifyText( game->scene, game->mousepos_text, TA_String, text );

        game->mouse_tile = Scene_screenToIso( game->scene, &event->v );
        snprintf( text, 32, "TileX: %d, TileY : %d", (int)game->mouse_tile.x, (int)game->mouse_tile.y );
        Scene_modifyText( game->scene, game->mousetile_text, TA_String, text );
    }
}

/// Game window resize function
void Game_windowResize( const Event *event, void *data ) {
    Context_setSize( event->v );

    glViewport( 0, 0, (GLsizei)event->v.x, (GLsizei)event->v.y );

    if( game->scene ) {
        Camera_calculateProjectionMatrix( game->scene->camera );
        Renderer_updateProjectionMatrix( ECamera, &game->scene->camera->mProjectionMatrix );
    }
}


/// Game instance declaration
Game *game = NULL;

bool Game_init( void (*init_func)(), bool (*frame_func)(f32) ) {
    check( !game, "Tried to initialize already initialized Game Instance. Aborting.\n" );

    // Init Game Memory Manager if in Debug Mode
#   ifdef _DEBUG
    MemoryManager_init();
#   endif


    // Allocate game instance
    game = byte_alloc( sizeof( Game ) );
    check_mem( game );

    // Load config file
    LoadConfig();


    // Initialize Context
    str32 title;
    MSG( title, 32, "Byte-Project v%d.%d.%d", BYTE_MAJOR, BYTE_MINOR, BYTE_PATCH );

    check( Context_init( game->config.window_size.x,
                         game->config.window_size.y,
                         game->config.fullscreen,
                         title,
                         game->config.multisamples ), "Error while creating Context!\n" );

    // Initialize Renderer
    check( Renderer_init(), "Error while creating Renderer!\n" );

    // Initialize Event Manager
    check( EventManager_init(), "Error while creating Event Manager!\n" );

    // Initialize Freetype
    check( !FT_Init_FreeType( &game->freetype_lib ), "Could not initialize Freetype library!\n" );



    // Load all resources game
    check( ResourceManager_init(), "Error while initializing resource manager. Aborting initialization.\n" );
    check( ResourceManager_loadAllResources(), "Error while loading Game Resources. Aborting initialization.\n");


    // Init Game World 
    check( World_init(), "Error while creating Game World. Aborting initialization.\n" );



    // Initialize Game current Scene
    game->scene = Scene_init();
    check( game->scene, "Error while creating Game Scene. Aborting initialization.\n" );

    // Register Game listeners
    EventManager_addListener( LT_ResizeListener, Game_windowResize, NULL );
    EventManager_addListener( LT_MouseListener, Game_mouseListener, NULL );

    Font *f = Font_get( "DejaVuSans.ttf", 12 );
    Color col = { 0.9f, 0.9f, 0.9f, 1.f };
    int text_depth = -10;
    vec2 pos = {0,0};



    printf( "\n" );
    log_info( "Game successfully initialized!\n" );
    printf( "\n\n" );

    // call init func
    if( init_func )
        init_func();

    // Load fps text
    game->fps_text = Scene_addText( game->scene, f, col );
    Scene_modifyText( game->scene, game->fps_text, TA_Position, &pos );
    Scene_modifyText( game->scene, game->fps_text, TA_String, "FPS : 0" );
    Scene_modifyText( game->scene, game->fps_text, TA_Depth, &text_depth );

    // Load mousepos text
    game->mousepos_text = Scene_addText( game->scene, f, col );
    pos.y = 15.f;
    Scene_modifyText( game->scene, game->mousepos_text, TA_Position, &pos );
    Scene_modifyText( game->scene, game->mousepos_text, TA_String, "X  : 0, Y : 0" );
    Scene_modifyText( game->scene, game->mousepos_text, TA_Depth, &text_depth );

    // Load mousetile text
    game->mousetile_text = Scene_addText( game->scene, f, col );
    pos.y = 30.f;
    Scene_modifyText( game->scene, game->mousetile_text, TA_Position, &pos );
    Scene_modifyText( game->scene, game->mousetile_text, TA_String, "X  : 0, Y : 0" );
    Scene_modifyText( game->scene, game->mousetile_text, TA_Depth, &text_depth );


    // Update all created shaders about their projection matrices
    Scene_updateShadersProjMatrix( game->scene );


    // register main.c frame function
    game->frame_func = frame_func;

    return true;

error:
    return false;
}

void Game_destroy() {
    if( game ) {
        Scene_destroy( game->scene );

        World_destroy();
        ResourceManager_destroy();

        Renderer_destroy();
        Context_destroy();
        EventManager_destroy();

        DEL_PTR( game );
    }

#   ifdef _DEBUG
    MemoryManager_destroy();
#   endif
}

bool Game_update( f32 frame_time ) {
    static const f32 phy_dt = 0.01f;
    static f32 phy_update = 0.f, one_sec = 0.f;

    // check for game termination
    if( IsKeyUp( K_Escape ) || !Context_isWindowOpen() )
        return false;

    EventManager_update();

    // GAMEPLAY
        phy_update += frame_time;
        one_sec += frame_time;

        // callback. return false immediatly if frame_func said it
        if( game->frame_func && !game->frame_func( frame_time ) )
            return false;

        // AI GAMEPLAY LOOP (fixed at 1/phy_dt FPS)
        while( phy_update >= phy_dt ) {
            Scene_update( game->scene );
            phy_update -= phy_dt;
        }

        // EACH 1 SECOND STUFF
        if( one_sec >= 1.f ) {
            str32 fps_str;
            snprintf( fps_str, 32, "FPS : %4.0f", (1.f/frame_time) );
            Scene_modifyText( game->scene, game->fps_text, TA_String, fps_str );
            one_sec = 0.f;
        }

    // RENDERING
        Renderer_beginFrame();
        Scene_render( game->scene );
        Context_swap();

    return true;
}

bool Game_loadActorAssets( Actor *actor ) {
    check( game, "Tried to load actor assets on an ininitialized game instance\n" );

    // load scaled mesh
    str256 scaled_mesh_str;
    str16 mesh_size;
    snprintf( mesh_size, 16, "%d.%d", (int)actor->size.x, (int)actor->size.y );

    strcpy( scaled_mesh_str, actor->assets.mesh );
    strcat( scaled_mesh_str, mesh_size );

    actor->mesh_id = ResourceManager_get( scaled_mesh_str );
    // if correctly sized mesh is not yet loaded, create it
    if( -1 == actor->mesh_id ) {
        actor->mesh_id = ResourceManager_get( actor->assets.mesh );
        check( actor->mesh_id >= 0, "Error while loading actor '%s' mesh. Mesh '%s' is not a loaded resource.\n", actor->mFirstname, actor->assets.mesh );

        // resize
        int scaled_mesh = Renderer_createRescaledMesh( actor->mesh_id, &actor->size );
        check( scaled_mesh >= 0, "Error while creating scaled mesh for actor '%s'. \n", actor->mFirstname );
        
        actor->mesh_id = scaled_mesh;

        // add newly rescaled mesh to resource manager
        ResourceManager_add( scaled_mesh_str, scaled_mesh );
    }   

    // load textures
    for( int i = 0; i < actor->assets.tex_n; ++i ) {
        if( !actor->assets.texture[i][0] )
            actor->texture_ids[i] = -1;
        else {
            actor->texture_ids[i] = ResourceManager_get( actor->assets.texture[i] );
            check( actor->assets.texture[i] >= 0, "Error while loading actor '%s' texture. Texture '%s' is not a loaded resource.\n", actor->mFirstname, actor->assets.texture[i] );
        }
    }

    return true;

error:
    return false;
}

FT_Library *Game_getFreetype() {
    if( game )
        return &game->freetype_lib;
    return NULL;
}
