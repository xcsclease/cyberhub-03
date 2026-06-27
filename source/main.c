#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <stdio.h>

#include "theme.h"
#include "scanner.h"
#include "ui.h"

// Boot splash: shown for ~90 frames before entering main loop
#define SPLASH_FRAMES 90

static C3D_RenderTarget *s_top;
static C3D_RenderTarget *s_bot;

static void draw_splash(u32 frame) {
    C2D_TextBuf tb = C2D_TextBufNew(512);
    C2D_Text    t;

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(s_top, THEME_BG);
    C2D_TargetClear(s_bot, THEME_BG);

    C2D_SceneBegin(s_top);

    // Progress bar fill
    float pct = (float)frame / SPLASH_FRAMES;
    float bar_w = 300.0f * pct;

    // Draw boot text lines (appear progressively)
    struct { u32 at; const char *msg; u32 color; float y; } lines[] = {
        {  0, "CYBERHUB SYSTEMS BOOT SEQUENCE v1.0",  0x00FFFFFF, 60  },
        { 10, "INITIALIZING NEURAL LINK...",           0x00FF88FF, 80  },
        { 20, "SCANNING CYBERWARE MATRIX...",          0x00FF88FF, 96  },
        { 35, "LOADING GAME DATABASE...",              0x00FF88FF, 112 },
        { 50, "CALIBRATING PSY MONITOR...",            0x00FF88FF, 128 },
        { 65, "ENGAGING PSYCHO CONTAINMENT...",        0xFF0A0AFF, 144 },
        { 80, "ACCESS GRANTED. STAY FROSTY, NETRUNNER.", 0x08FF44FF, 164 },
    };

    for (int i = 0; i < 7; i++) {
        if (frame >= lines[i].at) {
            C2D_TextParse(&t, tb, lines[i].msg);
            C2D_TextOptimize(&t);
            C2D_DrawText(&t, C2D_WithColor,
                         (TOP_W - 300) / 2.0f, lines[i].y, Z_FG,
                         0.42f, 0.42f, lines[i].color);
        }
    }

    // Progress bar
    C2D_DrawRectSolid(50, 200, Z_FG, 300, 6, C2D_Color32(0x00, 0x22, 0x33, 0xFF));
    if (bar_w > 0)
        C2D_DrawRectSolid(50, 200, Z_FG, bar_w, 6, C2D_Color32(0x00, 0xFF, 0xFF, 0xFF));
    C2D_DrawRectSolid(50 - 1, 199, Z_FG, 302, 8,
                      C2D_Color32(0, 0, 0, 0)); // clear inner (not needed, just border)
    // Border around bar
    C2D_DrawRectSolid(49,  199, Z_FG, 302, 1, C2D_Color32(0, 0xAA, 0xAA, 0x80));
    C2D_DrawRectSolid(49,  207, Z_FG, 302, 1, C2D_Color32(0, 0xAA, 0xAA, 0x80));
    C2D_DrawRectSolid(49,  199, Z_FG, 1, 8,   C2D_Color32(0, 0xAA, 0xAA, 0x80));
    C2D_DrawRectSolid(351, 199, Z_FG, 1, 8,   C2D_Color32(0, 0xAA, 0xAA, 0x80));

    // Percent text
    char pct_str[16];
    snprintf(pct_str, sizeof(pct_str), "%d%%", (int)(pct * 100));
    C2D_TextParse(&t, tb, pct_str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, 358, 198, Z_FG, 0.45f, 0.45f, C2D_Color32(0, 0xFF, 0xFF, 0xFF));

    C2D_SceneBegin(s_bot);
    // Bottom screen during splash: just show name
    C2D_TextParse(&t, tb, "C Y B E R H U B");
    C2D_TextOptimize(&t);
    float tw, th;
    C2D_TextGetDimensions(&t, 0.70f, 0.70f, &tw, &th);
    C2D_DrawText(&t, C2D_WithColor, (BOT_W - tw) / 2.0f, 100, Z_FG, 0.70f, 0.70f,
                 C2D_Color32(0x00, 0xFF, 0xFF, 0xFF));

    C2D_TextParse(&t, tb, "FOR THE WIRED GENERATION");
    C2D_TextOptimize(&t);
    C2D_TextGetDimensions(&t, 0.38f, 0.38f, &tw, &th);
    C2D_DrawText(&t, C2D_WithColor, (BOT_W - tw) / 2.0f, 130, Z_FG, 0.38f, 0.38f,
                 C2D_Color32(0xFF, 0x00, 0x88, 0xFF));

    C3D_FrameEnd(0);
    C2D_TextBufDelete(tb);
}

int main(int argc, char *argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    cfguInit();

    s_top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    s_bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Boot splash
    for (u32 sf = 0; sf < SPLASH_FRAMES && aptMainLoop(); sf++) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            sf = SPLASH_FRAMES;  // skip splash
        }
        draw_splash(sf);
    }

    // Initialize subsystems after splash
    scanner_init();
    scanner_scan();
    ui_init();

    int selected      = 0;
    int scroll_offset = 0;
    u32 frame         = 0;
    bool quit         = false;

    while (aptMainLoop() && !quit) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        int count = scanner_get_count();

        if (kDown & KEY_START) break;

        if (kDown & KEY_DOWN && selected < count - 1) {
            selected++;
            if (selected >= scroll_offset + ENTRIES_PER_PAGE)
                scroll_offset++;
        }
        if (kDown & KEY_UP && selected > 0) {
            selected--;
            if (selected < scroll_offset)
                scroll_offset--;
        }

        if (kDown & KEY_A && count > 0) {
            scanner_launch(selected);
            quit = true;  // exit so envSetNextLoad takes effect
        }

        if (kDown & KEY_R) {
            // Refresh game list
            scanner_scan();
            selected = scroll_offset = 0;
        }

        // Render
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(s_top, THEME_BG);
        C2D_TargetClear(s_bot, THEME_BG);

        C2D_SceneBegin(s_top);
        ui_draw_top(frame, selected, scroll_offset);

        C2D_SceneBegin(s_bot);
        ui_draw_bottom(frame, selected);

        C3D_FrameEnd(0);
        frame++;
    }

    // Teardown
    ui_cleanup();
    scanner_cleanup();

    cfguExit();

    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
