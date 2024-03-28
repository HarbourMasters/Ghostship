#include "GeoLayoutParser.h"

#include <unordered_map>
#include "port/Engine.h"
#include <libultraship.h>

extern "C" {
#include "engine/geo_layout.h"
#include "engine/graph_node.h"
#include "engine/math_util.h"

// Function Table
#include "menu/intro_geo.h"
}
LUS::BinaryReader* GeoLayoutParser::mReader;
LUS::BinaryReader* GeoLayoutParser::mLoadedReader;

typedef void (*GeoCommandFunction)();
#undef cur_geo_cmd_ptr

std::unordered_map<uint32_t, GraphNodeFunc> mUSFunctionTable = {
    { 0x8016F670, geo_intro_super_mario_64_logo },
    { 0x8016f984, geo_intro_tm_copyright },
    { 0x8016fe70, geo_intro_regular_backdrop },
    { 0x8016fffc, geo_intro_gameover_backdrop }
};

std::unordered_map<uint32_t, GraphNodeFunc> mJPFunctionTable = {};

std::unordered_map<uint32_t, std::unordered_map<uint32_t, GraphNodeFunc>> mFunctionTable = {
    { 0xFF2B5A63, mUSFunctionTable },
    { 0xE3DAA4E0, mJPFunctionTable }
};

GraphNodeFunc GetFunctionByAddr(const uint32_t addr) {
    auto table = mFunctionTable[GameEngine::Instance->GetGameVersion()];

    if(!table.contains(addr)) {
        SPDLOG_ERROR("Function table does not contain address: 0x{:X}", addr);
        return nullptr;
    }

    return table[addr];
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


void process_cmd_branch_and_link() {
    const auto crc = GeoLayoutParser::mReader->ReadUInt64();

    const auto data = static_cast<char*>(ResourceGetDataByCrc(crc));
    const auto size = ResourceGetSizeByCrc(crc);
    GeoLayoutParser::mReader = new LUS::BinaryReader(data, size);

    gGeoLayoutStack[gGeoLayoutStackIndex++] = reinterpret_cast<uintptr_t>(GeoLayoutParser::mLoadedReader);
    gGeoLayoutStack[gGeoLayoutStackIndex++] = (gCurGraphNodeIndex << 16) + gGeoLayoutReturnIndex;
    gGeoLayoutReturnIndex = gGeoLayoutStackIndex;
}

void process_cmd_end() {
    gGeoLayoutStackIndex = gGeoLayoutReturnIndex;
    gGeoLayoutReturnIndex = gGeoLayoutStack[--gGeoLayoutStackIndex] & 0xFFFF;
    gCurGraphNodeIndex = gGeoLayoutStack[gGeoLayoutStackIndex] >> 16;
    GeoLayoutParser::mReader = reinterpret_cast<LUS::BinaryReader*>(gGeoLayoutStack[--gGeoLayoutStackIndex]);
}

void process_cmd_branch() {
    const auto param = GeoLayoutParser::mReader->ReadUByte();
    const auto crc = GeoLayoutParser::mReader->ReadUInt32();

    const auto data = static_cast<char*>(ResourceGetDataByCrc(crc));
    const auto size = ResourceGetSizeByCrc(crc);

    if (param == 1) {
        gGeoLayoutStack[gGeoLayoutStackIndex++] = reinterpret_cast<uintptr_t>(GeoLayoutParser::mLoadedReader);
        GeoLayoutParser::mReader = new LUS::BinaryReader(data, size);
    } else {
        GeoLayoutParser::mLoadedReader = new LUS::BinaryReader(data, size);
        GeoLayoutParser::mReader = GeoLayoutParser::mLoadedReader;
    }
}

void process_cmd_return() {
    GeoLayoutParser::mReader = reinterpret_cast<LUS::BinaryReader*>(gGeoLayoutStack[--gGeoLayoutStackIndex]);
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

void process_cmd_node_root() {
    gGeoNumViews = GeoLayoutParser::mReader->ReadInt16() + 2;
    const auto x = GeoLayoutParser::mReader->ReadInt16();
    const auto y = GeoLayoutParser::mReader->ReadInt16();
    const auto width = GeoLayoutParser::mReader->ReadInt16();
    const auto height = GeoLayoutParser::mReader->ReadInt16();

    GraphNodeRoot* graphNode = init_graph_node_root(gGraphNodePool, nullptr, 0, x, y, width, height);

    gGeoViews = static_cast<GraphNode**>(alloc_only_pool_alloc(gGraphNodePool, gGeoNumViews * sizeof(GraphNode *)));

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
    const auto fov = GeoLayoutParser::mReader->ReadInt16();
    const auto near = GeoLayoutParser::mReader->ReadInt16();
    const auto far = GeoLayoutParser::mReader->ReadInt16();

    if (param != 0) {
        const auto func = GeoLayoutParser::mReader->ReadUInt32();
        frustumFunc = GetFunctionByAddr(func);
    }

    GraphNodePerspective* graphNode = init_graph_node_perspective(gGraphNodePool, nullptr, fov, near, far, frustumFunc, 0);

    register_scene_graph_node(&graphNode->fnNode.node);
}

void process_cmd_node_start() {
    GraphNodeStart* graphNode = init_graph_node_start(gGraphNodePool, nullptr);
    register_scene_graph_node(&graphNode->node);
}

void process_cmd_nop3() {}

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

    const auto cs = GeoLayoutParser::mReader->ReadInt16();
    const auto func = GeoLayoutParser::mReader->ReadUInt32();

    GraphNodeSwitchCase *graphNode =
        init_graph_node_switch_case(gGraphNodePool, nullptr,
                                    cs, // case which is initially selected
                                    0,
                                    GetFunctionByAddr(func), // case update function
                                    0);

    register_scene_graph_node(&graphNode->fnNode.node);
}

void process_cmd_node_camera() {

    Vec3f pos, focus;

    const auto type = GeoLayoutParser::mReader->ReadUInt16();
    
    ReadVec3f(pos);
    ReadVec3f(focus);

    const auto addr = GeoLayoutParser::mReader->ReadUInt32();

    GraphNodeCamera* graphNode = init_graph_node_camera(gGraphNodePool, nullptr, pos, focus,
                                        GetFunctionByAddr(addr), type);

    register_scene_graph_node(&graphNode->fnNode.node);

    gGeoViews[0] = &graphNode->fnNode.node;
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
        displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());
        drawingLayer = params & 0x0F;
    }

    GraphNodeTranslationRotation* graphNode = init_graph_node_translation_rotation(
        gGraphNodePool, nullptr,
        drawingLayer, displayList,
        translation, rotation
    );

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_translation() {
    Vec3s translation;

    const auto params = GeoLayoutParser::mReader->ReadUByte();
    s16 drawingLayer = 0;
    void *displayList = nullptr;

    ReadVec3s(translation);

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());
        drawingLayer = params & 0x0F;
        SPDLOG_INFO("Current Offset {}", GeoLayoutParser::mReader->GetBaseAddress());
    }

    GraphNodeTranslation* graphNode =
        init_graph_node_translation(gGraphNodePool, nullptr, drawingLayer, displayList, translation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_rotation() {
    Vec3s rotation;

    const auto params = GeoLayoutParser::mReader->ReadUByte();
    s16 drawingLayer = 0;
    void *displayList = nullptr;

    ReadVec3sAngle(rotation);

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());
        drawingLayer = params & 0x0F;
    }

    GraphNodeRotation* graphNode =
        init_graph_node_rotation(gGraphNodePool, nullptr, drawingLayer, displayList, rotation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_scale() {
    const auto params = GeoLayoutParser::mReader->ReadUByte();
    const auto scale = GeoLayoutParser::mReader->ReadUInt32() / 65536.0f;
    s16 drawingLayer = 0;
    void *displayList = nullptr;

    if (params & 0x80) {
        displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());
        drawingLayer = params & 0x0F;
    }

    GraphNodeScale* graphNode = init_graph_node_scale(gGraphNodePool, nullptr, drawingLayer, displayList, scale);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_nop2() {}

void process_cmd_node_animated_part() {
    Vec3s translation;
    const auto drawingLayer = GeoLayoutParser::mReader->ReadUByte();

    ReadVec3s(translation);

    void* displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());

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
        displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());
        drawingLayer = params & 0x0F;
    }

    GraphNodeBillboard* graphNode = init_graph_node_billboard(gGraphNodePool, nullptr, drawingLayer, displayList, translation);

    register_scene_graph_node(&graphNode->node);
}

