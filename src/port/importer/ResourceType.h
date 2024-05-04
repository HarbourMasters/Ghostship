#pragma once

namespace SM64 {
enum class ResourceType {
    // SM64
    Bank = 0x42414E4B,           // BANK
    Sample = 0x41554643,         // AIFC
    Sequence = 0x53455143,       // SEQC
    Anim = 0x414E494D,           // ANIM
    BehaviorScript = 0x42485653, // BHVS
    SDialog = 0x53444C47,        // SDLG
    Dictionary = 0x44494354,     // DICT
    GeoLayout = 0x47454F20,      // GEO
    Collision = 0x434F4C20,      // COL
    LevelScript = 0x4C564C53,    // LVLS
    MacroObject = 0x4D41434F,    // MACO
    Movtex = 0x4D4F5654,         // MOVT
    MovtexQuad = 0x4D4F5651,     // MOVQ
    Painting = 0x504E5420,       // PNT
    PaintingData = 0x504E5444,   // PNTD
    Trajectory = 0x5452414A,     // TRAJ
    WaterDroplet = 0x57545244,   // WTRD
};
} // namespace SOH
