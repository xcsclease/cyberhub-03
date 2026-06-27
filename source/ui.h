#pragma once
#include <citro2d.h>

void ui_init(void);
void ui_draw_top(u32 frame, int selected, int scroll);
void ui_draw_bottom(u32 frame, int selected);
void ui_cleanup(void);
