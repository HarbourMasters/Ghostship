#ifndef RANDO_TYPES_H
#define RANDO_TYPES_H

typedef enum {
    RCTYPE_UNKNOWN,
    RCTYPE_MAX,
} RandoCheckType;

typedef enum {
    RC_UNKNOWN,
    RC_MAX,
} RandoCheckId;

typedef enum {
    RITYPE_MAX,
} RandoItemType;

typedef enum {
    RI_UNKNOWN,
    RI_MAX,
} RandoItemId;

typedef enum {
    RR_MAX,
} RandoRegionId;

typedef enum {
    RO_MAX,
} RandoOptionId;

typedef enum {
    RO_GENERIC_OFF,
    RO_GENERIC_ON,
} RandoOptionGenericOffOn;

typedef enum {
    RO_LOGIC_GLITCHLESS,
    RO_LOGIC_NO_LOGIC,
} RandoOptionLogic;

typedef enum {
    RANDO_INF_MAX,
} RandoInf;

typedef enum {
    RE_MAX,
} RandoEvent;

#endif // RANDO_TYPES_H
