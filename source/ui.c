#include "ui.h"
#include "theme.h"
#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DYN_BUF_SIZE 8192

static C2D_TextBuf s_buf;

// Glitch state

// ── Primitives ────────────────────────────────────────────────────────────────

static void draw_text(const char *str, float x, float y, float z, float scale, u32 color) {
    C2D_Text t;
    C2D_TextParse(&t, s_buf, str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, scale, scale, color);
}

static void draw_text_centered(const char *str, float cx, float y, float z, float scale, u32 color) {
    C2D_Text t;
    C2D_TextParse(&t, s_buf, str);
    C2D_TextOptimize(&t);
    float w, h;
    C2D_TextGetDimensions(&t, scale, scale, &w, &h);
    C2D_DrawText(&t, C2D_WithColor, cx - w * 0.5f, y, z, scale, scale, color);
}

static void fill_rect(float x, float y, float z, float w, float h, u32 c) {
    C2D_DrawRectSolid(x, y, z, w, h, c);
}

static void outline_rect(float x, float y, float w, float h, float t, u32 c) {
    fill_rect(x,         y,         Z_FG, w, t, c);
    fill_rect(x,         y + h - t, Z_FG, w, t, c);
    fill_rect(x,         y,         Z_FG, t, h, c);
    fill_rect(x + w - t, y,         Z_FG, t, h, c);
}

// Multi-layer glow border
static void neon_border(float x, float y, float w, float h,
                        u32 g1, u32 g2, u32 g3, u32 solid) {
    outline_rect(x - 3, y - 3, w + 6, h + 6, 1, g1);
    outline_rect(x - 2, y - 2, w + 4, h + 4, 1, g2);
    outline_rect(x - 1, y - 1, w + 2, h + 2, 1, g3);
    outline_rect(x,     y,     w,     h,     1, solid);
}

static void draw_grid(float sw, float sh) {
    for (float gx = 0; gx < sw; gx += 20.0f)
        fill_rect(gx, 0, Z_BG, 1, sh, THEME_GRID);
    for (float gy = 0; gy < sh; gy += 20.0f)
        fill_rect(0, gy, Z_BG, sw, 1, THEME_GRID);
}

static void draw_scanlines(float sw, float sh) {
    for (float sy = 1; sy < sh; sy += 2.0f)
        fill_rect(0, sy, Z_MID - 0.01f, sw, 1, THEME_SCANLINE);
}

// Small L-shaped corner ornament
static void corner(float x, float y, float arm, u32 c, bool fx, bool fy) {
    float dx = fx ? -1.0f : 1.0f;
    float dy = fy ? -1.0f : 1.0f;
    fill_rect(x, y,             Z_FG, dx * arm, 2, c);
    fill_rect(x, y,             Z_FG, 2, dy * arm, c);
    fill_rect(x - 1, y - 1,    Z_FG, 4, 4, c);
}

// Horizontal separator line with tick marks
static void separator(float y, float sw, u32 c) {
    fill_rect(0, y, Z_FG, sw, 1, c);
    for (float tx = 10; tx < sw; tx += 40.0f)
        fill_rect(tx, y - 2, Z_FG, 2, 5, c);
}

// ── Glitch logic ──────────────────────────────────────────────────────────────

// Returns true during brief glitch windows
static bool glitch_active(u32 frame) {
    return (frame % 120 < 4) || (frame % 73 < 3);
}

// ── Top screen ────────────────────────────────────────────────────────────────

