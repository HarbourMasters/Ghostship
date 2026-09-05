#include "GeoLayoutParser.h"

#include <unordered_map>
#include <libultraship.h>

#include "port/Engine.h"
extern "C" {
#include "sm64.h"
}
#include "port/events/Events.h"
#include "port/interpolation/FrameInterpolation.h"

#include "engine/geo_layout.h"
#include "game/paintings.h"
#include "menu/intro_geo.h"
#include "engine/graph_node.h"
#include "game/camera.h"
#include "game/geo_misc.h"
#include "engine/math_util.h"
#include "menu/file_select.h"
#include "menu/star_select.h"
#include "game/object_helpers.h"
#include "game/behavior_actions.h"
#include "game/mario_misc.h"
#include "game/mario_actions_cutscene.h"
#include "game/level_geo.h"
#include "game/moving_texture.h"
#include "game/screen_transition.h"

Ship::BinaryReader* GeoLayoutParser::mReader = nullptr;

typedef void (*GeoCommandFunction)();
#undef cur_geo_cmd_ptr

struct GraphNodeEntry {
    const char* name;
    GraphNodeFunc function;
};

#define FUNC(f)                                  \
    GraphNodeEntry {                             \
#f, reinterpret_cast < GraphNodeFunc>(f) \
    }

std::unordered_map<uint32_t, GraphNodeEntry> mUSFunctionTable = {
    { 0x8016f670, FUNC(geo_intro_super_mario_64_logo) },
    { 0x8016f984, FUNC(geo_intro_tm_copyright) },
    { 0x8016fe70, FUNC(geo_intro_regular_backdrop) },
    { 0x8016fffc, FUNC(geo_intro_gameover_backdrop) },
    { 0x80257198, FUNC(geo_switch_peach_eyes) },
    { 0x802761d0, FUNC(geo_envfx_main) },
    { 0x802763d4, FUNC(geo_skybox_main) },
    { 0x802764b0, FUNC(geo_draw_mario_head_goddard) },
    { 0x802770a4, FUNC(geo_mirror_mario_set_alpha) },
    { 0x80277150, FUNC(geo_switch_mario_stand_run) },
    { 0x802771bc, FUNC(geo_switch_mario_eyes) },
    { 0x80277294, FUNC(geo_mario_tilt_torso) },
    { 0x802773a4, FUNC(geo_mario_head_rotation) },
    { 0x802774f4, FUNC(geo_switch_mario_hand) },
    { 0x802775cc, FUNC(geo_mario_hand_foot_scaler) },
    { 0x802776d8, FUNC(geo_switch_mario_cap_effect) },
    { 0x80277740, FUNC(geo_switch_mario_cap_on_off) },
    { 0x80277824, FUNC(geo_mario_rotate_wing_cap_wings) },
    { 0x8027795c, FUNC(geo_switch_mario_hand_grab_pos) },
    { 0x80277b14, FUNC(geo_render_mirror_mario) },
    { 0x80277d6c, FUNC(geo_mirror_mario_backface_culling) },
    { 0x80287d30, FUNC(geo_camera_main) },
    { 0x8029aa3c, FUNC(geo_camera_fov) },
    { 0x8029d890, FUNC(geo_update_projectile_pos_from_parent) },
    { 0x8029d924, FUNC(geo_update_layer_transparency) },
    { 0x8029db48, FUNC(geo_switch_anim_state) },
    { 0x8029dbd4, FUNC(geo_switch_area) },
    { 0x802a45e4, FUNC(geo_offset_klepto_held_object) },
    { 0x802a719c, FUNC(geo_update_held_mario_pos) },
    { 0x802b1bb0, FUNC(geo_move_mario_part_from_parent) },
    { 0x802b798c, FUNC(geo_update_body_rot_from_parent) },
    { 0x802b7c64, FUNC(geo_switch_bowser_eyes) },
    { 0x802b7d44, FUNC(geo_bits_bowser_coloring) },
    { 0x802ba2b0, FUNC(geo_scale_bowser_key) },
    { 0x802bfbac, FUNC(geo_switch_tuxie_mother_eyes) },
    { 0x802cd1e8, FUNC(geo_cannon_circle_base) },
    { 0x802d0080, FUNC(geo_wdw_set_initial_water_level) },
    { 0x802d01e0, FUNC(geo_movtex_pause_control) },
    { 0x802d104c, FUNC(geo_movtex_draw_water_regions) },
    { 0x802d1b70, FUNC(geo_movtex_draw_nocolor) },
    { 0x802d1cdc, FUNC(geo_movtex_draw_colored) },
    { 0x802d1e48, FUNC(geo_movtex_draw_colored_no_update) },
    { 0x802d1fa8, FUNC(geo_movtex_draw_colored_2_no_update) },
    { 0x802d2108, FUNC(geo_movtex_update_horizontal) },
    { 0x802d2360, FUNC(geo_exec_inside_castle_light) },
    { 0x802d2470, FUNC(geo_exec_flying_carpet_timer_update) },
    { 0x802d2520, FUNC(geo_exec_flying_carpet_create) },
    { 0x802d28cc, FUNC(geo_exec_cake_end_screen) },
    { 0x802d5b98, FUNC(geo_painting_draw) },
    { 0x802d5d0c, FUNC(geo_painting_update) },
    { 0x8030d93c, FUNC(geo_snufit_move_mask) },
    { 0x8030d9ac, FUNC(geo_snufit_scale_body) },
    { 0x80177518, FUNC(geo_act_selector_strings) },
    { 0x80176688, FUNC(geo_file_select_strings_and_menu_cursor) },
};

