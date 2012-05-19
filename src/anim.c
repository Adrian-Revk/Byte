#include "anim.h"
#include "json/cJSON.h"

#include <dirent.h>

str32 AnimDirectory = "data/animations/";

str32 Animations_str[ANIM_N] = {
    "man_idle"
};

void Anim_loadFromFile( Anim *a, const char *file ) {
    char *file_contents = NULL;
    cJSON *root = NULL;

    Byte_ReadFile( &file_contents, file );
    check( file_contents, " " );

    root = cJSON_Parse( file_contents );
    check( root, "JSON parse error [%s] before :\n%s\n", file, cJSON_GetErrorPtr() );

    cJSON *frames = cJSON_GetObjectItem( root, "frames" );
    check( frames, "Animation '%s' does not have any frame!\n", file );

    a->frame_n = cJSON_GetArraySize( frames );
    check( a->frame_n > 0, "Animation '%s' does not have any frame!\n", file );

    a->frames = byte_alloc( a->frame_n * sizeof(vec2) );

    for( int i = 0; i < a->frame_n; ++i ) {
        cJSON *frame = cJSON_GetArrayItem( frames, i );

        // frames positions are position on a 2048^2 texture. find their relative pos to it :
        if( frame ) 
            a->frames[i] = (vec2){ cJSON_GetArrayItem( frame, 0 )->valuedouble / 2048.f,
                                   cJSON_GetArrayItem( frame, 1 )->valuedouble / 2048.f };
    }

    cJSON *ft = cJSON_GetObjectItem( root, "frame_time" );
    check( ft, "Animation '%s' does not have a frame time!\n", file );

    a->frame_t = ft->valuedouble;
    a->curr_t = 0.f;
    a->curr_n = 0;
    a->running = false;


    DEL_PTR( file_contents );
    if( root ) cJSON_Delete( root );

    return;

error:
    Anim_destroy( a );
    DEL_PTR( file_contents );
    if( root ) cJSON_Delete( root );
}

inline void Anim_destroy( Anim *a ) {
    if( a ) 
        DEL_PTR( a->frames );
}

inline bool Anim_update( Anim *a, f32 frame_time ) {
    if( a->running ) {
        a->curr_t += frame_time;
        
        if( a->curr_t >= a->frame_t ) {
            a->curr_t = 0.f;
            a->curr_n = (a->curr_n + 1) % a->frame_n;
            return true;
        }
    }
    return false;
}

inline void Anim_start( Anim *a ) {
    a->running = true;
}

inline void Anim_stop( Anim *a ) {
    a->running = false;
    a->curr_n = 0;
    a->curr_t = 0.f;
}

inline void Anim_restart( Anim *a ) {
    Anim_stop( a );
    Anim_start( a );
}

inline void Anim_pause( Anim *a ) {
    a->running = false;
}

inline void Anim_toggle( Anim *a ) {
    a->running ? Anim_pause( a ) : Anim_start( a );
}


void AnimManager_loadAll( AnimManager *am ) {
    if( am ) {
        struct dirent *entry = NULL;

        DIR *anim_dir = opendir( AnimDirectory );

        while( ( entry = readdir( anim_dir ) ) ) {
            const char *file = entry->d_name;

            if( CheckExtension( file, "json" ) ) {
                str32 anim_name;

                // remove '.json. extension from name
                int filename_n = strlen( file );
                strncpy( anim_name, file, filename_n - 5 );
                anim_name[filename_n-5] = 0;

                // find anim that match the name
                Anim *a = AnimManager_gets( am, anim_name );

                if( a ) {
                    str64 anim_path;
                    strcpy( anim_path, AnimDirectory );
                    strcat( anim_path, file );
                    Anim_loadFromFile( a, anim_path );
                }
            }
        }
    }
}

inline void AnimManager_unloadAll( AnimManager *am ) {
    if( am ) {
        for( int i = 0; i < ANIM_N; ++i )
            Anim_destroy( &am->animations[i] );
    }
}
 
inline Anim *AnimManager_gets( AnimManager *am, const char *anim_name ) {
    Anim *a = NULL;
    if( am && anim_name ) {
        int i;
        for( i = 0; i < ANIM_N; ++i )
            if( !strcmp( Animations_str[i], anim_name ) )
                break;
       
        if( i < ANIM_N )
            a = &am->animations[i];
        else
            log_err( "Animation '%s' is inexistant\n", anim_name );
    }

    return a;
}
