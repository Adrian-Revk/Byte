#include "color.h"
#include "shader.h"
#include "device.h"
#include "camera.h"
#include "GL/glew.h"
#include "scene.h"
#include "world.h"


int main() {
    check( Device_init(), "Error while creating Device, exiting program.\n" );


    printf( "Hello, Byte World!!\n" );

    str64 date;
    str16 time;
    GetTime( date, 64, DateFmt );
    GetTime( time, 16, TimeFmt );

    strncat( date, " - ", 3 );
    strncat( date, time, 16 );
    printf( "%s\n\n", date );




    World *world = World_new();
    check( world, "Error in world creation!\n" );

    World_loadAllResources( world );

    // ###############################3
    //      TEXTURE
    int t1 = World_getResource( world, "crate.jpg" );
    int texture = World_getResource( world, "img_test.png" );
    check( texture >= 0 && t1 >= 0, "Error in texture creation. Exiting program!\n" );
    Renderer_useTexture( t1, 0 );


    // ###############################3
    //      SHADER
    int shader = World_getResource( world, "defaultShader.json" ); 
    check( shader >= 0, "Error in shader creation. Exiting program!\n" );



    // ###############################3
    //      MESH
    vec2 data[] = {
        { .x = -5.f, .y = -5.f },
        { .x = -5.f, .y = 5.f },
        { .x = 5.f, .y = 5.f },
        { .x = 5.f, .y = -5.f }
    };

    vec2 texcoords[] = {
        { .x = 0.f, .y = 0.f },
        { .x = 0.f, .y = 1.f },
        { .x = 1.f, .y = 1.f },
        { .x = 1.f, .y = 0.f }
    };
    
    u32 indices[] = {
        0, 1, 2, 0, 2, 3
    };

    int vao = Renderer_beginVao();
    check( vao >= 0, "Could not create vao!\n" );
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );


    u32 mesh = Renderer_createMesh( indices, sizeof( indices ), data, sizeof( data ), texcoords, sizeof( texcoords ) );
    check( mesh >= 0, "Error while creating mesh!\n" );

    
    // ###############################3
    //      MATRICES
    mat3 ModelMatrix, MM;
    mat3_identity( &ModelMatrix );
    mat3_rotatef( &ModelMatrix, 45.f );
    mat3_scalef( &ModelMatrix, 2.f, 1.f );

    mat3_translatef( &ModelMatrix, 400.f, 300.f );

    mat3_identity( &MM );
    mat3_scalef( &MM, 3.f, 3.f );
    mat3_translatef( &MM, 500.f, 400.f );



    Scene *scene = Scene_new();

    Entity e = { .mMesh = mesh, .mShader = shader, .mModelMatrix = &ModelMatrix, .mTexture = t1 };

    int entity = Scene_addEntity( scene, &e );
    check( entity >= 0, "Failed to create entity!\n" );


    int cpt = 0;
    f32 accum = 0;

    while( !IsKeyUp( K_Escape ) && Context_isWindowOpen() ) {
        Device_beginFrame();
            Scene_update( scene );

            // stuff
            accum += Device_getFrameTime();
            if( accum > 1.f ) {
                ++cpt;
                accum = 0.f;

                if( cpt == 2 ) {
                    Scene_removeEntity( scene, shader, entity );
                }
                if( cpt == 3 ) {
                    e.mTexture = texture;
                    Scene_addEntity( scene, &e );
                }
                //log_info( "Camera Position : <%f, %f>\n", cam->mPosition.x, cam->mPosition.y );
                //log_info( "Cam zoom = %f\n", cam->mZoom );
            }


            Scene_render( scene );
        Device_endFrame();
    }


    Scene_destroy( scene );
    World_destroy( world );
    Device_destroy();

    return 0;

error :

    Scene_destroy( scene );
    World_destroy( world );
    Device_destroy();
    return -1;
}

