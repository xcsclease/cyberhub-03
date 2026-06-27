#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

// ── Title ID filtering ────────────────────────────────────────────────────────
#define TITLE_HIGH_3DS_GAME 0x00040000
#define TITLE_HIGH_DSiWARE  0x00048004

// ── Known emulator names ──────────────────────────────────────────────────────
// Filenames (without .3dsx) that are emulators, not games.
// Case-insensitive match. Add your own if needed.
static const char *KNOWN_EMUS[] = {
    "retroarch",
    "mgba",
    "snes9x",
    "snes9x_3ds",
    "tempgba",
    "gbarunner2",
    "twlightmenu",
    "twilightmenu",
    "twiliightmenu",   // common typo
    "universal-updater",
    "universal_updater",
    "fbi",
    "godmode9",
    "luma",
    "pcsx",
    "pcsxr",
    "nds-bootstrap",
    "citra",
    "unes",
    "nes_emu",
};
#define KNOWN_EMU_COUNT (sizeof(KNOWN_EMUS) / sizeof(KNOWN_EMUS[0]))

static bool is_known_emulator(const char *name_no_ext) {
    char lower[MAX_NAME_LEN];
    strncpy(lower, name_no_ext, MAX_NAME_LEN - 1);
    lower[MAX_NAME_LEN - 1] = '\0';
    for (char *p = lower; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;

    for (size_t i = 0; i < KNOWN_EMU_COUNT; i++)
        if (strcmp(lower, KNOWN_EMUS[i]) == 0) return true;
    return false;
}

// ── ROM type table ────────────────────────────────────────────────────────────
// Maps file extension → system label → emulator .3dsx path on SD card.
// CyberHub checks whether the emulator actually exists before showing the ROM.
typedef struct {
    const char *ext;
    const char *system;
    const char *emu_subpath;  // relative to sdmc:/3ds/
} RomType;

static const RomType ROM_TYPES[] = {
    // ── Game Boy family ───────────────────────────────────────────────────────
    { ".gb",   "GB",    "mGBA/mGBA.3dsx"                        },
    { ".gbc",  "GBC",   "mGBA/mGBA.3dsx"                        },
    { ".sgb",  "SGB",   "mGBA/mGBA.3dsx"                        },
    { ".gba",  "GBA",   "mGBA/mGBA.3dsx"                        },

    // ── Nintendo DS ───────────────────────────────────────────────────────────
    { ".nds",  "NDS",   "TWiLightMenu/TWiLightMenu.3dsx"         },
    { ".dsi",  "DSi",   "TWiLightMenu/TWiLightMenu.3dsx"         },
    { ".ids",  "NDS",   "TWiLightMenu/TWiLightMenu.3dsx"         },

    // ── NES / Famicom ─────────────────────────────────────────────────────────
    { ".nes",  "NES",   "RetroArch/retroarch.3dsx"               },
    { ".fds",  "FDS",   "RetroArch/retroarch.3dsx"               },
    { ".unf",  "NES",   "RetroArch/retroarch.3dsx"               },
    { ".unif", "NES",   "RetroArch/retroarch.3dsx"               },

    // ── SNES / Super Famicom ──────────────────────────────────────────────────
    { ".sfc",  "SNES",  "snes9x/snes9x_3ds.3dsx"                },
    { ".smc",  "SNES",  "snes9x/snes9x_3ds.3dsx"                },
    { ".fig",  "SNES",  "snes9x/snes9x_3ds.3dsx"                },
    { ".swc",  "SNES",  "snes9x/snes9x_3ds.3dsx"                },
    { ".bs",   "SNES",  "snes9x/snes9x_3ds.3dsx"                },

    // ── Sega ──────────────────────────────────────────────────────────────────
    { ".gen",  "GEN",   "RetroArch/retroarch.3dsx"               },
    { ".md",   "GEN",   "RetroArch/retroarch.3dsx"               },
    { ".smd",  "GEN",   "RetroArch/retroarch.3dsx"               },
    { ".gg",   "GG",    "RetroArch/retroarch.3dsx"               },
    { ".sms",  "SMS",   "RetroArch/retroarch.3dsx"               },
    { ".32x",  "32X",   "RetroArch/retroarch.3dsx"               },
    { ".cdi",  "CDi",   "RetroArch/retroarch.3dsx"               },

    // ── PC Engine / TurboGrafx-16 ─────────────────────────────────────────────
    { ".pce",  "PCE",   "RetroArch/retroarch.3dsx"               },
    { ".tg16", "TG16",  "RetroArch/retroarch.3dsx"               },

    // ── Atari ─────────────────────────────────────────────────────────────────
    { ".a26",  "A26",   "RetroArch/retroarch.3dsx"               },
    { ".a78",  "A78",   "RetroArch/retroarch.3dsx"               },
    { ".lnx",  "LYNX",  "RetroArch/retroarch.3dsx"               },
    { ".xex",  "A800",  "RetroArch/retroarch.3dsx"               },
    { ".atr",  "A800",  "RetroArch/retroarch.3dsx"               },

    // ── SNK ───────────────────────────────────────────────────────────────────
    { ".ngp",  "NGP",   "RetroArch/retroarch.3dsx"               },
    { ".ngc",  "NGPC",  "RetroArch/retroarch.3dsx"               },
    { ".ngpc", "NGPC",  "RetroArch/retroarch.3dsx"               },

    // ── WonderSwan ────────────────────────────────────────────────────────────
    { ".ws",   "WS",    "RetroArch/retroarch.3dsx"               },
    { ".wsc",  "WSC",   "RetroArch/retroarch.3dsx"               },

    // ── Virtual Boy ───────────────────────────────────────────────────────────
    { ".vb",   "VB",    "RetroArch/retroarch.3dsx"               },
    { ".vboy", "VB",    "RetroArch/retroarch.3dsx"               },

    // ── MSX ───────────────────────────────────────────────────────────────────
    { ".mx1",  "MSX",   "RetroArch/retroarch.3dsx"               },
    { ".mx2",  "MSX2",  "RetroArch/retroarch.3dsx"               },

    // ── Commodore ─────────────────────────────────────────────────────────────
    { ".d64",  "C64",   "RetroArch/retroarch.3dsx"               },
    { ".t64",  "C64",   "RetroArch/retroarch.3dsx"               },
    { ".crt",  "C64",   "RetroArch/retroarch.3dsx"               },
    { ".prg",  "C64",   "RetroArch/retroarch.3dsx"               },
    { ".tap",  "C64",   "RetroArch/retroarch.3dsx"               },

    // ── ZX Spectrum ───────────────────────────────────────────────────────────
    { ".tzx",  "ZX",    "RetroArch/retroarch.3dsx"               },
    { ".z80",  "ZX",    "RetroArch/retroarch.3dsx"               },
    { ".sna",  "ZX",    "RetroArch/retroarch.3dsx"               },

    // ── Arcade (MAME / FBA) ───────────────────────────────────────────────────
    { ".zip",  "ARC",   "RetroArch/retroarch.3dsx"               },

    // ── PlayStation 1 (New 3DS recommended) ───────────────────────────────────
    { ".cue",  "PS1",   "RetroArch/retroarch.3dsx"               },
    { ".pbp",  "PS1",   "RetroArch/retroarch.3dsx"               },
    { ".chd",  "PS1",   "RetroArch/retroarch.3dsx"               },

    // ── Nintendo 64 (New 3DS only, limited) ───────────────────────────────────
    { ".n64",  "N64",   "RetroArch/retroarch.3dsx"               },
    { ".z64",  "N64",   "RetroArch/retroarch.3dsx"               },
    { ".v64",  "N64",   "RetroArch/retroarch.3dsx"               },
};
#define ROM_TYPE_COUNT (sizeof(ROM_TYPES) / sizeof(ROM_TYPES[0]))

// ── State ─────────────────────────────────────────────────────────────────────
static ScanEntry s_entries[MAX_ENTRIES];
static int       s_count          = 0;
static bool      s_launch_pending = false;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void strip_ext(char *dst, const char *src, size_t maxlen) {
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = '\0';
    char *dot = strrchr(dst, '.');
    if (dot) *dot = '\0';
}

static void prettify_name(char *name) {
    bool cap = true;
    for (char *p = name; *p; p++) {
        if (*p == '_' || *p == '-') { *p = ' '; cap = true; }
        else if (cap && *p >= 'a' && *p <= 'z') { *p -= 32; cap = false; }
        else { cap = false; }
    }
}

static void utf16_to_ascii(char *dst, const u16 *src, size_t max) {
    size_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = (src[i] < 0x80) ? (char)src[i] : '?'; i++; }
    dst[i] = '\0';
    while (i > 0 && dst[i-1] == ' ') dst[--i] = '\0';
}

