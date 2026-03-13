#ifndef GAME_ENEMIES_SPIDER_H
#define GAME_ENEMIES_SPIDER_H

#include "../../engine/entities/entities.h"

typedef struct WormSegmentComponent {
    Entity next, last;
} WormSegmentComponent;

typedef struct WormTailComponent {
    Entity next;
} WormTailComponent;

typedef struct WormHeadComponent {
    Entity last;
} WormHeadComponent;

#define _SYSTEM_NAMING_MACRO(base) base##Worm
#define _SYSTEM_COMPONENTS_MACRO\
    X(COMPONENT_WORM_SEGMENT, WormSegmentComponent)\
    X(COMPONENT_WORM_TAIL, WormTailComponent)\
    X(COMPONENT_WORM_HEAD, WormHeadComponent)

#include "../../engine/entities/system.h"

#endif
