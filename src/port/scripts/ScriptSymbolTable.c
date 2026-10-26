// Names scripts can use. Built like data/behavior_data.c so object fields are behavior command indices.
#define OBJECT_FIELDS_INDEX_DIRECTLY
#include "sm64.h"
#include "object_constants.h"
#include "model_ids.h"
#include "seq_ids.h"
#include "dialog_ids.h"
#include "surface_terrains.h"
#include "geo_commands.h"
#include "level_commands.h"
#include "course_table.h"
#include "game/object_list_processor.h"
#include "game/interaction.h"
#include "game/behavior_actions.h"
#include "game/mario_actions_cutscene.h"
#include "game/mario_misc.h"
#include "game/object_helpers.h"
#include "game/debug.h"
#include "game/level_update.h"
#include "game/shadow.h"
#include "menu/file_select.h"
#include "menu/star_select.h"
#include "menu/title_screen.h"
#include "engine/surface_load.h"
#include "levels/scripts.h"
#include "behavior_data.h"

#include "ScriptSymbolTable.h"

#define SCRIPT_BEHAVIOR(name) { #name, (const void*)(name) },
#define SCRIPT_FUNCTION(name) { #name, (const void*)(name) },
#define SCRIPT_LEVEL_SCRIPT(name) { #name, (const void*)(name) },
#define SCRIPT_CONSTANT(name)
const ScriptPointerSymbol gScriptPointerSymbols[] = {
#include "generated/ScriptSymbols.inc"
};
const size_t gScriptPointerSymbolCount = ARRAY_COUNT(gScriptPointerSymbols);
#undef SCRIPT_BEHAVIOR
#undef SCRIPT_FUNCTION
#undef SCRIPT_LEVEL_SCRIPT
#undef SCRIPT_CONSTANT

#define SCRIPT_BEHAVIOR(name)
#define SCRIPT_FUNCTION(name)
#define SCRIPT_LEVEL_SCRIPT(name)
#define SCRIPT_CONSTANT(name) { #name, (int64_t)(name) },
const ScriptConstantSymbol gScriptConstantSymbols[] = {
#include "generated/ScriptSymbols.inc"
};
const size_t gScriptConstantSymbolCount = ARRAY_COUNT(gScriptConstantSymbols);
