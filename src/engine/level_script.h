#ifndef LEVEL_SCRIPT_H
#define LEVEL_SCRIPT_H

#include <libultra/types.h>

struct LevelCommand;

extern_s u8 level_script_entry[];
extern_s struct AllocOnlyPool *sLevelPool;

extern_s struct LevelCommand *level_script_execute(struct LevelCommand *cmd);

#endif // LEVEL_SCRIPT_H
