#include <elc/core.h>
#include <stddef.h>
#include <string.h>
#include "miniaudio.h"

#ifndef _SOUNDS_MACRO
#define _SOUNDS_MACRO
#endif

#ifndef _SOUNDS_NAMING_MACRO
#define _SOUNDS_NAMING_MACRO(base) base
#endif

#undef X
#define X(sound_name) ma_sound sound_name;
typedef struct _SOUNDS_NAMING_MACRO(Sounds) {
    ma_engine engine;
    _SOUNDS_MACRO
} _SOUNDS_NAMING_MACRO(Sounds);
#undef X
#define X(sound_name)\
    {size_t n_sound_name = strlen(#sound_name);\
    char path[n_base_path + n_sound_name + n_file_extension + 1];\
    memcpy(path, base_path, n_base_path * sizeof(char));\
    memcpy(&path[n_base_path], #sound_name, n_sound_name * sizeof(char));\
    memcpy(&path[n_base_path + n_sound_name], file_extension, n_file_extension * sizeof(char));\
    path[ARRAY_LENGTH(path) - 1] = '\0';\
    if (ma_sound_init_from_file(&sounds->engine, path, 0, NULL, NULL, &sounds->sound_name) != MA_SUCCESS) printf("failed to load sound %s from file %s\n", #sound_name, path);}
ELC_INLINE void _SOUNDS_NAMING_MACRO(loadSounds)(_SOUNDS_NAMING_MACRO(Sounds)* sounds, const char* base_path) {
    const char* file_extension = ".wav";
    size_t n_file_extension = strlen(file_extension);
    if (ma_engine_init(NULL, &sounds->engine) != MA_SUCCESS) puts("failed to initialize audio engine");
    size_t n_base_path = strlen(base_path);
    _SOUNDS_MACRO
}
#undef X
#define X(sound_name) ma_sound_uninit(&sounds->sound_name);
ELC_INLINE void _SOUNDS_NAMING_MACRO(unloadSounds)(_SOUNDS_NAMING_MACRO(Sounds)* sounds) {
    _SOUNDS_MACRO
    ma_engine_uninit(&sounds->engine);
}
#undef X

#undef _SOUNDS_NAMING_MACRO
#undef _SOUNDS_MACRO

#ifndef ENGINE_AUDIO_SOUNDS_H
#define ENGINE_AUDIO_SOUNDS_H

ELC_INLINE float getSoundLength(const ma_sound* sound) {
    float length;
    ma_sound_get_length_in_seconds(sound, &length);
    return length;
}

ELC_INLINE float getSoundCursor(const ma_sound* sound) {
    float cursor;
    ma_sound_get_cursor_in_seconds(sound, &cursor);
    return cursor;
}

#endif
