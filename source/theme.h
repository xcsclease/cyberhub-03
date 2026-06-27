#pragma once
#include <citro2d.h>

// Screen dimensions
#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240

// Z-depth layers
#define Z_BG  0.9f
#define Z_MID 0.5f
#define Z_FG  0.1f

// Entries visible on top screen
#define ENTRIES_PER_PAGE 7

// ── Cyberpunk color palette ───────────────────────────────────────────────────
#define THEME_BG           C2D_Color32(0x06, 0x06, 0x12, 0xFF)  // deep void
#define THEME_PANEL        C2D_Color32(0x04, 0x0E, 0x20, 0xE8)  // panel bg
#define THEME_GRID         C2D_Color32(0x00, 0x38, 0x58, 0x24)  // subtle grid
#define THEME_SCANLINE     C2D_Color32(0x00, 0x00, 0x00, 0x3A)  // scanline stripe

#define THEME_CYAN         C2D_Color32(0x00, 0xFF, 0xFF, 0xFF)
#define THEME_CYAN_DIM     C2D_Color32(0x00, 0x99, 0xAA, 0xFF)
#define THEME_CYAN_G1      C2D_Color32(0x00, 0xFF, 0xFF, 0x16)  // glow layer 1
#define THEME_CYAN_G2      C2D_Color32(0x00, 0xFF, 0xFF, 0x38)  // glow layer 2
#define THEME_CYAN_G3      C2D_Color32(0x00, 0xFF, 0xFF, 0x66)  // glow layer 3

#define THEME_MAGENTA      C2D_Color32(0xFF, 0x00, 0x88, 0xFF)
#define THEME_MAGENTA_DIM  C2D_Color32(0xAA, 0x00, 0x55, 0xFF)
#define THEME_MAGENTA_G1   C2D_Color32(0xFF, 0x00, 0x88, 0x18)
#define THEME_MAGENTA_G2   C2D_Color32(0xFF, 0x00, 0x88, 0x44)

#define THEME_RED          C2D_Color32(0xFF, 0x08, 0x08, 0xFF)
#define THEME_RED_G1       C2D_Color32(0xFF, 0x00, 0x00, 0x20)
#define THEME_RED_G2       C2D_Color32(0xFF, 0x00, 0x00, 0x55)

#define THEME_GREEN        C2D_Color32(0x08, 0xFF, 0x44, 0xFF)
#define THEME_GREEN_DIM    C2D_Color32(0x04, 0x99, 0x28, 0xFF)

#define THEME_YELLOW       C2D_Color32(0xFF, 0xCC, 0x00, 0xFF)
#define THEME_WHITE        C2D_Color32(0xE0, 0xE0, 0xFF, 0xFF)
#define THEME_GRAY         C2D_Color32(0x55, 0x55, 0x77, 0xFF)
#define THEME_DARK_GRAY    C2D_Color32(0x22, 0x22, 0x33, 0xFF)

#define THEME_SEL_BG       C2D_Color32(0x00, 0x28, 0x48, 0xCC)

