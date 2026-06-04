#ifndef LEVEL_GEO_H
#define LEVEL_GEO_H

#include <libultra/types.h>
#include <libultra/gbi.h> 

extern_s Gfx *geo_envfx_main(s32 callContext, struct GraphNode *node, Mat4 mtxf);
extern_s Gfx *geo_skybox_main(s32 callContext, struct GraphNode *node, UNUSED Mat4 *mtx);

#endif // LEVEL_GEO_H
