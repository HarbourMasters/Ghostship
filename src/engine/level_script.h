#ifndef LEVEL_SCRIPT_H
#define LEVEL_SCRIPT_H

#include <libultra/types.h>

struct LevelCommand;

#ifdef __cplusplus
extern "C" {
#endif

extern u8 level_script_entry[];

struct LevelCommand *level_script_execute(struct LevelCommand *cmd);

#ifdef __cplusplus
};
#endif

#endif // LEVEL_SCRIPT_H
