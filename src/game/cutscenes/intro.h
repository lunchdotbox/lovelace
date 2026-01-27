#ifndef GAME_CUTSCENES_INTRO_H
#define GAME_CUTSCENES_INTRO_H

#include "../../engine/graphics/text.h"
#include "../audio/sounds.h"

typedef enum IntroState {
    INTRO_STATE_DISPLAY_CONTROLS,
    INTRO_STATE_CONTROLS_SLIDE_OUT,
    INTRO_STATE_QUESTION_AUDIO,
    INTRO_STATE_DISPLAY_DANIEL_TEXT,
    INTRO_STATE_DISPLAY_DRIVE_TEXT,
    INTRO_STATE_WAKING_UP_FADE,
} IntroState;

typedef union IntroData {

} IntroData;

typedef struct IntroCutscene {
    IntroData data;
    u64 start_time;
    IntroState state;
} IntroCutscene;

IntroCutscene createIntroCutscene();
void tickIntroCutscene(IntroCutscene* intro, TextFont* font, SoundsGame* sounds, u64 ct);

#endif
