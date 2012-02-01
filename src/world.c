#include "world.h"
#include "handlemanager.h"
#include "resource.h"

typedef struct {
    HandleManager   mActors;

    ResourceManager *mResourceManager;  
} World;

World *world = NULL;

bool World_init() {
    world = byte_alloc( sizeof( World ) );
    check_mem( world );

    world->mResourceManager = ResourceManager_new();
    check( world->mResourceManager, "Error while creating world's resource manager!\n" );

    return true;

error:
    World_destroy();
    return false;
}

void World_destroy() {
    if( world ) {
        ResourceManager_destroy( world->mResourceManager );
        DEL_PTR( world );
    }
}   

int  World_getResource( const char *pFile ) {
    if( world && pFile) {
        return ResourceManager_getResource( world->mResourceManager, pFile ); 
    }
    return -1;
}

bool World_loadAllResources() {
    if( world ) {
       return ResourceManager_loadAllResources( world->mResourceManager );
    }
    return false;
}
