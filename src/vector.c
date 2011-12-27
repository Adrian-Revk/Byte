#include "vector.h"

vec2 vec2_add( const vec2 *A, const vec2 *B ) {
    vec2 ret;
    ret.x = A->x + B->x;
    ret.y = A->y + B->y;

    return ret;
}

vec2 vec2_sub( const vec2 *A, const vec2 *B ) {
    vec2 ret;
    ret.x = A->x - B->x;
    ret.y = A->y - B->y;

    return ret;
}

vec2 vec2_mul( const vec2 *A, f32 pFactor ) {
    vec2 ret;
    ret.x = A->x * pFactor;
    ret.y = A->y * pFactor;

    return ret;
}

vec2 vec2_div( const vec2 *A, f32 pFactor ) {
    vec2 ret;
    ret.x = A->x / pFactor;
    ret.y = A->y / pFactor;

    return ret;
}


f32  vec2_len( const vec2 *A ) {
    return sqrtf( vec2_sqlen( A ) );
}

f32  vec2_sqlen( const vec2 *A ) {
    return A->x * A->x + A->y * A->y;
}


f32  vec2_dot( const vec2 *A, const vec2 *B ) {
    return A->x * B->y + A->y * B->y;
}

void vec2_normalize( vec2 *A ) {
    f32 len = vec2_len( A );
    if( len > M_EPS ) 
        *A = vec2_div( A, len );
}


