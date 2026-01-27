#include "intro.h"

#include <cglm/types.h>
#include <elc/core.h>
#include <stdint.h>
#include "../../engine/math/transition.h"

IntroCutscene createIntroCutscene() {
    return (IntroCutscene){.start_time = milliseconds()};
}

void tickIntroCutscene(IntroCutscene* intro, TextFont* font, SoundsGame* sounds, u64 ct) {
    switch (intro->state) {
        case INTRO_STATE_DISPLAY_CONTROLS:
            if (ct - intro->start_time >= stoms(1.0)) intro->state++, intro->start_time = ct;
            addTextRectangle(font, COLOR_BLACK, (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            addTextCentered(font, (vec2){0}, (vec2){0.1f, 0.1f}, COLOR_WHITE, "press W to accelerate\npress S to brake\npress A or D to turn");
            break;
        case INTRO_STATE_CONTROLS_SLIDE_OUT:
            if (ct - intro->start_time >= stoms(0.75)) {
                intro->state++, intro->start_time = ct;
                ma_sound_start(&sounds->daniel_intro_temp);
            }
            addTextRectangle(font, COLOR_BLACK, (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            addTextCentered(font, (vec2){0.0f, easeInQuad((float)(ct - intro->start_time) / stoms(0.75)) * 2.0f}, (vec2){0.1f, 0.1f}, COLOR_WHITE, "press W to accelerate\npress S to brake\npress A or D to turn");
            break;
        case INTRO_STATE_QUESTION_AUDIO:
            if (getSoundCursor(&sounds->daniel_intro_temp) >= 1.203) intro->state++, intro->start_time = ct;
            addTextRectangle(font, COLOR_BLACK, (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            break;
        case INTRO_STATE_DISPLAY_DANIEL_TEXT:
            if (getSoundCursor(&sounds->daniel_intro_temp) >= 3.698) intro->state++, intro->start_time = ct;
            addTextRectangle(font, COLOR_BLACK, (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            addTextCentered(font, (vec2){-1.0f + easeOutElastic(((float)(ct - intro->start_time) / stoms(1.0))), -0.25f}, (vec2){0.4f, 0.4f}, COLOR_BURLYWOOD, "daniel");
            break;
        case INTRO_STATE_DISPLAY_DRIVE_TEXT:
            if (getSoundCursor(&sounds->daniel_intro_temp) >= 4.792) intro->state++, intro->start_time = ct;
            addTextRectangle(font, COLOR_BLACK, (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            addTextCentered(font, (vec2){0.0f, -0.25f}, (vec2){0.4f, 0.4f}, COLOR_BURLYWOOD, "daniel");
            addTextCentered(font, (vec2){1.0f - easeOutElastic(((float)(ct - intro->start_time) / stoms(1.0))), 0.25f}, (vec2){0.4f, 0.4f}, COLOR_RED, "drive");
            break;
        case INTRO_STATE_WAKING_UP_FADE:
            if (getSoundCursor(&sounds->daniel_intro_temp) >= 11.935) intro->state++, intro->start_time = ct;
            float alpha = CLAMP((double)(ct - intro->start_time) / stoms(7.143), 0.0, 1.0);
            addTextRectangle(font, changeAlpha(COLOR_BLACK, (1.0 - alpha) * UINT8_MAX), (vec2){-2.0f, -2.0f}, (vec2){4.0f, 4.0f});
            addTextCentered(font, (vec2){0.0f, -0.25f}, (vec2){0.4f, 0.4f}, changeAlpha(COLOR_BURLYWOOD, MAX(1.0 - (alpha * 3.0), 0.0) * UINT8_MAX), "daniel");
            addTextCentered(font, (vec2){0.0f, 0.25f}, (vec2){0.4f, 0.4f}, changeAlpha(COLOR_RED, MAX(1.0 - (alpha * 3.0), 0.0) * UINT8_MAX), "drive");
            break;
        default: break;
    }
}