// Check whether a file exists on the SD card
static bool file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

// Find the matching ROM type for a filename extension
static const RomType *match_rom_type(const char *fname) {
    const char *dot = strrchr(fname, '.');
    if (!dot) return NULL;
    for (size_t i = 0; i < ROM_TYPE_COUNT; i++)
        if (strcasecmp(dot, ROM_TYPES[i].ext) == 0)
            return &ROM_TYPES[i];
    return NULL;
}

// ── SMDH title reader ─────────────────────────────────────────────────────────

typedef struct {
    u16 short_desc[0x40];
    u16 long_desc[0x80];
    u16 publisher[0x40];
} __attribute__((packed)) SmdhTitle;

typedef struct {
    char      magic[4];
    u16       version;
    u16       reserved;
    SmdhTitle titles[16];
} __attribute__((packed)) SmdhHeader;

static bool read_smdh_name(u64 title_id, char *dst, size_t dst_size) {
    char path[128];
    snprintf(path, sizeof(path), "sdmc:/title/%08lx/%08lx/content/00000000.app",
             (unsigned long)(title_id >> 32), (unsigned long)(title_id & 0xFFFFFFFF));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    u8 buf[512];
    SmdhHeader smdh;
    bool found = false;
    long pos = 0;

    while (!found && pos < 256 * 1024) {
        size_t rd = fread(buf, 1, sizeof(buf), f);
        if (rd < 4) break;
        for (size_t i = 0; i < rd - 3; i++) {
            if (buf[i]=='S' && buf[i+1]=='M' && buf[i+2]=='D' && buf[i+3]=='H') {
                fseek(f, pos + (long)i, SEEK_SET);
                if (fread(&smdh, sizeof(smdh), 1, f) == 1) found = true;
                break;
            }
        }
        pos += (long)rd;
    }
    fclose(f);
    if (!found) return false;
    utf16_to_ascii(dst, smdh.titles[1].short_desc, dst_size);
    return dst[0] != '\0';
}

