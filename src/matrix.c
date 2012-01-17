#include "matrix.h"


// inline function declarations
//extern inline Matrix Matrix_new( f32, f32, f32, f32, f32, f32, f32, f32, f32 );
extern inline void mat3_print( const mat3 *mat );
extern inline mat3 mat3_new( f32 x00, f32 x01, f32 x02, f32 x10, f32 x11, f32 x12, f32 x20, f32 x21, f32 x22 );

void mat3_identity( mat3 *A ) {
    if( A ) {
        memset( A->x, 0, 9 * sizeof( f32 ) );
        A->x[0] = A->x[4] = A->x[8] = 1.f;
    }
}

void mat3_mul( mat3 *A, const mat3 *B ) {
    if( A && B ) {
        mat3 m;
        m.x[0] = A->x[0] * B->x[0] + A->x[1] * B->x[3] + A->x[2] * B->x[6];
        m.x[1] = A->x[0] * B->x[1] + A->x[1] * B->x[4] + A->x[2] * B->x[7];

        m.x[3] = A->x[3] * B->x[0] + A->x[4] * B->x[3] + A->x[5] * B->x[6];
        m.x[4] = A->x[3] * B->x[1] + A->x[4] * B->x[4] + A->x[5] * B->x[7];


        memcpy( A->x, m.x, 9 * sizeof( f32 ) );
    }
}


void mat3_translationMatrixf( mat3 *mat, f32 x, f32 y ) {
    if( mat ) {
        f32 val[9] = {  1, 0, 0,
                        0, 1, 0,
                        x, y, 1 };

        memcpy( mat->x, val, 9 * sizeof( f32 ) );
    }
}

void mat3_translationMatrixfv( mat3 *mat, const vec2 *v ) {
    if( mat && v ) {
        f32 val[9] = {  1,    0,    0,
                        0,    1,    0,
                        v->x, v->y, 1 };

        memcpy( mat->x, val, 9 * sizeof( f32 ) );
    }
}

void mat3_translatef( mat3 *mat, f32 x, f32 y ) {
    mat3 trans;
    mat3_translationMatrixf( &trans, x, y );
    mat3_mul( mat, &trans );
}

void mat3_translatefv( mat3 *mat, const vec2 *v ) {
    mat3 trans;
    mat3_translationMatrixfv( &trans, v );
    mat3_mul( mat, &trans );
}

void mat3_scaleMatrixf( mat3 *mat, f32 x, f32 y ) {
    if( mat ) {
        f32 val[9] = {  x,    0,    0,
                        0,    y,    0,
                        0,    0,    1 };

        memcpy( mat->x, val, 9 * sizeof( f32 ) );
    }
}

void mat3_scaleMatrixfv( mat3 *mat, const vec2 *v ) {
    if( mat && v ) {
        f32 val[9] = {  v->x, 0,    0,
                        0,    v->y, 0,
                        0,    0,    1 };

        memcpy( mat->x, val, 9 * sizeof( f32 ) );
    }
}

void mat3_scalef( mat3 *mat, f32 x, f32 y ) {
    mat3 scaleMat;
    mat3_scaleMatrixf( &scaleMat, x, y );
    mat3_mul( mat, &scaleMat );
}

void mat3_scalefv( mat3 *mat, const vec2 *v ) {
    mat3 scaleMat;
    mat3_scaleMatrixfv( &scaleMat, v );
    mat3_mul( mat, &scaleMat );
}


void mat3_rotationMatrixf( mat3 *mat, f32 angle ) {
    if( mat ) {
        angle = Deg2Rad( angle );
        f32 val[9] = {  cos( angle ), - sin( angle ),   0,
                        sin( angle ), cos( angle ),     0,
                        0,            0,                1 };

        memcpy( mat->x, val, 9 * sizeof( f32 ) );
    }
}

void mat3_rotatef( mat3 *mat, f32 angle ) {
    mat3 rot;
    mat3_rotationMatrixf( &rot, angle );
    mat3_mul( mat, &rot );
}


void mat3_ortho( mat3 *mat, f32 left, f32 right, f32 bottom, f32 top ) {
    if( mat ) {
        mat->x[0] = ( 2.f / ( right - left ) );
        mat->x[1] = 0.f;
        mat->x[2] = 0.f;

        mat->x[3] = 0.f;
        mat->x[4] = ( 2.f / ( top - bottom ) );
        mat->x[5] = 0.f;

        mat->x[6] = - ( right + left ) / ( right - left );
        mat->x[7] = - ( top + bottom ) / ( top - bottom );
        mat->x[8] = 0.f;
    }
}