void ui_draw_top(u32 frame, int selected, int scroll) {
    char tmp[256];
    int  count   = scanner_get_count();
    bool glitch  = glitch_active(frame);
    int  goff    = glitch ? (rand() % 7 - 3) : 0;

    C2D_TextBufClear(s_buf);

    // Background
    fill_rect(0, 0, Z_BG, TOP_W, TOP_H, THEME_BG);
    draw_grid(TOP_W, TOP_H);

    // ── Header bar ───────────────────────────────────────────────────────────
    fill_rect(0, 0, Z_MID, TOP_W, 26, THEME_PANEL);
    separator(26, TOP_W, THEME_CYAN_G3);

    // Animated bracket blink
    u32 hdr_color = (frame % 60 < 30) ? THEME_CYAN : THEME_CYAN_DIM;
    draw_text("[ CYBER HUB ]", 6 + goff, 5, Z_FG, 0.55f, hdr_color);

    // Status indicator (right side)
    u32 status_c = count > 0 ? THEME_GREEN : THEME_RED;
    const char *status_s = count > 0 ? "ONLINE" : "NO SIG";
    snprintf(tmp, sizeof(tmp), "// %s //", status_s);
    draw_text(tmp, TOP_W - 80, 5, Z_FG, 0.45f, status_c);

    // Frame counter (subtle, top-right)
    snprintf(tmp, sizeof(tmp), "%06u", frame % 999999);
    draw_text(tmp, TOP_W - 52, 15, Z_FG, 0.35f, THEME_DARK_GRAY);

    // ── Entry list ───────────────────────────────────────────────────────────
    const float LIST_Y = 32.0f;
    const float ENTRY_H = 27.0f;

    if (count == 0) {
        draw_text_centered("-- NO SIGNAL --", TOP_W / 2, 100, Z_FG, 0.60f, THEME_GRAY);
        draw_text_centered("PLACE .3DSX FILES IN /3DS/", TOP_W / 2, 120, Z_FG, 0.38f, THEME_GRAY);
        draw_text_centered("PLACE ROM FILES IN /ROMS/", TOP_W / 2, 134, Z_FG, 0.38f, THEME_GRAY);
        draw_text_centered("INSTALL CIA GAMES VIA FBI", TOP_W / 2, 148, Z_FG, 0.38f, THEME_GRAY);
    } else {
        for (int i = 0; i < ENTRIES_PER_PAGE; i++) {
            int idx = scroll + i;
            if (idx >= count) break;

            ScanEntry *e = scanner_get_entry(idx);
            float ey = LIST_Y + i * ENTRY_H;
            bool  sel = (idx == selected);

            if (sel) {
                // Selection background
                fill_rect(4, ey, Z_MID, TOP_W - 8, ENTRY_H - 2, THEME_SEL_BG);
                // Neon selection border
                neon_border(4, ey, TOP_W - 8, ENTRY_H - 2,
                            THEME_CYAN_G1, THEME_CYAN_G2, THEME_CYAN_G3, THEME_CYAN);
                // Arrow indicator with glitch offset
                u32 arr_c = glitch ? THEME_MAGENTA : THEME_CYAN;
                draw_text(">", 8 + goff, ey + 5, Z_FG, 0.55f, arr_c);
            }

            // Entry name
            u32 name_c = sel ? THEME_WHITE : THEME_CYAN_DIM;
            if (sel && glitch) name_c = THEME_RED;
            draw_text(e->name, 22 + (sel ? goff : 0), ey + 5, Z_FG, 0.50f, name_c);

            // Type badge
            char badge[10];
            u32  badge_c;
            if (e->type == ENTRY_ROM) {
                snprintf(badge, sizeof(badge), "[%s]", e->system);
                badge_c = e->emu_missing ? THEME_RED : THEME_YELLOW;
            } else if (e->type == ENTRY_EMULATOR) {
                snprintf(badge, sizeof(badge), "[EMU]");
                badge_c = THEME_CYAN_DIM;
            } else if (e->type == ENTRY_HOMEBREW) {
                snprintf(badge, sizeof(badge), "[HB]");
                badge_c = THEME_MAGENTA_DIM;
            } else {
                snprintf(badge, sizeof(badge), "[3DS]");
                badge_c = THEME_GREEN_DIM;
            }
            draw_text(badge, TOP_W - 46, ey + 5, Z_FG, 0.40f, badge_c);

            // Subtle divider between entries
            if (!sel)
                fill_rect(22, ey + ENTRY_H - 2, Z_FG, TOP_W - 44, 1, THEME_DARK_GRAY);
        }
    }

    // ── Footer bar ───────────────────────────────────────────────────────────
    float fy = TOP_H - 20;
    separator(fy, TOP_W, THEME_MAGENTA_G2);
    fill_rect(0, fy, Z_MID, TOP_W, 20, THEME_PANEL);

    snprintf(tmp, sizeof(tmp), "ENTRIES: %d", count);
    draw_text(tmp, 6, fy + 3, Z_FG, 0.40f, THEME_MAGENTA);

    if (count > 0) {
        int page_cur = scroll / ENTRIES_PER_PAGE + 1;
        int page_tot = (count + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;
        snprintf(tmp, sizeof(tmp), "PAGE %d/%d", page_cur, page_tot);
        draw_text(tmp, TOP_W / 2 - 30, fy + 3, Z_FG, 0.40f, THEME_GRAY);
    }

    snprintf(tmp, sizeof(tmp), "PSY: %d%%", 12 + (frame / 60) % 88);
    u32 psy_c = THEME_GREEN;
    int psy_val = 12 + (frame / 60) % 88;
    if (psy_val > 60) psy_c = THEME_YELLOW;
    if (psy_val > 80) psy_c = THEME_RED;
    draw_text(tmp, TOP_W - 70, fy + 3, Z_FG, 0.40f, psy_c);

    // ── Corner ornaments ─────────────────────────────────────────────────────
    u32 corn_c = glitch ? THEME_RED : THEME_CYAN_G3;
    corner(3,       28, 8, corn_c, false, false);
    corner(TOP_W-3, 28, 8, corn_c, true,  false);
    corner(3,       fy-1, 8, corn_c, false, true);
    corner(TOP_W-3, fy-1, 8, corn_c, true,  true);

    // Scanlines — always on top
    draw_scanlines(TOP_W, TOP_H);
}

// ── Bottom screen ─────────────────────────────────────────────────────────────

void ui_draw_bottom(u32 frame, int selected) {
    char tmp[256];
    int  count  = scanner_get_count();
    bool glitch = glitch_active(frame);

    C2D_TextBufClear(s_buf);

    fill_rect(0, 0, Z_BG, BOT_W, BOT_H, THEME_BG);
    draw_grid(BOT_W, BOT_H);

    // ── Logo panel ───────────────────────────────────────────────────────────
    fill_rect(0, 0, Z_MID, BOT_W, 42, THEME_PANEL);
    separator(42, BOT_W, THEME_MAGENTA_G2);

    // Pulsing logo color
    float pulse = (frame % 60) / 60.0f;
    u32 lo_r = (u32)(0x00 + pulse * 0x00);
    u32 lo_g = (u32)(0xCC + pulse * 0x33);
    u32 lo_b = (u32)(0xCC + pulse * 0x33);
    u32 logo_c = C2D_Color32(lo_r, lo_g, lo_b, 0xFF);
    if (glitch) logo_c = THEME_MAGENTA;

    int gx = glitch ? (rand() % 5 - 2) : 0;
    draw_text_centered("C Y B E R H U B", BOT_W / 2 + gx, 6, Z_FG, 0.65f, logo_c);
    draw_text_centered("v1.0 // NEURAL LINK ACTIVE", BOT_W / 2, 26, Z_FG, 0.38f, THEME_CYAN_DIM);

    // ── Selected game info ───────────────────────────────────────────────────
    float info_y = 50;
    fill_rect(6, info_y, Z_MID, BOT_W - 12, 78, THEME_PANEL);
    neon_border(6, info_y, BOT_W - 12, 78,
                THEME_CYAN_G1, THEME_CYAN_G2, THEME_CYAN_G3, THEME_CYAN_DIM);

    draw_text("> SELECTED:", 14, info_y + 4, Z_FG, 0.40f, THEME_CYAN_DIM);

    if (count > 0) {
        ScanEntry *e = scanner_get_entry(selected);
        if (e) {
            // Truncate long names for display
            char display_name[48];
            strncpy(display_name, e->name, 47);
            display_name[47] = '\0';

            u32 sel_c = glitch ? THEME_RED : THEME_WHITE;
            draw_text(display_name, 14, info_y + 18, Z_FG, 0.52f, sel_c);

            const char *type_str;
            u32 type_c;
            if (e->type == ENTRY_ROM) {
                type_str = e->emu_missing ? "ROM [NO EMULATOR FOUND]" : "ROM [AUTO-LAUNCH]";
                type_c   = e->emu_missing ? THEME_RED : THEME_YELLOW;
            } else if (e->type == ENTRY_EMULATOR) {
                type_str = "EMULATOR [.3DSX]";
                type_c   = THEME_CYAN;
            } else if (e->type == ENTRY_HOMEBREW) {
                type_str = "HOMEBREW GAME [.3DSX]";
                type_c   = THEME_MAGENTA;
            } else {
                type_str = "INSTALLED [3DS]";
                type_c   = THEME_GREEN;
            }
            snprintf(tmp, sizeof(tmp), "TYPE: %s", type_str);
            draw_text(tmp, 14, info_y + 36, Z_FG, 0.40f, type_c);

            // Truncate path display
            char path_disp[44];
            if (strlen(e->path) > 43) {
                strncpy(path_disp, "...", 3);
                strncpy(path_disp + 3, e->path + strlen(e->path) - 40, 40);
                path_disp[43] = '\0';
            } else {
                strncpy(path_disp, e->path, 43);
                path_disp[43] = '\0';
            }
            draw_text(path_disp, 14, info_y + 52, Z_FG, 0.35f, THEME_GRAY);
        }
    } else {
        draw_text("-- NO PROGRAMS FOUND --", 14, info_y + 24, Z_FG, 0.45f, THEME_GRAY);
        draw_text("Place .3dsx files in sdmc:/3ds/", 14, info_y + 44, Z_FG, 0.38f, THEME_GRAY);
    }

    // ── Controls bar ─────────────────────────────────────────────────────────
    float ctrl_y = 138;
    separator(ctrl_y, BOT_W, THEME_CYAN_G3);
    fill_rect(0, ctrl_y, Z_MID, BOT_W, BOT_H - ctrl_y, THEME_PANEL);

    // Control hints
    struct { const char *btn; const char *desc; float x; u32 btn_c; } ctrls[] = {
        { "[A]",     "LAUNCH",    8,   THEME_GREEN    },
        { "[R]",     "REFRESH",  68,   THEME_CYAN     },
        { "[U/D]",   "NAVIGATE", 128,  THEME_YELLOW   },
        { "[START]", "EXIT",     195,  THEME_RED      },
    };

    for (int i = 0; i < 4; i++) {
        draw_text(ctrls[i].btn,  ctrls[i].x, ctrl_y + 6,  Z_FG, 0.40f, ctrls[i].btn_c);
        draw_text(ctrls[i].desc, ctrls[i].x, ctrl_y + 18, Z_FG, 0.36f, THEME_GRAY);
    }

    // ── Corner ornaments ─────────────────────────────────────────────────────
    u32 corn_c = glitch ? THEME_RED : THEME_MAGENTA_DIM;
    corner(3,       44, 6, corn_c, false, false);
    corner(BOT_W-3, 44, 6, corn_c, true,  false);

    // Bottom data ticker (scrolling hex string for aesthetics)
    char hex_str[64];
    snprintf(hex_str, sizeof(hex_str), "%08X %08X %08X %08X",
             frame * 0xDEAD, frame * 0xBEEF, frame ^ 0xCAFE, ~frame & 0xFFFFFFFF);
    int ticker_off = (int)(frame * 2) % (int)(BOT_W + 280);
    draw_text(hex_str, BOT_W - ticker_off, BOT_H - 12, Z_FG, 0.32f, THEME_DARK_GRAY);

    draw_scanlines(BOT_W, BOT_H);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void ui_init(void) {
    s_buf = C2D_TextBufNew(DYN_BUF_SIZE);
    srand(osGetTime() & 0xFFFFFFFF);
}

void ui_cleanup(void) {
    C2D_TextBufDelete(s_buf);
}
