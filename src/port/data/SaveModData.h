#pragma once

#include <stdint.h>

// Values mods keep in a save file (port/api/savedata.h), stored under "mods" in save_<n>.json. Each file has the
// values on disk and the live ones mods change while it is played; the game's own save calls move them between the
// two, so mod values are kept, erased, copied and reverted exactly when the game's own data is.

#ifdef __cplusplus
#include <nlohmann/json.hpp>

namespace SaveModData {

// Takes the "mods" object of a file read from disk, or an empty one for a file that isn't there.
void Load(int32_t fileIndex, const nlohmann::json& mods);
// The live values of the file, now also what is on disk.
const nlohmann::json& Commit(int32_t fileIndex);
// The live values of the file being played; mods read and write these.
nlohmann::json& Current();

} // namespace SaveModData

extern "C" {
#endif

// The calls the game makes alongside its own save data. fileIndex is 0-based.
void SaveModData_Erase(int32_t fileIndex);
void SaveModData_Copy(int32_t srcFileIndex, int32_t destFileIndex);
// Back to what is on disk, as after a game over.
void SaveModData_Revert(int32_t fileIndex);

#ifdef __cplusplus
}
#endif
