#include "camera.h"
#include "context.h"
#include "renderer.h"

Camera *Camera_new() {
    Camera *c = NULL;
    check( Context_isInitialized(), "Can't create a camera if no context has been created!\n" );

    c = byte_alloc( sizeof( Camera ) );

    c->mSpeed = 0.1f;
    c->mZoom = 1.f;
    c->mBaseZoomSpeed = c->mCurrZoomSpeed = 0.2f;
    Camera_calculateProjectionMatrix( c );

error:
    return c;
}

void Camera_destroy( Camera *pCamera ) {
    DEL_PTR( pCamera );
}

void Camera_registerListener( Camera *pCamera, ListenerFunc pFunc, ListenerType pType ) {
    if( pCamera ) {
        switch( pType ) {
            case LT_KeyListener :
                pCamera->mKeyListener = pFunc;
                EventManager_addListener( pType, pCamera->mKeyListener, pCamera );
                break;
            case LT_MouseListener :
                pCamera->mMouseListener = pFunc;
                EventManager_addListener( pType, pCamera->mMouseListener, pCamera );
                break;
        }
    }
}

void Camera_registerUpdateFunction( Camera *pCamera, CameraUpdate pFunc ) {
    if( pCamera ) {
        pCamera->mUpdateFunc = pFunc;
    }
}

void Camera_calculateProjectionMatrix( Camera *pCamera ) {
    if( pCamera && Context_isInitialized() ) {
       vec2 windowSize = Context_getSize();

       f32 xoffset = (windowSize.x - pCamera->mZoom * windowSize.x) / 2.f, 
           yoffset = (windowSize.y - pCamera->mZoom * windowSize.y) / 2.f,
           width = windowSize.x - xoffset,
           height = windowSize.y - yoffset;

       vec2 move = { .x = pCamera->mPosition.x * pCamera->mSpeed, 
                     .y = pCamera->mPosition.y * pCamera->mSpeed };

       mat3_ortho( &pCamera->mProjectionMatrix, xoffset + move.x, 
                                                width + move.x, 
                                                height + move.y, 
                                                yoffset + move.y );
    }
}

void Camera_update( Camera *pCamera ) {
    if( pCamera ) 
        pCamera->mUpdateFunc( pCamera );
}

void Camera_move( Camera *pCamera, vec2 *pVector ) {
    if( pCamera && pVector ) {
        pCamera->mPosition.x += pVector->x;
        pCamera->mPosition.y += pVector->y;

        // recalculate projection matrix and warn every shaders using it
        Camera_calculateProjectionMatrix( pCamera );
        Renderer_updateProjectionMatrix( &pCamera->mProjectionMatrix );
    }
}

void Camera_zoom( Camera *pCamera, int pZoom ) {
    if( pCamera ) {
        // Zoom speed is constant if zoom is superior to 1. Else, it decrease based on current zoom level
        if( pCamera->mZoom >= 1.f ) pCamera->mCurrZoomSpeed = pCamera->mBaseZoomSpeed;
        else
            pCamera->mCurrZoomSpeed = pCamera->mBaseZoomSpeed * (0.5f * pCamera->mZoom);

        // alterate current zoom level depending on zoom direction and speed
        pCamera->mZoom -= pZoom * pCamera->mCurrZoomSpeed;

        // no negative zoom
        if( pCamera->mZoom < 0.1f ) pCamera->mZoom = 0.1f;

        // recalculate projection matrix and warn every shaders using it
        Camera_calculateProjectionMatrix( pCamera );
        Renderer_updateProjectionMatrix( &pCamera->mProjectionMatrix );
    }
}
