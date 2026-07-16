#pragma once
#include <libultraship/libultraship.h>

typedef enum {
  SAVE_FLAGS,
  MARIO_FLAGS,
} FlagTableType;

typedef struct {
    const char* name;
    FlagTableType flagTableType;
    uint32_t size;
    std::map<uint16_t, const char*> flagDescriptions;
} FlagTable;

const std::vector<FlagTable> flagTables = {
    { "Save Flags", SAVE_FLAGS, 0x04, {
        { 0x00, "SAVE_FLAG_FILE_EXISTS" },
        { 0x01, "SAVE_FLAG_HAVE_WING_CAP" },
        { 0x02, "SAVE_FLAG_HAVE_METAL_CAP" },
        { 0x03, "SAVE_FLAG_HAVE_VANISH_CAP" },
        { 0x04, "SAVE_FLAG_HAVE_KEY_1" },
        { 0x05, "SAVE_FLAG_HAVE_KEY_2" },
        { 0x06, "SAVE_FLAG_UNLOCKED_BASEMENT_DOOR" },
        { 0x07, "SAVE_FLAG_UNLOCKED_UPSTAIRS_DOOR" },
        { 0x08, "SAVE_FLAG_DDD_MOVED_BACK" },
        { 0x09, "SAVE_FLAG_MOAT_DRAINED" },
        { 0x0A, "SAVE_FLAG_UNLOCKED_PSS_DOOR" },
        { 0x0B, "SAVE_FLAG_UNLOCKED_WF_DOOR" },
        { 0x0C, "SAVE_FLAG_UNLOCKED_CCM_DOOR" },
        { 0x0D, "SAVE_FLAG_UNLOCKED_JRB_DOOR" },
        { 0x0E, "SAVE_FLAG_UNLOCKED_BITDW_DOOR" },
        { 0x0F, "SAVE_FLAG_UNLOCKED_BITFS_DOOR" },
        { 0x10, "SAVE_FLAG_CAP_ON_GROUND" },
        { 0x11, "SAVE_FLAG_CAP_ON_KLEPTO" },
        { 0x12, "SAVE_FLAG_CAP_ON_UKIKI" },
        { 0x13, "SAVE_FLAG_CAP_ON_MR_BLIZZARD" },
        { 0x14, "SAVE_FLAG_UNLOCKED_50_STAR_DOOR" },
        // { 0x15, "" },
        // { 0x16, "" },
        // { 0x17, "" },
        { 0x18, "SAVE_FLAG_COLLECTED_TOAD_STAR_1" },
        { 0x19, "SAVE_FLAG_COLLECTED_TOAD_STAR_2" },
        { 0x1A, "SAVE_FLAG_COLLECTED_TOAD_STAR_3" },
        { 0x1B, "SAVE_FLAG_COLLECTED_MIPS_STAR_1" },
        { 0x1C, "SAVE_FLAG_COLLECTED_MIPS_STAR_2" },
        // { 0x1D, "" },
        // { 0x1E, "" },
        // { 0x1F, "" },
    } },
    { "Mario Flags", MARIO_FLAGS, 0x04, {
        { 0x00, "MARIO_NORMAL_CAP" },
        { 0x01, "MARIO_VANISH_CAP" },
        { 0x02, "MARIO_METAL_CAP" },
        { 0x03, "MARIO_WING_CAP" },
        { 0x04, "MARIO_CAP_ON_HEAD" },
        { 0x05, "MARIO_CAP_IN_HAND" },
        { 0x06, "MARIO_METAL_SHOCK" },
        { 0x07, "MARIO_TELEPORTING" },
        { 0x08, "MARIO_UNKNOWN_08" },
        // { 0x09, "" },
        // { 0x0A, "" },
        // { 0x0B, "" },
        // { 0x0C, "" },
        { 0x0D, "MARIO_UNKNOWN_13" },
        // { 0x0E, "" },
        // { 0x0F, "" },
        { 0x10, "MARIO_ACTION_SOUND_PLAYED" },
        { 0x11, "MARIO_MARIO_SOUND_PLAYED" },
        { 0x12, "MARIO_UNKNOWN_18" },
        // { 0x13, "" },
        { 0x14, "MARIO_PUNCHING" },
        { 0x15, "MARIO_KICKING" },
        { 0x16, "MARIO_TRIPPING" },
        // { 0x17, "" },
        // { 0x18, "" },
        { 0x19, "MARIO_UNKNOWN_25" },
        // { 0x1A, "" },
        // { 0x1B, "" },
        // { 0x1C, "" },
        // { 0x1D, "" },
        { 0x1E, "MARIO_UNKNOWN_30" },
        { 0x1F, "MARIO_UNKNOWN_31" },
    } },
};

class SaveEditorWindow : public Ship::GuiWindow {
  public:
    using Ship::GuiWindow::GuiWindow;

    void DrawElement() override;
    void UpdateElement() override {};
};