std::unordered_map<uint32_t, GraphNodeEntry> mJPFunctionTable = {
    { 0x8029d3cc, FUNC(geo_switch_anim_state) },
    { 0x8029d194, FUNC(geo_update_layer_transparency) },
    { 0x8029a320, FUNC(geo_camera_fov) },
    { 0x80287748, FUNC(geo_camera_main) },
    { 0x8016f5d0, FUNC(geo_intro_super_mario_64_logo) },
    { 0x8016f8e4, FUNC(geo_intro_tm_copyright) },
    { 0x8016fdd0, FUNC(geo_intro_regular_backdrop) },
    { 0x8016ff5c, FUNC(geo_intro_gameover_backdrop) },
    { 0x8029d458, FUNC(geo_switch_area) },
    { 0x802d50b8, FUNC(geo_painting_draw) },
    { 0x8029d100, FUNC(geo_update_projectile_pos_from_parent) },
    { 0x802a6938, FUNC(geo_update_held_mario_pos) },
    { 0x802a3e3c, FUNC(geo_offset_klepto_held_object) },
    { 0x802bf1e0, FUNC(geo_switch_tuxie_mother_eyes) },
    { 0x8030c83c, FUNC(geo_snufit_move_mask) },
    { 0x8030c8ac, FUNC(geo_snufit_scale_body) },
    { 0x802b7034, FUNC(geo_update_body_rot_from_parent) },
    { 0x802b730c, FUNC(geo_switch_bowser_eyes) },
    { 0x802b73ec, FUNC(geo_bits_bowser_coloring) },
    { 0x80276df4, FUNC(geo_mario_head_rotation) },
    { 0x80277190, FUNC(geo_switch_mario_cap_on_off) },
    { 0x80276c0c, FUNC(geo_switch_mario_eyes) },
    { 0x80277274, FUNC(geo_mario_rotate_wing_cap_wings) },
    { 0x80276f44, FUNC(geo_switch_mario_hand) },
    { 0x8027701c, FUNC(geo_mario_hand_foot_scaler) },
    { 0x802773ac, FUNC(geo_switch_mario_hand_grab_pos) },
    { 0x802b1310, FUNC(geo_move_mario_part_from_parent) },
    { 0x80276ce4, FUNC(geo_mario_tilt_torso) },
    { 0x80277128, FUNC(geo_switch_mario_cap_effect) },
    { 0x802777bc, FUNC(geo_mirror_mario_backface_culling) },
    { 0x80276af4, FUNC(geo_mirror_mario_set_alpha) },
    { 0x80276ba0, FUNC(geo_switch_mario_stand_run) },
    { 0x802b9958, FUNC(geo_scale_bowser_key) },
    { 0x80256f20, FUNC(geo_switch_peach_eyes) },
    { 0x80275e24, FUNC(geo_skybox_main) },
    { 0x802cf700, FUNC(geo_movtex_pause_control) },
    { 0x802d056c, FUNC(geo_movtex_draw_water_regions) },
    { 0x80275c20, FUNC(geo_envfx_main) },
    { 0x802cc708, FUNC(geo_cannon_circle_base) },
    { 0x802d1090, FUNC(geo_movtex_draw_nocolor) },
    { 0x802d14c8, FUNC(geo_movtex_draw_colored_2_no_update) },
    { 0x802d1628, FUNC(geo_movtex_update_horizontal) },
    { 0x802d11fc, FUNC(geo_movtex_draw_colored) },
    { 0x80275f00, FUNC(geo_draw_mario_head_goddard) },
    { 0x802d522c, FUNC(geo_painting_update) },
    { 0x802d1368, FUNC(geo_movtex_draw_colored_no_update) },
    { 0x802d1a40, FUNC(geo_exec_flying_carpet_create) },
    { 0x802d1990, FUNC(geo_exec_flying_carpet_timer_update) },
    { 0x802d1dec, FUNC(geo_exec_cake_end_screen) },
    { 0x802cf5a0, FUNC(geo_wdw_set_initial_water_level) },
    { 0x802d1880, FUNC(geo_exec_inside_castle_light) },
    { 0x80277564, FUNC(geo_render_mirror_mario) },
    { 0x801773D8, FUNC(geo_act_selector_strings) },
    { 0x8017654C, FUNC(geo_file_select_strings_and_menu_cursor) },
};