// ── Scanners ──────────────────────────────────────────────────────────────────

// Scan a directory for .3dsx homebrew files
static void scan_homebrew_dir(const char *dir_path) {
    DIR *d = opendir(dir_path);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && s_count < MAX_ENTRIES) {
        if (ent->d_type == DT_DIR) continue;
        const char *fname = ent->d_name;
        size_t flen = strlen(fname);
        if (flen <= 5 || strcmp(fname + flen - 5, ".3dsx") != 0) continue;
        // Skip CyberHub itself
        if (strcasecmp(fname, "CyberHub.3dsx") == 0) continue;

        ScanEntry *e = &s_entries[s_count];
        memset(e, 0, sizeof(*e));
        char nameraw[MAX_NAME_LEN];
        strip_ext(nameraw, fname, MAX_NAME_LEN);
        strip_ext(e->name, fname, MAX_NAME_LEN);
        prettify_name(e->name);
        snprintf(e->path, MAX_PATH_LEN, "%s/%s", dir_path, fname);
        // Detect emulators automatically by filename
        e->type = is_known_emulator(nameraw) ? ENTRY_EMULATOR : ENTRY_HOMEBREW;
        strncpy(e->system, "3DSX", sizeof(e->system));
        s_count++;
    }
    closedir(d);
}

// Scan a directory for ROM files
static void scan_rom_dir(const char *dir_path) {
    DIR *d = opendir(dir_path);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && s_count < MAX_ENTRIES) {
        if (ent->d_type == DT_DIR) continue;
        const RomType *rt = match_rom_type(ent->d_name);
        if (!rt) continue;

        ScanEntry *e = &s_entries[s_count];
        memset(e, 0, sizeof(*e));
        strip_ext(e->name, ent->d_name, MAX_NAME_LEN);
        prettify_name(e->name);
        snprintf(e->path, MAX_PATH_LEN, "%s/%s", dir_path, ent->d_name);
        snprintf(e->emu_path, MAX_PATH_LEN, "sdmc:/3ds/%s", rt->emu_subpath);
        strncpy(e->system, rt->system, sizeof(e->system) - 1);
        e->type = ENTRY_ROM;
        e->emu_missing = !file_exists(e->emu_path);
        s_count++;
    }
    closedir(d);
}

