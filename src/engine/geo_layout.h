#ifndef GEO_LAYOUT_H
#define GEO_LAYOUT_H

#include <libultra/types.h>

#include "game/memory.h"
#include "macros.h"
#include "types.h"

#define GEO_CMD_FLAGS_RESET 0
#define GEO_CMD_FLAGS_SET   1
#define GEO_CMD_FLAGS_CLEAR 2

#define CMD_SIZE_SHIFT (sizeof(void *) >> 3)
#define CMD_PROCESS_OFFSET(offset) (((offset) & 3) | (((offset) & ~3) << CMD_SIZE_SHIFT))

#define cur_geo_cmd_u8(offset) \
    (gGeoLayoutCommand[CMD_PROCESS_OFFSET(offset)])

#define cur_geo_cmd_s16(offset) \
    (*(s16 *) &gGeoLayoutCommand[CMD_PROCESS_OFFSET(offset)])

#define cur_geo_cmd_s32(offset) \
    (*(s32 *) &gGeoLayoutCommand[CMD_PROCESS_OFFSET(offset)])

#define cur_geo_cmd_u32(offset) \
    (*(u32 *) &gGeoLayoutCommand[CMD_PROCESS_OFFSET(offset)])

#define cur_geo_cmd_ptr(offset) \
    (*(void **) &gGeoLayoutCommand[CMD_PROCESS_OFFSET(offset)])

extern_s struct AllocOnlyPool *gGraphNodePool;
extern_s struct GraphNode *gCurRootGraphNode;
extern_s UNUSED s32 D_8038BCA8;
extern_s struct GraphNode **gGeoViews;
extern_s u16 gGeoNumViews;
extern_s uintptr_t gGeoLayoutStack[];
extern_s struct GraphNode *gCurGraphNodeList[];
extern_s s16 gCurGraphNodeIndex;
extern_s s16 gGeoLayoutStackIndex;
extern_s UNUSED s16 D_8038BD7C;
extern_s s16 gGeoLayoutReturnIndex;
extern_s u8 *gGeoLayoutCommand;
extern_s struct GraphNode gObjParentGraphNode;

extern_s struct AllocOnlyPool *D_8038BCA0;
extern_s struct GraphNode *D_8038BCA4;
extern_s s16 D_8038BD78;
extern_s struct GraphNode *D_8038BCF8[];

extern_s void geo_layout_cmd_branch_and_link(void);
extern_s void geo_layout_cmd_end(void);
extern_s void geo_layout_cmd_branch(void);
extern_s void geo_layout_cmd_return(void);
extern_s void geo_layout_cmd_open_node(void);
extern_s void geo_layout_cmd_close_node(void);
extern_s void geo_layout_cmd_assign_as_view(void);
extern_s void geo_layout_cmd_update_node_flags(void);
extern_s void geo_layout_cmd_node_root(void);
extern_s void geo_layout_cmd_node_ortho_projection(void);
extern_s void geo_layout_cmd_node_perspective(void);
extern_s void geo_layout_cmd_node_start(void);
extern_s void geo_layout_cmd_nop3(void);
extern_s void geo_layout_cmd_node_master_list(void);
extern_s void geo_layout_cmd_node_level_of_detail(void);
extern_s void geo_layout_cmd_node_switch_case(void);
extern_s void geo_layout_cmd_node_camera(void);
extern_s void geo_layout_cmd_node_translation_rotation(void);
extern_s void geo_layout_cmd_node_translation(void);
extern_s void geo_layout_cmd_node_rotation(void);
extern_s void geo_layout_cmd_node_scale(void);
extern_s void geo_layout_cmd_nop2(void);
extern_s void geo_layout_cmd_node_animated_part(void);
extern_s void geo_layout_cmd_node_billboard(void);
extern_s void geo_layout_cmd_node_display_list(void);
extern_s void geo_layout_cmd_node_shadow(void);
extern_s void geo_layout_cmd_node_object_parent(void);
extern_s void geo_layout_cmd_node_generated(void);
extern_s void geo_layout_cmd_node_background(void);
extern_s void geo_layout_cmd_nop(void);
extern_s void geo_layout_cmd_copy_view(void);
extern_s void geo_layout_cmd_node_held_obj(void);
extern_s void geo_layout_cmd_node_culling_radius(void);

extern_s struct GraphNode *process_geo_layout(struct AllocOnlyPool *a0, void *segptr);

#endif // GEO_LAYOUT_H