std::unordered_map<uint32_t, std::unordered_map<uint32_t, GraphNodeEntry>> mFunctionTable = {
    { GAME_VERSION_US, mUSFunctionTable }, { GAME_VERSION_JP, mJPFunctionTable }
};

GraphNodeFunc GetFunctionByAddr(const uint32_t addr, std::string opcode) {
    const auto version = GameEngine::Instance->GetGameVersion();
    auto table = mFunctionTable[version];

    if (addr == 0) {
        return nullptr;
    }

    if (!table.contains(addr)) {
        SPDLOG_ERROR("Function table does not contain address: 0x{:X} on {}", addr, opcode);
        return nullptr;
    }

    return table[addr].function;
}

void ReadVec3f(Vec3f dst) {
    dst[0] = GeoLayoutParser::mReader->ReadFloat();
    dst[1] = GeoLayoutParser::mReader->ReadFloat();
    dst[2] = GeoLayoutParser::mReader->ReadFloat();
}

void ReadVec3sToVec3f(Vec3f dst) {
    dst[0] = GeoLayoutParser::mReader->ReadInt16();
    dst[1] = GeoLayoutParser::mReader->ReadInt16();
    dst[2] = GeoLayoutParser::mReader->ReadInt16();
}

void ReadVec3s(Vec3s dst) {
    dst[0] = GeoLayoutParser::mReader->ReadInt16();
    dst[1] = GeoLayoutParser::mReader->ReadInt16();
    dst[2] = GeoLayoutParser::mReader->ReadInt16();
}

void ReadVec3sAngle(Vec3s dst) {
    dst[0] = (GeoLayoutParser::mReader->ReadInt16() << 15) / 180;
    dst[1] = (GeoLayoutParser::mReader->ReadInt16() << 15) / 180;
    dst[2] = (GeoLayoutParser::mReader->ReadInt16() << 15) / 180;
}

uint64_t ReadSafeCrc() {
    const auto crc = GeoLayoutParser::mReader->ReadUInt64();
    if (crc != 0) {
        const auto name = ResourceGetNameByCrc(crc);
        SPDLOG_TRACE("CRC: {:X} Name: {}", crc, name);
    } else {
        SPDLOG_TRACE("Found NULL CRC");
    }
    return crc;
}