void process_cmd_node_display_list() {
    const auto drawingLayer = GeoLayoutParser::mReader->ReadUByte();
    void *displayList = ResourceGetDataByCrc(GeoLayoutParser::mReader->ReadUInt64());

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
    const auto param = GeoLayoutParser::mReader->ReadInt16();
    const auto addr = GeoLayoutParser::mReader->ReadUInt32();

    GraphNodeGenerated* graphNode = init_graph_node_generated(gGraphNodePool, nullptr,
                                          GetFunctionByAddr(addr), // asm function
                                          param);                  // parameter

    register_scene_graph_node(&graphNode->fnNode.node);
}

void process_cmd_node_background() {
    const auto param = GeoLayoutParser::mReader->ReadInt16();
    const auto addr = GeoLayoutParser::mReader->ReadUInt32();

    GraphNodeBackground* graphNode = init_graph_node_background(
        gGraphNodePool, nullptr,
        param, // background ID, or RGBA5551 color if asm function is null
        GetFunctionByAddr(addr), // asm function
        0);

    register_scene_graph_node(&graphNode->fnNode.node);
}

void process_cmd_nop() {}

void process_cmd_copy_view() {
    GraphNode *node = nullptr;
    const auto index = GeoLayoutParser::mReader->ReadInt16();

    if (index >= 0) {
        node = gGeoViews[index];

        if (node->type == GRAPH_NODE_TYPE_OBJECT_PARENT) {
            node = reinterpret_cast<GraphNodeObjectParent *>(node)->sharedChild;
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

    GraphNodeHeldObject *graphNode = init_graph_node_held_object(
        gGraphNodePool, nullptr, nullptr,
        offset,
        GetFunctionByAddr(addr),
        player
    );

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

    mLoadedReader = new LUS::BinaryReader(data, size);
    mReader = mLoadedReader;

    while (mReader != nullptr) {
        const auto cmdId = mReader->ReadUByte();
        GeoLayoutFunctionTable[cmdId]();
    }
}

extern "C" void GeoLayoutExecute(char const* path) {
    GeoLayoutParser::execute(path);
}