// Scan installed CIA titles via AM service
static void scan_installed_titles(void) {
    u32 count = 0;
    if (R_FAILED(AM_GetTitleCount(MEDIATYPE_SD, &count)) || count == 0) return;

    u64 *ids = (u64 *)malloc(count * sizeof(u64));
    if (!ids) return;
    if (R_FAILED(AM_GetTitleList(NULL, MEDIATYPE_SD, count, ids))) { free(ids); return; }

    for (u32 i = 0; i < count && s_count < MAX_ENTRIES; i++) {
        u32 high = (u32)(ids[i] >> 32);
        if (high != TITLE_HIGH_3DS_GAME && high != TITLE_HIGH_DSiWARE) continue;

        ScanEntry *e = &s_entries[s_count];
        memset(e, 0, sizeof(*e));
        e->type     = ENTRY_TITLE;
        e->title_id = ids[i];
        strncpy(e->system, "3DS", sizeof(e->system));

        if (!read_smdh_name(ids[i], e->name, MAX_NAME_LEN)) {
            char prod[16] = {0};
            AM_GetTitleProductCode(MEDIATYPE_SD, ids[i], prod);
            if (prod[0]) snprintf(e->name, MAX_NAME_LEN, "%s", prod);
            else         snprintf(e->name, MAX_NAME_LEN, "Title %08lX", (unsigned long)(ids[i] & 0xFFFFFFFF));
        }
        s_count++;
    }
    free(ids);
}

// ── Public API ────────────────────────────────────────────────────────────────

void scanner_init(void) {
    s_count = 0;
    s_launch_pending = false;
    amInit();
}

void scanner_scan(void) {
    s_count = 0;
    memset(s_entries, 0, sizeof(s_entries));

    // 1. Homebrew .3dsx files in /3ds/ (and one level of subdirs)
    scan_homebrew_dir("sdmc:/3ds");
    DIR *d = opendir("sdmc:/3ds");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (ent->d_type != DT_DIR || ent->d_name[0] == '.') continue;
            char sub[MAX_PATH_LEN];
            snprintf(sub, MAX_PATH_LEN, "sdmc:/3ds/%s", ent->d_name);
            scan_homebrew_dir(sub);
        }
        closedir(d);
    }

    // 2. ROM files — scan /roms/ and common subdirectories
    const char *rom_roots[] = { "sdmc:/roms", "sdmc:/ROMs", "sdmc:/games" };
    for (int r = 0; r < 3; r++) {
        scan_rom_dir(rom_roots[r]);
        DIR *rd = opendir(rom_roots[r]);
        if (!rd) continue;
        struct dirent *ent;
        while ((ent = readdir(rd)) != NULL) {
            if (ent->d_type != DT_DIR || ent->d_name[0] == '.') continue;
            char sub[MAX_PATH_LEN];
            snprintf(sub, MAX_PATH_LEN, "%s/%s", rom_roots[r], ent->d_name);
            scan_rom_dir(sub);
        }
        closedir(rd);
    }

    // 3. Installed CIA / 3DS titles
    scan_installed_titles();
}

int        scanner_get_count(void)    { return s_count; }
ScanEntry *scanner_get_entry(int idx) { return (idx >= 0 && idx < s_count) ? &s_entries[idx] : NULL; }
bool       scanner_launch_pending(void) { return s_launch_pending; }

void scanner_launch(int idx) {
    if (idx < 0 || idx >= s_count) return;
    ScanEntry *e = &s_entries[idx];

    if (e->type == ENTRY_HOMEBREW) {
        // Directly chain-load the .3dsx (your own games work this way too)
        envSetNextLoad(e->path, e->path);
        s_launch_pending = true;

    } else if (e->type == ENTRY_ROM) {
        if (e->emu_missing) return;  // no emulator found, do nothing
        // Launch emulator with ROM path as argument
        char args[MAX_PATH_LEN * 2];
        snprintf(args, sizeof(args), "%s \"%s\"", e->emu_path, e->path);
        envSetNextLoad(e->emu_path, args);
        s_launch_pending = true;

    } else if (e->type == ENTRY_TITLE) {
        Result r = APT_PrepareToDoApplicationJump(0, e->title_id, MEDIATYPE_SD);
        if (R_SUCCEEDED(r)) {
            u8 param[0x300] = {0};
            u8 hmac[0x20]   = {0};
            APT_DoApplicationJump(param, sizeof(param), hmac);
        }
    }
}

void scanner_cleanup(void) {
    amExit();
}