void process_cmd_branch_and_link() {
    const auto crc = ReadSafeCrc();

    const auto data = static_cast<char*>(ResourceGetDataByCrc(crc));
    const auto size = ResourceGetSizeByCrc(crc);

    gGeoLayoutStack[gGeoLayoutStackIndex++] = reinterpret_cast<uintptr_t>(GeoLayoutParser::mReader);
    gGeoLayoutStack[gGeoLayoutStackIndex++] = (gCurGraphNodeIndex << 16) + gGeoLayoutReturnIndex;
    gGeoLayoutReturnIndex = gGeoLayoutStackIndex;
    GeoLayoutParser::mReader = new Ship::BinaryReader(data, size);
}

void process_cmd_end() {
    delete GeoLayoutParser::mReader;

    gGeoLayoutStackIndex = gGeoLayoutReturnIndex;
    gGeoLayoutReturnIndex = gGeoLayoutStack[--gGeoLayoutStackIndex] & 0xFFFF;
    gCurGraphNodeIndex = gGeoLayoutStack[gGeoLayoutStackIndex] >> 16;
    GeoLayoutParser::mReader = reinterpret_cast<Ship::BinaryReader*>(gGeoLayoutStack[--gGeoLayoutStackIndex]);
}

void process_cmd_branch() {
    const auto param = GeoLayoutParser::mReader->ReadUByte();
    const auto crc = ReadSafeCrc();

    const auto name = ResourceGetNameByCrc(crc);
    const auto data = static_cast<char*>(ResourceGetDataByCrc(crc));
    const auto size = ResourceGetSizeByCrc(crc);

    if (param == 1) {
        gGeoLayoutStack[gGeoLayoutStackIndex++] = reinterpret_cast<uintptr_t>(GeoLayoutParser::mReader);
        SPDLOG_TRACE("Branching and returning to {}:{}", name, gGeoLayoutStackIndex);
    } else {
        delete GeoLayoutParser::mReader;
    }

    GeoLayoutParser::mReader = new Ship::BinaryReader(data, size);
}

void process_cmd_return() {
    delete GeoLayoutParser::mReader;
    GeoLayoutParser::mReader = reinterpret_cast<Ship::BinaryReader*>(gGeoLayoutStack[--gGeoLayoutStackIndex]);
    SPDLOG_TRACE("Returning to {}", gGeoLayoutStackIndex);
}

void process_cmd_open_node() {
    gCurGraphNodeList[gCurGraphNodeIndex + 1] = gCurGraphNodeList[gCurGraphNodeIndex];
    gCurGraphNodeIndex++;
}

void process_cmd_close_node() {
    gCurGraphNodeIndex--;
}

void process_cmd_assign_as_view() {
    const auto index = GeoLayoutParser::mReader->ReadUInt16();

    if (index < gGeoNumViews) {
        gGeoViews[index] = gCurGraphNodeList[gCurGraphNodeIndex];
    }
}

void process_cmd_update_node_flags() {
    const auto operation = GeoLayoutParser::mReader->ReadUByte();
    const auto flagBits = GeoLayoutParser::mReader->ReadInt16();

    switch (operation) {
        case GEO_CMD_FLAGS_RESET:
            gCurGraphNodeList[gCurGraphNodeIndex]->flags = flagBits;
            break;
        case GEO_CMD_FLAGS_SET:
            gCurGraphNodeList[gCurGraphNodeIndex]->flags |= flagBits;
            break;
        case GEO_CMD_FLAGS_CLEAR:
            gCurGraphNodeList[gCurGraphNodeIndex]->flags &= ~flagBits;
            break;
        default:
            throw std::runtime_error("Unknown flag");
    }
}

uint64_t generate_uuid64() {
    uint64_t high = rand();
    uint64_t low = rand();
    return (high << 32) | low;
}

