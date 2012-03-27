#ifndef BYTE_CAMERA_HPP
#define BYTE_CAMERA_HPP

#include "common.h"
#include "event.h"
#include "matrix.h"

/// Forward declaration
typedef struct s_Camera Camera;

/// Camera Update function (take the camera as parameter to modify it)
typedef void (*CameraUpdate)(Camera *);


/// 2D orthographic projection Camera
/// Keep track of position and zoom level
struct s_Camera {
    vec2            mPosition;          ///< Camera world position

    f32             mSpeed;             ///< Camera movement speed

    f32             mZoom,              ///< Camera absolute zoom level
                    mBaseZoomSpeed;     ///< Zoom speed for each zoom action (constant)


    ListenerFunc    mKeyListener,       ///< Camera event listener for keyboard
                    mMouseListener;     ///< Camera event listener for mouse

    mat3            mProjectionMatrix;  ///< The projection matrix used to render

    CameraUpdate    mUpdateFunc;        ///< Update function called every frame. use this to manage inputs

};

/// Create a new camera and returns it
Camera *Camera_new();

/// Destroy the given camera
void Camera_destroy( Camera *pCamera );

/// Register a given ListenerFunction to be one of the camera's one
void Camera_registerListener( Camera *pCamera, ListenerFunc pFunc, ListenerType pType );

/// Register an update function
void Camera_registerUpdateFunction( Camera *pCamera, CameraUpdate pFunc );

/// Ask the given camera to recalculate its projection matrix (if the window size has
/// changed for example).
void Camera_calculateProjectionMatrix( Camera *pCamera );

void Camera_update( Camera *pCamera );

/// Moves the camera by a given distance vector
void Camera_move( Camera *pCamera, vec2 *pVector );

/// Zoom the camera by a given offset
void Camera_zoom( Camera *pCamera, int pZoom );

#endif // BYTE_CAMERA
