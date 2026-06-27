#pragma once
#include <3ds.h>
#include <stdbool.h>

#define MAX_ENTRIES    512
#define MAX_NAME_LEN   128
#define MAX_PATH_LEN   256

typedef enum {
    ENTRY_HOMEBREW,  // .3dsx — your own homebrew games / apps
    ENTRY_EMULATOR,  // .3dsx — known emulator (mGBA, RetroArch, etc.)
    ENTRY_TITLE,     // CIA-installed 3DS retail game
    ENTRY_ROM,       // ROM file — auto-launched through the right emulator
} EntryType;

typedef struct {
    char      name[MAX_NAME_LEN];
    char      path[MAX_PATH_LEN];      // .3dsx path, title path, or ROM path
    char      emu_path[MAX_PATH_LEN];  // emulator .3dsx (ENTRY_ROM only)
    char      system[12];              // "GBA", "SNES", "NES", "NDS", etc.
    EntryType type;
    u64       title_id;
    bool      emu_missing;             // true if required emulator not on SD card
} ScanEntry;

void       scanner_init(void);
void       scanner_scan(void);
int        scanner_get_count(void);
ScanEntry *scanner_get_entry(int idx);
void       scanner_launch(int idx);
bool       scanner_launch_pending(void);
void       scanner_cleanup(void);