void process_cmd_node_root() {
    gGeoNumViews = GeoLayoutParser::mReader->ReadInt16() + 2;
    const auto x = GeoLayoutParser::mReader->ReadInt16();
    const auto y = GeoLayoutParser::mReader->ReadInt16();
    const auto width = GeoLayoutParser::mReader->ReadInt16();
    const auto height = GeoLayoutParser::mReader->ReadInt16();

    GraphNodeRoot* graphNode = init_graph_node_root(gGraphNodePool, nullptr, 0, x, y, width, height);

    gGeoViews = static_cast<GraphNode**>(alloc_only_pool_alloc(gGraphNodePool, gGeoNumViews * sizeof(GraphNode*)));

    graphNode->nodeId = generate_uuid64();
    graphNode->views = gGeoViews;
    graphNode->numViews = gGeoNumViews;

    for (int32_t i = 0; i < gGeoNumViews; i++) {
        gGeoViews[i] = nullptr;
    }

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_ortho_projection() {
    const auto scale = GeoLayoutParser::mReader->ReadInt16() / 100.0f;

    GraphNodeOrthoProjection* graphNode = init_graph_node_ortho_projection(gGraphNodePool, nullptr, scale);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_perspective() {
    GraphNodeFunc frustumFunc = nullptr;

    const auto param = GeoLayoutParser::mReader->ReadUByte();
    auto fov = GeoLayoutParser::mReader->ReadInt16();
    auto _near = GeoLayoutParser::mReader->ReadInt16();
    auto _far = GeoLayoutParser::mReader->ReadInt16();

    if (param != 0) {
        const auto func = GeoLayoutParser::mReader->ReadUInt32();
        frustumFunc = GetFunctionByAddr(func, "NODE_PERSPECTIVE");
    }

    auto fovF = (f32)fov;
    CALL_CANCELLABLE_EVENT(GeoLayoutNodePerspective, &fovF, &_near, &_far, &frustumFunc) {
        GraphNodePerspective* graphNode =
            init_graph_node_perspective(gGraphNodePool, nullptr, fovF, _near, _far, frustumFunc, 0);
        register_scene_graph_node(&graphNode->fnNode.node);
    }
}

void process_cmd_node_start() {
    GraphNodeStart* graphNode = init_graph_node_start(gGraphNodePool, nullptr);
    register_scene_graph_node(&graphNode->node);
}

void process_cmd_nop3() {
    int bp = 0;
}

void process_cmd_node_master_list() {
    const auto enable = GeoLayoutParser::mReader->ReadUByte();
    GraphNodeMasterList* graphNode = init_graph_node_master_list(gGraphNodePool, nullptr, enable);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_level_of_detail() {
    const auto minDistance = GeoLayoutParser::mReader->ReadInt16();
    const auto maxDistance = GeoLayoutParser::mReader->ReadInt16();

    GraphNodeLevelOfDetail* graphNode = init_graph_node_render_range(gGraphNodePool, nullptr, minDistance, maxDistance);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_switch_case() {
    auto cs = GeoLayoutParser::mReader->ReadInt16();
    GraphNodeFunc func = GetFunctionByAddr(GeoLayoutParser::mReader->ReadUInt32(), "NODE_SWITCH_CASE");

    CALL_CANCELLABLE_EVENT(GeoLayoutNodeSwitchCase, &cs, &func) {
        GraphNodeSwitchCase* graphNode = init_graph_node_switch_case(gGraphNodePool, nullptr, cs, 0, func, 0);
        register_scene_graph_node(&graphNode->fnNode.node);
    }
}

void process_cmd_node_camera() {
    Vec3f pos, focus;

    auto type = GeoLayoutParser::mReader->ReadInt16();

    ReadVec3f(pos);
    ReadVec3f(focus);

    GraphNodeFunc func = GetFunctionByAddr(GeoLayoutParser::mReader->ReadUInt32(), "NODE_CAMERA");

    CALL_CANCELLABLE_EVENT(GeoLayoutNodeCamera, &type, pos, focus, &func) {
        GraphNodeCamera* graphNode = init_graph_node_camera(gGraphNodePool, nullptr, pos, focus, func, type);
        register_scene_graph_node(&graphNode->fnNode.node);
        gGeoViews[0] = &graphNode->fnNode.node;
    }
}

void process_cmd_node_translation_rotation() {
    Vec3s translation, rotation;

    void* displayList = nullptr;
    s16 drawingLayer = 0;

    const auto params = GeoLayoutParser::mReader->ReadUByte();

    switch ((params & 0x70) >> 4) {
        case 0:
            ReadVec3s(translation);
            ReadVec3sAngle(rotation);
            break;
        case 1:
            ReadVec3s(translation);
            vec3s_copy(rotation, gVec3sZero);
            break;
        case 2:
            ReadVec3sAngle(rotation);
            vec3s_copy(translation, gVec3sZero);
            break;
        case 3:
            const auto y = GeoLayoutParser::mReader->ReadInt16();
            vec3s_copy(translation, gVec3sZero);
            vec3s_set(rotation, 0, (y << 15) / 180, 0);
            break;
    }

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(ReadSafeCrc());
        drawingLayer = params & 0x0F;
    }

    GraphNodeTranslationRotation* graphNode =
        init_graph_node_translation_rotation(gGraphNodePool, nullptr, drawingLayer, displayList, translation, rotation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_translation() {
    Vec3s translation;

    const auto params = GeoLayoutParser::mReader->ReadUByte();
    s16 drawingLayer = 0;
    void* displayList = nullptr;

    ReadVec3s(translation);

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(ReadSafeCrc());
        drawingLayer = params & 0x0F;
        SPDLOG_TRACE("Current Offset {}", GeoLayoutParser::mReader->GetBaseAddress());
    }

    GraphNodeTranslation* graphNode =
        init_graph_node_translation(gGraphNodePool, nullptr, drawingLayer, displayList, translation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_rotation() {
    Vec3s rotation;

    const auto params = GeoLayoutParser::mReader->ReadUByte();
    s16 drawingLayer = 0;
    void* displayList = nullptr;

    ReadVec3sAngle(rotation);

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(ReadSafeCrc());
        drawingLayer = params & 0x0F;
    }

    GraphNodeRotation* graphNode =
        init_graph_node_rotation(gGraphNodePool, nullptr, drawingLayer, displayList, rotation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_scale() {
    s16 drawingLayer = 0;
    const auto params = GeoLayoutParser::mReader->ReadUByte();
    const auto scale = GeoLayoutParser::mReader->ReadUInt32() / 65536.0f;
    void* displayList = nullptr;

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(ReadSafeCrc());
        drawingLayer = params & 0x0F;
    }

    GraphNodeScale* graphNode = init_graph_node_scale(gGraphNodePool, nullptr, drawingLayer, displayList, scale);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_nop2() {
}

void process_cmd_node_animated_part() {
    Vec3s translation;
    const auto drawingLayer = GeoLayoutParser::mReader->ReadUByte();

    ReadVec3s(translation);

    uint64_t crc = ReadSafeCrc();
    void* displayList = crc == 0 ? nullptr : ResourceGetDataByCrc(crc);

    GraphNodeAnimatedPart* graphNode =
        init_graph_node_animated_part(gGraphNodePool, nullptr, drawingLayer, displayList, translation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_billboard() {
    Vec3s translation;
    s16 drawingLayer = 0;
    const auto params = GeoLayoutParser::mReader->ReadUByte();
    void* displayList = nullptr;

    ReadVec3s(translation);

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(ReadSafeCrc());
        drawingLayer = params & 0x0F;
    }

    GraphNodeBillboard* graphNode =
        init_graph_node_billboard(gGraphNodePool, nullptr, drawingLayer, displayList, translation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_display_list() {
    const auto drawingLayer = GeoLayoutParser::mReader->ReadUByte();
    void* displayList = ResourceGetDataByCrc(ReadSafeCrc());

    GraphNodeDisplayList* graphNode = init_graph_node_display_list(gGraphNodePool, nullptr, drawingLayer, displayList);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_shadow() {
    const auto type = GeoLayoutParser::mReader->ReadInt16();
    const auto solidity = GeoLayoutParser::mReader->ReadInt16();
    const auto scale = GeoLayoutParser::mReader->ReadInt16();

    GraphNodeShadow* graphNode = init_graph_node_shadow(gGraphNodePool, nullptr, scale, solidity, type);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_object_parent() {
    GraphNodeObjectParent* graphNode = init_graph_node_object_parent(gGraphNodePool, nullptr, &gObjParentGraphNode);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_generated() {
    auto param = GeoLayoutParser::mReader->ReadInt16();
    const auto addr = GeoLayoutParser::mReader->ReadUInt32();

    GraphNodeFunc func = GetFunctionByAddr(addr, "NODE_ASM");

    CALL_CANCELLABLE_EVENT(GeoLayoutCallASM, &func, &param) {
        GraphNodeGenerated* graphNode = init_graph_node_generated(gGraphNodePool, nullptr, func, // asm function
                                                                  param);                        // parameter

        register_scene_graph_node(&graphNode->fnNode.node);
    }
}

void process_cmd_node_background() {
    auto param = GeoLayoutParser::mReader->ReadInt16();
    const auto addr = GeoLayoutParser::mReader->ReadUInt32();
    GraphNodeFunc func = GetFunctionByAddr(addr, "NODE_BACKGROUND");

    CALL_CANCELLABLE_EVENT(GeoLayoutCallASM, &func, &param) {
        GraphNodeBackground* graphNode = init_graph_node_background(gGraphNodePool, nullptr, param, func, 0);
        register_scene_graph_node(&graphNode->fnNode.node);
    }
}

void process_cmd_nop() {
}

void process_cmd_copy_view() {
    GraphNode* node = nullptr;
    const auto index = GeoLayoutParser::mReader->ReadInt16();

    if (index >= 0) {
        node = gGeoViews[index];

        if (node->type == GRAPH_NODE_TYPE_OBJECT_PARENT) {
            node = reinterpret_cast<GraphNodeObjectParent*>(node)->sharedChild;
        } else {
            node = nullptr;
        }
    }

    GraphNodeObjectParent* graphNode = init_graph_node_object_parent(gGraphNodePool, nullptr, node);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_held_obj() {
    Vec3s offset;

    const auto addr = GeoLayoutParser::mReader->ReadUInt32();
    const auto player = GeoLayoutParser::mReader->ReadUByte();

    ReadVec3s(offset);

    GraphNodeHeldObject* graphNode = init_graph_node_held_object(gGraphNodePool, nullptr, nullptr, offset,
                                                                 GetFunctionByAddr(addr, "NODE_HELD_OBJ"), player);

    register_scene_graph_node(&graphNode->fnNode.node);
}

void process_cmd_node_culling_radius() {
    const auto radius = GeoLayoutParser::mReader->ReadInt16();
    GraphNodeCullingRadius* graphNode = init_graph_node_culling_radius(gGraphNodePool, nullptr, radius);
    register_scene_graph_node(&graphNode->node);
}

GeoCommandFunction GeoLayoutFunctionTable[] = {
    process_cmd_branch_and_link,
    process_cmd_end,
    process_cmd_branch,
    process_cmd_return,
    process_cmd_open_node,
    process_cmd_close_node,
    process_cmd_assign_as_view,
    process_cmd_update_node_flags,
    process_cmd_node_root,
    process_cmd_node_ortho_projection,
    process_cmd_node_perspective,
    process_cmd_node_start,
    process_cmd_node_master_list,
    process_cmd_node_level_of_detail,
    process_cmd_node_switch_case,
    process_cmd_node_camera,
    process_cmd_node_translation_rotation,
    process_cmd_node_translation,
    process_cmd_node_rotation,
    process_cmd_node_animated_part,
    process_cmd_node_billboard,
    process_cmd_node_display_list,
    process_cmd_node_shadow,
    process_cmd_node_object_parent,
    process_cmd_node_generated,
    process_cmd_node_background,
    process_cmd_nop,
    process_cmd_copy_view,
    process_cmd_node_held_obj,
    process_cmd_node_scale,
    process_cmd_nop2,
    process_cmd_nop3,
    process_cmd_node_culling_radius,
};

void GeoLayoutParser::execute(const char* path) {
    const auto data = static_cast<char*>(ResourceGetDataByName(path));
    const auto size = ResourceGetSizeByName(path);

    mReader = new Ship::BinaryReader(data, size);

    while (mReader != nullptr) {
        const auto cmdId = mReader->ReadUByte();
        FrameInterpolation_RecordOpenChild("GeoLayout Command", cmdId);
        GeoLayoutFunctionTable[cmdId]();
        FrameInterpolation_RecordCloseChild();
    }

    delete mReader;
    mReader = nullptr;
}

extern "C" void GeoLayoutExecute(char const* path) {
    SPDLOG_TRACE("Executing GeoLayout: {}", path);
    GeoLayoutParser::execute(path);
}