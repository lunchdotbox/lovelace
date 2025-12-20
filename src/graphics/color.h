#ifndef COLOR_H
#define COLOR_H

#include <elc/core.h>

typedef struct Color {
    u8 r, g, b, a;
} Color;

#undef _COLORS_MACRO
#define _COLORS_MACRO\
    X(LIGHTGRAY, "light gray", 200, 200, 200, 255)\
    X(GRAY, "gray", 130, 130, 130, 255)\
    X(DARKGRAY, "dark gray", 40, 40, 40, 255)\
    X(YELLOW, "yellow", 255, 255, 0, 255)\
    X(GOLD, "gold", 255, 203, 0, 255)\
    X(ORANGE, "orange", 255, 161, 0, 255)\
    X(PINK, "pink", 255, 109, 194, 255)\
    X(RED, "red", 255, 0, 0, 255)\
    X(MAROON, "maroon", 190, 33, 55, 255)\
    X(GREEN, "green", 0, 228, 48, 255)\
    X(LIME, "lime", 0, 158, 47, 255)\
    X(DARKGREEN, "dark green", 0, 117, 44, 255)\
    X(SKYBLUE, "sky blue", 102, 191, 255, 255)\
    X(BLUE, "blue", 0, 0, 255, 255)\
    X(DARKBLUE, "dark blue", 0, 82, 172, 255)\
    X(PURPLE, "purple", 200, 122, 255, 255)\
    X(VIOLET, "violet", 135, 60, 190, 255)\
    X(DARKPURPLE, "dark purple", 112, 31, 126, 255)\
    X(BEIGE, "beige", 211, 176, 131, 255)\
    X(BROWN, "brown", 127, 106, 79, 255)\
    X(DARKBROWN, "dark brown", 76, 63, 47, 255)\
    X(WHITE, "white", 255, 255, 255, 255)\
    X(BLACK, "black", 0, 0, 0, 255)\
    X(BLANK, "blank", 0, 0, 0, 0)\
    X(MAGENTA, "magenta", 255, 0, 255, 255)\

#undef X
#define X(id, name, ...) static const Color COLOR_##id = (Color){__VA_ARGS__};
_COLORS_MACRO
#undef X
#define X(id, name, ...) COLOR_ID_##id,
typedef enum ColorID {
    _COLORS_MACRO
} ColorID;
#undef X
#define X(id, name, ...) case COLOR_ID_##id: return COLOR_##id;
ELC_INLINE Color colorFromColorID(ColorID id) {
    switch (id) {
        _COLORS_MACRO
    }
}
#undef X
#define X(id, name, ...) case COLOR_ID_##id: return name;
ELC_INLINE const char* nameFromColorID(ColorID id) {
    switch (id) {
        _COLORS_MACRO
    }
}
#undef X
#undef _COLORS_MACRO

#endif
