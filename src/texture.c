#include "texture.h"
#include "renderer.h"

#include "GL/SOIL.h"
#ifdef USE_GLDL
#include "GL/gldl.h"
#else
#include "GL/glew.h"
#endif

Texture *Texture_new() {
    Texture *t = byte_alloc( sizeof( Texture ) );
    return t;
}

void Texture_destroy( Texture *pTexture ) {
    if( pTexture ) {
        glDeleteTextures( 1, &pTexture->mID );
        DEL_PTR( pTexture );
    }
}

bool Texture_loadFromFile( Texture *pTexture, const char *pFile, bool pMipmaps ) { 
    if( pTexture && pFile ) {
        u32 gl_id = SOIL_load_OGL_texture(  pFile, 
                                            SOIL_LOAD_AUTO,
                                            SOIL_CREATE_NEW_ID,
                                            (pMipmaps ? SOIL_FLAG_MIPMAPS : 0) );

        check( gl_id, "Error while loading texture \"%s\" : %s\n", pFile, SOIL_last_result() );

        //remove error due to OpenGL3 Core profile not used in SOIL provoking Error
        glGetError();


        pTexture->mID = gl_id;
       

        return true;
    }
error:
    return false;
}


void Texture_bind( Texture *pTexture, int pTarget ) {
    u32 tex = pTexture ? pTexture->mID : 0;

    // if pTarget < 0, no target change, else, switch the target :
    if( pTarget >=0 ) 
        glActiveTexture( GL_TEXTURE0 + pTarget );


    glBindTexture( GL_TEXTURE_2D, tex );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
}
