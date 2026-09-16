#include "menus/file_select.h"
#include "dma.h"
#include "oam_id.h"
#include "macros.h"
#include "callbacks.h"
#include "text.h"
#include "save_file.h"
#include "color_effects.h"
#include "time_attack.h"

#include "data/shortcut_pointers.h"
#include "data/text_data.h"
#include "data/nes_metroid.h"
#include "data/io_transfer_data.h"
#include "data/menus/file_select_data.h"

#include "constants/audio.h"
#include "constants/cable_link.h"
#include "constants/color_fading.h"
#include "constants/connection.h"
#include "constants/text.h"
#include "constants/game_state.h"
#include "constants/menus/file_select.h"
#include "constants/menus/pause_screen.h"

#include "structs/audio.h"
#include "structs/cable_link.h"
#include "structs/display.h"
#include "structs/game_state.h"
#include "structs/menus/file_select.h"

#define FILE_SELECT_TILE_TWO_DOTS_HIGH 0x1CA
#define FILE_SELECT_TILE_TWO_DOTS_LOW 0x1EA
#define FILE_SELECT_TILE_MINUS_HIGH 0x1CB
#define FILE_SELECT_TILE_MINUS_LOW 0x1EB
#define FILE_SELECT_TILE_NUMBER_HIGH 0x1CC
#define FILE_SELECT_TILE_NUMBER_LOW 0x1EC

static void OptionsUpdateStereoOam(u16 flags);
static void FileSelectResetIOTransferInfo(void);
static u8 OptionsNesMetroidHandler(void);
static u8 OptionsSubMenu_Empty(void);
static u8 OptionsGalleryHandler(void);
static u8 OptionsStereoHandler(void);
static u8 OptionsSoundTestHandler(void);
static u32 OptionsSoundTestCheckNotAlreadyPlaying(void);
static void OptionsSoundTestUpdateIdGfx(void);
static u8 OptionsTimeAttackRecordsHandler(void);
#ifndef REGION_US_BETA
static void OptionsTimeAttackLoadBestTimeMessage(void);
#endif // !REGION_US_BETA
static void OptionsTimeAttackLoadRecord(u8 id);
static void unk_7b854(void);
static void OptionsTimeAttackLoadPassword(u8 part);
static u8 OptionsMetroidFusionLinkHandler(void);
static u32 FileSelectUpdateFading(void);
static void FileSelectInitFading(u8 fadingOut);
static void FileSelectApplyFading(void);
static void FileSelectInit(void);
static void FileSelectVBlank(void);
static void FileSelectVBlank_Empty(void);
static void FileSelectDisplaySaveFileInfo(void);
static void FileSelectDisplaySaveFileHealth(u8 file);
static void FileSelectDisplaySaveFileTimer(u8 file);
static void FileSelectDisplaySaveFileMiscInfo(struct SaveFileInfo* pFile, FileSelectCursorPosition file);
static void FileScreenSetEnabledMenuFlags(void);
static u8 FileSelectApplyMenuSelectInput(u8 set, FileSelectCursorPosition* pFileNumber);
static void FileSelectFindFirstNonEmptyFile(FileSelectCursorPosition* pFileNumber);
static u8 FileSelectUpdateSubMenu(void);
static u8 FileSelectProcessFileSelection(void);
static void unk_7e3fc(u8 param_1, u8 param_2);
static u32 FileSelectUpdateTilemap(TilemapRequest request);
static void unk_7eedc(u16* pTilemap);

static s8 sSaveFileAreasId[12] = {
    [0] = AREA_KRAID,
    [1] = AREA_NORFAIR,
    [2] = AREA_RIDLEY,
    [3] = AREA_TOURIAN,
    [4] = AREA_CRATERIA,
    [5] = AREA_BRINSTAR,
    [6] = AREA_CHOZODIA,
    [7] = AREA_NONE,
    [8] = AREA_NONE,
    [9] = AREA_NONE,
    [10] = AREA_NONE,
    [11] = AREA_BRINSTAR
};

static const u32* sFileSelectOptionsTextGfxPointers[LANGUAGE_COUNT - LANGUAGE_ENGLISH] = {
    [LANGUAGE_ENGLISH - LANGUAGE_ENGLISH] = sFileSelectOptionsTextEnglishGfx,
#if defined(DEBUG) || defined(REGION_EU)
    [LANGUAGE_GERMAN - LANGUAGE_ENGLISH] = sFileSelectOptionsTextGermanGfx,
    [LANGUAGE_FRENCH - LANGUAGE_ENGLISH] = sFileSelectOptionsTextFrenchGfx,
    [LANGUAGE_ITALIAN - LANGUAGE_ENGLISH] = sFileSelectOptionsTextItalianGfx,
    [LANGUAGE_SPANISH - LANGUAGE_ENGLISH] = sFileSelectOptionsTextSpanishGfx
#else // !(DEBUG || REGION_EU)
    [LANGUAGE_GERMAN - LANGUAGE_ENGLISH] = sFileSelectOptionsTextEnglishGfx,
    [LANGUAGE_FRENCH - LANGUAGE_ENGLISH] = sFileSelectOptionsTextEnglishGfx,
    [LANGUAGE_ITALIAN - LANGUAGE_ENGLISH] = sFileSelectOptionsTextEnglishGfx,
    [LANGUAGE_SPANISH - LANGUAGE_ENGLISH] = sFileSelectOptionsTextEnglishGfx
#endif // DEBUG || REGION_EU
};

#ifdef REGION_EU
static const u32* sFileSelectLargeTextGfxPointers[LANGUAGE_COUNT - LANGUAGE_ENGLISH] = {
    [LANGUAGE_ENGLISH - LANGUAGE_ENGLISH] = sFileSelectLargeTextEnglishGfx,
    [LANGUAGE_GERMAN - LANGUAGE_ENGLISH] = sFileSelectLargeTextGermanGfx,
    [LANGUAGE_FRENCH - LANGUAGE_ENGLISH] = sFileSelectLargeTextFrenchGfx,
    [LANGUAGE_ITALIAN - LANGUAGE_ENGLISH] = sFileSelectLargeTextItalianGfx,
    [LANGUAGE_SPANISH - LANGUAGE_ENGLISH] = sFileSelectLargeTextSpanishGfx
};

static const u32* sFileSelectDifficultyTextGfxPointers[LANGUAGE_COUNT - LANGUAGE_ENGLISH] = {
    [LANGUAGE_ENGLISH - LANGUAGE_ENGLISH] = sFileSelectDifficultyTextEnglishGfx,
    [LANGUAGE_GERMAN - LANGUAGE_ENGLISH] = sFileSelectDifficultyTextGermanGfx,
    [LANGUAGE_FRENCH - LANGUAGE_ENGLISH] = sFileSelectDifficultyTextFrenchGfx,
    [LANGUAGE_ITALIAN - LANGUAGE_ENGLISH] = sFileSelectDifficultyTextItalianGfx,
    [LANGUAGE_SPANISH - LANGUAGE_ENGLISH] = sFileSelectDifficultyTextSpanishGfx
};
#endif // REGION_EU

static struct FileSelectMenuCursors sFileSelectMenuCursors_Empty = {
    .confirmCopy = 1,
    .confirmErase = 1,
    .startGame = 0,
    .completedFileOptions = 0,
    .confirmOverwritingCompleted = 1,
    .japaneseText = 0,
    .difficulty = 1
};

static struct FileSelectCursorOamData sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_COUNT] = {
    [FILE_SELECT_CURSOR_POSITION_FILE_A] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 2 - QUARTER_BLOCK_SIZE + 12,
        .oamFileMarkerId = FILE_SELECT_OAM_ID_FILE_A_MARKER_IDLE,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_SAMUS_SUITLESS_HEAD_TURNING_ON
        }
    },
    [FILE_SELECT_CURSOR_POSITION_FILE_B] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 3 + QUARTER_BLOCK_SIZE + 12,
        .oamFileMarkerId = FILE_SELECT_OAM_ID_FILE_B_MARKER_IDLE,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_SAMUS_SUITLESS_HEAD_TURNING_ON
        }
    },
    [FILE_SELECT_CURSOR_POSITION_FILE_C] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 5 - QUARTER_BLOCK_SIZE + 12,
        .oamFileMarkerId = FILE_SELECT_OAM_ID_FILE_C_MARKER_IDLE,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_SAMUS_HEAD_TURNING_ON,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_SAMUS_SUITLESS_HEAD_TURNING_ON
        }
    },
    [FILE_SELECT_CURSOR_POSITION_COPY] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 7 - QUARTER_BLOCK_SIZE + 8,
        .oamFileMarkerId = 0,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_CURSOR
        }
    },
    [FILE_SELECT_CURSOR_POSITION_ERASE] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 8 - QUARTER_BLOCK_SIZE + 8,
        .oamFileMarkerId = 0,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_CURSOR
        }
    },
    [FILE_SELECT_CURSOR_POSITION_OPTIONS] = {
        .xPosition = BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        .yPosition = BLOCK_SIZE * 9 - QUARTER_BLOCK_SIZE + 8,
        .oamFileMarkerId = 0,
        .oamIds = {
            [SUIT_NORMAL] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_FULLY_POWERED] = FILE_SELECT_OAM_ID_CURSOR,
            [SUIT_SUITLESS] = FILE_SELECT_OAM_ID_CURSOR
        }
    }
};

static u8 sFileSelectCopyFileArrowsOamIds[3][3] = {
    [0] = {
        [0] = 0,
        [1] = FILE_SELECT_OAM_ID_COPY_ARROW_ONE_DOWN_ACTIVE,
        [2] = FILE_SELECT_OAM_ID_COPY_ARROW_TWO_DOWN_ACTIVE
    },
    [1] = {
        [0] = FILE_SELECT_OAM_ID_COPY_ARROW_ONE_UP_ACTIVE,
        [1] = 0,
        [2] = FILE_SELECT_OAM_ID_COPY_ARROW_ONE_DOWN_ACTIVE
    },
    [2] = {
        [0] = FILE_SELECT_OAM_ID_COPY_ARROW_TWO_UP_ACTIVE,
        [1] = FILE_SELECT_OAM_ID_COPY_ARROW_ONE_UP_ACTIVE,
        [2] = 0
    }
};

static u8 sFileSelectFileOamOffsets[3][2] = {
    [0] = {
        [0] = FILE_SELECT_OAM_FILE_A_MARKER,
        [1] = FILE_SELECT_OAM_FILE_A_LOGO
    },
    [1] = {
        [0] = FILE_SELECT_OAM_FILE_B_MARKER,
        [1] = FILE_SELECT_OAM_FILE_B_LOGO
    },
    [2] = {
        [0] = FILE_SELECT_OAM_FILE_C_MARKER,
        [1] = FILE_SELECT_OAM_FILE_C_LOGO
    }
};

static u16 sOptionsCursorPosition[7][2] = {
    [0] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE + 8
    },
    [1] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 3 + QUARTER_BLOCK_SIZE + 8
    },
    [2] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 4 + QUARTER_BLOCK_SIZE + 8
    },
    [3] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 5 + QUARTER_BLOCK_SIZE + 8
    },
    [4] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 6 + QUARTER_BLOCK_SIZE + 8
    },
    [5] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 7 + QUARTER_BLOCK_SIZE + 8
    },
    [6] = {
        BLOCK_SIZE * 2 + QUARTER_BLOCK_SIZE,
        BLOCK_SIZE * 8 + QUARTER_BLOCK_SIZE + 8
    }
};

// x_pos, y_pos, idle_id, active_id
static u16 sStereoOamData[2][4] = {
    [FALSE] = {
        BLOCK_SIZE * 7 + HALF_BLOCK_SIZE,
        BLOCK_SIZE * 2 + HALF_BLOCK_SIZE,
        OPTIONS_OAM_ID_GBA_IDLE,
        OPTIONS_OAM_ID_GBA_ACTIVE
    },
    [TRUE] = {
        BLOCK_SIZE * 11,
        BLOCK_SIZE * 2 + HALF_BLOCK_SIZE,
        OPTIONS_OAM_ID_SPEAKERS_IDLE,
        OPTIONS_OAM_ID_SPEAKERS_ACTIVE
    }
};

static u16 sSoundTestSoundIds[24] = {
    [0] = MUSIC_NONE,
    [1] = MUSIC_FILE_SELECT,
    [2] = MUSIC_BRINSTAR,
    [3] = MUSIC_TITLE_SCREEN,
    [4] = MUSIC_WORMS_BATTLE,
    [5] = MUSIC_NORFAIR,
    [6] = MUSIC_CHOZO_RUINS_DEPTH,
    [7] = MUSIC_CHOZO_RUINS,
    [8] = MUSIC_KRAID,
    [9] = MUSIC_KRAID_BATTLE,
    [10] = MUSIC_STATUE_ROOM,
    [11] = MUSIC_CATTERPILLARS_BATTLE,
    [12] = MUSIC_IMAGO_COCOON_BATTLE,
    [13] = MUSIC_RIDLEY,
    [14] = MUSIC_IMAGO_BATTLE,
    [15] = MUSIC_RIDLEY_BATTLE_2,
    [16] = MUSIC_TOURIAN,
    [17] = MUSIC_ESCAPE,
    [18] = MUSIC_STEALTH,
    [19] = MUSIC_MAP_ROOM,
    [20] = MUSIC_ALARM_ACTIVATED,
    [21] = MUSIC_RUINS_TEST_BATTLE,
    [22] = MUSIC_MECHA_RIDLEY_BATTLE_2,
    [23] = MUSIC_CREDITS
};

/**
 * @brief 78228 | 24 | Applies the stereo settings
 * 
 */
void FileSelectApplyStereo(void)
{
    u32 action;

    if (gStereoFlag >= 2)
        return;

    if (gStereoFlag)
        action = SOUND_ACTION_ENABLE_STEREO;
    else
        action = SOUND_ACTION_DISABLE_STEREO;

    DoSoundAction(action);
}

/**
 * @brief 7824c | 60 | Processes the OAM for the file select and options screens
 * 
 */
void FileSelectProcessOam(void)
{
    gNextOamSlot = 0;

    switch (gSubGameMode1)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        default:
            ProcessMenuOam(ARRAY_SIZE(FILE_SELECT_DATA.fileScreenOam), FILE_SELECT_DATA.fileScreenOam, sFileSelectOam);
            break;

        case 9:
        case 10:
        case 11:
            ProcessMenuOam(ARRAY_SIZE(FILE_SELECT_DATA.optionsOam), FILE_SELECT_DATA.optionsOam, sOptionsOam);
    }

    ResetFreeOam();
}

/**
 * @brief 782ac | 258 | Initializes the OAM for the file and options screens
 * 
 */
static void FileSelectResetOam(void)
{
    s32 i;
    struct MenuOamData* pOam;

    // Reset file screen OAM
    pOam = FILE_SELECT_DATA.fileScreenOam;
    for (i = 0; i < ARRAY_SIZE(FILE_SELECT_DATA.fileScreenOam); i++)
        pOam[i] = sFileSelectOamData_Empty[0];

    // Setup file markers
    pOam[FILE_SELECT_OAM_FILE_A_MARKER].xPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].xPosition;
    pOam[FILE_SELECT_OAM_FILE_A_MARKER].yPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].yPosition;
    UpdateMenuOamDataId(&pOam[FILE_SELECT_OAM_FILE_A_MARKER], sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId);

    pOam[FILE_SELECT_OAM_FILE_B_MARKER].xPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].xPosition;
    pOam[FILE_SELECT_OAM_FILE_B_MARKER].yPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].yPosition;
    UpdateMenuOamDataId(&pOam[FILE_SELECT_OAM_FILE_B_MARKER], sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId);

    pOam[FILE_SELECT_OAM_FILE_C_MARKER].xPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].xPosition;
    pOam[FILE_SELECT_OAM_FILE_C_MARKER].yPosition = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].yPosition;
    UpdateMenuOamDataId(&pOam[FILE_SELECT_OAM_FILE_C_MARKER], sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId);

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].boundBackground = 1;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].priority = BGCNT_LOW_MID_PRIORITY;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].boundBackground = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].priority = BGCNT_HIGH_MID_PRIORITY;
    
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0].boundBackground = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_1].boundBackground = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_2].boundBackground = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0].priority = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_1].priority = 0;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_2].priority = 0;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].priority = BGCNT_LOW_PRIORITY;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].priority = BGCNT_LOW_PRIORITY;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].priority = BGCNT_LOW_PRIORITY;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].oamId = FILE_SELECT_OAM_ID_METROID_LOGO;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].oamId = FILE_SELECT_OAM_ID_METROID_LOGO;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].oamId = FILE_SELECT_OAM_ID_METROID_LOGO;

    // Check enable metroid logos (completed save file indicator)
    if (gSaveFilesInfo[0].completedGame)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].exists = TRUE;

    if (gSaveFilesInfo[1].completedGame)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].exists = TRUE;

    if (gSaveFilesInfo[2].completedGame)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].exists = TRUE;

    if (gSaveFilesInfo[0].corruptionFlag)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = TRUE;

    if (gSaveFilesInfo[1].corruptionFlag)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = TRUE;

    if (gSaveFilesInfo[2].corruptionFlag)
        FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = TRUE;

    // Set position of the metroid logos
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].xPosition = BLOCK_SIZE * 14;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].xPosition = BLOCK_SIZE * 14;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].xPosition = BLOCK_SIZE * 14;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].yPosition = BLOCK_SIZE * 2;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].yPosition = BLOCK_SIZE * 3 + HALF_BLOCK_SIZE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].yPosition = BLOCK_SIZE * 5;

    // Reset options OAM
    pOam = FILE_SELECT_DATA.optionsOam;
    for (i  = 0; i < ARRAY_SIZE(FILE_SELECT_DATA.optionsOam); i++)
        pOam[i] = sFileSelectOamData_Empty[0];

    // Update stereo OAM
    OptionsUpdateStereoOam(USHORT_MAX | STEREO_UPDATE_FLAGS_SOUND_TEST | STEREO_UPDATE_FLAGS_ACTIVE | STEREO_UPDATE_FLAGS_IDLE);
}

/**
 * @brief 78504 | 24c | Updates the cursor and file marker OAM
 * 
 * @param cursorPose Cursor pose
 * @param position Cursor position
 */
static void FileSelectUpdateCursor(CursorPose cursorPose, FileSelectCursorPosition position)
{
    u32 oamId;

    if (position < FILE_SELECT_CURSOR_POSITION_COPY)
        oamId = gSaveFilesInfo[position].suitType; // Cursor based on current suit
    else
        oamId = SUIT_NORMAL;

    oamId = sFileSelectCursorOamData[position].oamIds[oamId];

    switch (cursorPose)
    {
        case CURSOR_POSE_DEFAULT:
            // Not doing anything
            oamId++;

        case CURSOR_POSE_MOVING:
            // Moving cursor, update oam and position
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR], oamId);
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].xPosition = sFileSelectCursorOamData[position].xPosition;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].yPosition = sFileSelectCursorOamData[position].yPosition;

            // Update each file marker
            // If on file -> red marker, otherwise use idle marker

            // File A
            if (position != FILE_SELECT_CURSOR_POSITION_FILE_A)
                oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId;
            else
                oamId = FILE_SELECT_OAM_ID_FILE_A_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER], oamId);

            // File B
            if (position != FILE_SELECT_CURSOR_POSITION_FILE_B)
                oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId;
            else
                oamId = FILE_SELECT_OAM_ID_FILE_B_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER], oamId);

            // File C
            if (position != FILE_SELECT_CURSOR_POSITION_FILE_C)
                oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId;
            else
                oamId = FILE_SELECT_OAM_ID_FILE_C_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER], oamId);
            break;

        case CURSOR_POSE_OPENING_OPTIONS:
            // Update OAM
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR], oamId + 2);
            break;

        case CURSOR_POSE_SELECTING_FILE:
            // Update OAM
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR], oamId + 2);

            if (position != FILE_SELECT_CURSOR_POSITION_FILE_A)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].objMode = 1;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].objMode = 1;
            }

            if (position != FILE_SELECT_CURSOR_POSITION_FILE_B)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].objMode = 1;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].objMode = 1;
            }

            if (position != FILE_SELECT_CURSOR_POSITION_FILE_C)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].objMode = 1;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].objMode = 1;
            }
            break;

        case CURSOR_POSE_DESELECTING_FILE:
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR], ++oamId);

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].objMode = 0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].objMode = 0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].objMode = 0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].objMode = 0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].objMode = 0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].objMode = 0;
            break;

        case CURSOR_POSE_STARTING_GAME:
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR], oamId + 5);
            break;

        case 8:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].oamId = oamId + 4;
            break;
    }
}

/**
 * @brief 78750 | 184 | Updates the copy cursor and file marker OAM
 * 
 * @param cursorPose Cursor pose
 * @param fileNumber File number
 */
static void FileSelectUpdateCopyCursor(CursorCopyPose cursorPose, FileSelectCursorPosition fileNumber)
{
    u32 oamId;

    if (fileNumber < FILE_SELECT_CURSOR_POSITION_COPY)
        oamId = gSaveFilesInfo[fileNumber].suitType; // Cursor based on current suit
    else
        oamId = SUIT_NORMAL;

    oamId = sFileSelectCursorOamData[fileNumber].oamIds[oamId];

    switch (cursorPose)
    {
        case CURSOR_COPY_POSE_DEFAULT:
            oamId++;

        case CURSOR_COPY_POSE_MOVING:
            // Update oam id and position
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], oamId);
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].xPosition = sFileSelectCursorOamData[fileNumber].xPosition;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].yPosition = sFileSelectCursorOamData[fileNumber].yPosition;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].boundBackground = 2;

            // Update each file marker
            // If on file and exists or intro played -> red marker, otherwise use idle marker

            // File A
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_A && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_A_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER], oamId);

            // File B
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_B && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_B_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER], oamId);

            // File C
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_C && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_C_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER], oamId);
            break;

        case CURSOR_COPY_POSE_COPIED:
            // Kill copy cursor OAM
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].exists = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].oamId = 0;

            // Reset every file marker to non-selected 
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId);
            break;

        case CURSOR_COPY_POSE_SELECTING_FILE:
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], oamId + 2);
            break;
    }
}

/**
 * @brief 788d4 | 140 | Updates the copy arrow and file marker OAM
 * 
 * @param arrowPose Arrow pose
 * @param dstFileNumber Destination file number
 */
static void FileSelectUpdateCopyArrow(ArrowCopyPose arrowPose, u8 dstFileNumber)
{
    u32 oamId;

    // Update position
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_COPY_ARROW].xPosition = sFileSelectCursorOamData[FILE_SELECT_DATA.copySourceFile].xPosition;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_COPY_ARROW].yPosition = sFileSelectCursorOamData[FILE_SELECT_DATA.copySourceFile].yPosition;

    switch (arrowPose)
    {
        case ARROW_COPY_POSE_DEFAULT:
        case ARROW_COPY_POSE_MOVING:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_COPY_ARROW].boundBackground = 2;

            // Update id
            oamId = sFileSelectCopyFileArrowsOamIds[FILE_SELECT_DATA.copySourceFile][dstFileNumber];
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_COPY_ARROW], oamId);

            // Update each file marker
            // If on file -> green marker, otherwise use idle marker

            // File A
            if (FILE_SELECT_DATA.copySourceFile != FILE_SELECT_CURSOR_POSITION_FILE_A)
            {
                if (dstFileNumber != FILE_SELECT_CURSOR_POSITION_FILE_A)
                    oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId;
                else
                    oamId = FILE_SELECT_OAM_ID_FILE_A_MARKER_SELECTED_GREEN;

                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER], oamId);
            }

            // File B
            if (FILE_SELECT_DATA.copySourceFile != FILE_SELECT_CURSOR_POSITION_FILE_B)
            {
                if (dstFileNumber != FILE_SELECT_CURSOR_POSITION_FILE_B)
                    oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId;
                else
                    oamId = FILE_SELECT_OAM_ID_FILE_B_MARKER_SELECTED_GREEN;

                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER], oamId);
            }

            // File C
            if (FILE_SELECT_DATA.copySourceFile != FILE_SELECT_CURSOR_POSITION_FILE_C)
            {
                if (dstFileNumber != FILE_SELECT_CURSOR_POSITION_FILE_C)
                    oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId;
                else
                    oamId = FILE_SELECT_OAM_ID_FILE_C_MARKER_SELECTED_GREEN;

                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER], oamId);
            }

            break;

        case ARROW_COPY_POSE_KILL:
            // Kill arrow OAM
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_COPY_ARROW].exists = FALSE;
            
            // Reset every file marker
            if (dstFileNumber == FILE_SELECT_CURSOR_POSITION_FILE_A)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER],
                    sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId);
            }
            
            if (dstFileNumber == FILE_SELECT_CURSOR_POSITION_FILE_B)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER],
                    sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId);
            }
            
            if (dstFileNumber == FILE_SELECT_CURSOR_POSITION_FILE_C)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER],
                    sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId);
            }
            break;

        case ARROW_COPY_POSE_COPYING:
            // Nothing to do
            break;
    }
}

/**
 * @brief 78a14 | 184 | Updates the erase cursor and file marker OAM
 * 
 * @param cursorPose Cursor pose
 * @param fileNumber File number
 */
static void FileSelectUpdateEraseCursor(CursorErasePose cursorPose, u8 fileNumber)
{
    u32 oamId;

    if (fileNumber < FILE_SELECT_CURSOR_POSITION_COPY)
        oamId = gSaveFilesInfo[fileNumber].suitType; // Cursor based on current suit
    else
        oamId = 0;

    oamId = sFileSelectCursorOamData[fileNumber].oamIds[oamId];

    switch (cursorPose)
    {
        case CURSOR_ERASE_POSE_DEFAULT:
            oamId++;

        case CURSOR_ERASE_POSE_MOVING:
            // Update oam id and position
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], oamId);
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].xPosition = sFileSelectCursorOamData[fileNumber].xPosition;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].yPosition = sFileSelectCursorOamData[fileNumber].yPosition;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].boundBackground = 2;

            // Update each file marker
            // If on file and exists or intro played -> red marker, otherwise use idle marker

            // File A
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_A && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_A_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER], oamId);

            // File B
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_B && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_B_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER], oamId);

            // File C
            oamId = sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId;
            if (fileNumber == FILE_SELECT_CURSOR_POSITION_FILE_C && (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].exists ||
                gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].introPlayed))
                oamId = FILE_SELECT_OAM_ID_FILE_C_MARKER_SELECTED_RED;
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER], oamId);
            break;

        case CURSOR_ERASE_POSE_ERASED:
            // Kill erase cursor OAM
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].exists = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].oamId = 0;

            // Reset every file marker to non-selected 
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_A].oamFileMarkerId);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_B].oamFileMarkerId);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER],
                sFileSelectCursorOamData[FILE_SELECT_CURSOR_POSITION_FILE_C].oamFileMarkerId);
            break;

        case CURSOR_ERASE_POSE_SELECTING_FILE:
            UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], oamId + 2);
            break;
    }
}

/**
 * @brief 78b98 | 108 | Updates the options cursor OAM
 * 
 * @param cursorPose Cursor pose
 */
static void OptionsUpdateCursor(CursorOptionsPose cursorPose)
{
    switch (cursorPose)
    {
        case CURSOR_OPTIONS_POSE_ENTERING:
            SoundPlay(SOUND_SUB_MENU_CURSOR);

        case CURSOR_OPTIONS_POSE_MOVING:
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].xPosition = sOptionsCursorPosition[gOptionsOptionSelected][0];
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].yPosition = sOptionsCursorPosition[gOptionsOptionSelected][1];
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].boundBackground = 2;

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].oamId != OPTIONS_OAM_ID_CURSOR)
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR], OPTIONS_OAM_ID_CURSOR);
            break;

        case CURSOR_OPTIONS_POSE_DESELECTING:
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].oamId != OPTIONS_OAM_ID_CURSOR)
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR], OPTIONS_OAM_ID_CURSOR);
            break;

        case CURSOR_OPTIONS_POSE_SELECTING:
            SoundPlay(SOUND_ACCEPT_CONFIRM_MENU);

        case 4:
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].oamId != OPTIONS_OAM_ID_CURSOR_SELECTED)
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR], OPTIONS_OAM_ID_CURSOR_SELECTED);
    }

    FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_CURSOR].priority = BGCNT_LOW_MID_PRIORITY;
}

/**
 * @brief 78ca0 | 100 | Updates the stereo symbols OAM
 * 
 * @param flags Update flags
 */
static void OptionsUpdateStereoOam(StereoUpdateFlags flags)
{
    u32 offset;

    if (flags & (STEREO_UPDATE_FLAGS_IDLE | STEREO_UPDATE_FLAGS_ACTIVE))
    {
        if (flags & STEREO_UPDATE_FLAGS_IDLE)
            offset = 2;
        else
            offset = 3;

        UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SPEAKER], sStereoOamData[gStereoFlag][offset]);

        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SPEAKER].xPosition = sStereoOamData[gStereoFlag][0];
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SPEAKER].yPosition = sStereoOamData[gStereoFlag][1];
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SPEAKER].priority = BGCNT_LOW_MID_PRIORITY;
    }

    if (flags & STEREO_UPDATE_FLAGS_SOUND_TEST)
    {
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].xPosition = BLOCK_SIZE * 10 + HALF_BLOCK_SIZE;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].yPosition = BLOCK_SIZE * 5;

        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].xPosition = FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].xPosition;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].yPosition = FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].yPosition;

        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].xPosition = FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].xPosition;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].yPosition = FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].yPosition;

#ifdef REGION_EU
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].priority = BGCNT_LOW_MID_PRIORITY;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].priority = BGCNT_LOW_MID_PRIORITY;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].priority = BGCNT_LOW_MID_PRIORITY;
#else // !REGION_EU
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].priority = BGCNT_LOW_PRIORITY;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].priority = BGCNT_LOW_PRIORITY;
        FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].priority = BGCNT_LOW_PRIORITY;
#endif // REGION_EU
    }
}

static u32* sFileSelect_760bdc[4] = {
    VRAM_BASE + 0x5800,
    VRAM_BASE + 0x6000,
    VRAM_BASE + 0x6200,
    VRAM_BASE + 0x7000
};

/**
 * @brief 78da0 | 32c | Processes the current file screen text
 * 
 */
static void FileScreenProcessText(void)
{
    u8 newIdQueue[2];
    u32* dst;
    s32 var_0;
#ifdef REGION_EU
    s32 i;
    u32 flag;
#endif // REGION_EU

    switch (FILE_SELECT_DATA.processTextStage)
    {
        case FILE_SCREEN_PROCESS_TEXT_STAGE_NONE:
            break;

        case FILE_SCREEN_PROCESS_TEXT_STAGE_INIT:
            gCurrentMessage = sFileScreenMessage_Empty;

            // always true?
            if (gCurrentMessage.isMessage)
                var_0 = -1;
            else
                var_0 = 0;

            if (sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1] == 3 ||
                sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1] == 1)
            {
#ifdef REGION_EU
                BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x1000, 16);
#else // !REGION_EU
                DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x1000);
#endif // REGION_EU
            }
            else if (sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1] == 2)
            {
#ifdef REGION_EU
                BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x200, 16);
                BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x100, 0x200, 16);
                BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x180, 0x800, 16);
#else // !REGION_EU
                DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x200);
                DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x400, 0x200);
                DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x600, 0x800);
#endif // REGION_EU
            }
            else
            {
#ifdef REGION_EU
                BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x800, 16);
#else // !REGION_EU
                DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]], 0x800);
#endif // REGION_EU

                if (sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][2] == 3)
                {
#ifdef REGION_EU
                    BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x200, 0x200, 16);
                    BitFill(3, (u16)var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x300, 0x200, 16);
#else // !REGION_EU
                    DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0x800, 0x200);
                    DMA3_FILL_16(var_0, sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]] + 0xC00, 0x200);
#endif // REGION_EU
                }
            }

            FILE_SELECT_DATA.processTextStage++;
            break;

#ifdef REGION_EU
        case FILE_SCREEN_PROCESS_TEXT_STAGE_LINE:
            i = 8;

            if (gCurrentMessage.line > 3 || gCurrentMessage.messageEnded)
            {
                i = 0;
            }
            else
            {
                dst = sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]];

                if (gCurrentMessage.line & 2)
                    dst += 0x200;
    
                if (gCurrentMessage.line & 1)
                {
                    dst += 0x80;
                
                    if (sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1] == 2)
                        dst += 0x100;
                }
            }

            if (i == 0)
            {
                gCurrentMessage.stage = 3;
                break;
            }

            while (i != 0)
            {
                var_0 = TextProcessCurrentMessage(&gCurrentMessage, sFileScreenTextPointers[gLanguage][sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][0]], dst);

                switch (var_0)
                {
                    case TEXT_STATE_ENDED:
                        FILE_SELECT_DATA.processTextStage++;

                    case TEXT_STATE_NEW_LINE:
                        gCurrentMessage.indent = 0;
                        flag = TRUE;
                        break;

                    default:
                        flag = FALSE;
                }
                if (flag)
                    break;
                
                if (gCurrentMessage.line > 3)
                    break;

                i--;
            }
            break;

#else // !REGION_EU
        case FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_1:
        case FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_2:
        case FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_3:
        case FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_4:
            dst = sFileSelect_760bdc[sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1]];

            var_0 = FILE_SELECT_DATA.processTextStage - FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_1;
            // FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_3
            if (var_0 & 2)
                dst += 0x200;

            // FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_2 and FILE_SCREEN_PROCESS_TEXT_STAGE_LINE_4
            if (var_0 & 1)
            {
                dst += 0x80;
            
                if (sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][1] == 2)
                    dst += 0x100;
            }

            gCurrentMessage.indent = 0;

            while (TRUE)
            {
                var_0 = TextProcessCurrentMessage(&gCurrentMessage, sFileScreenTextPointers[gLanguage][sFileScreenMessagesInfo[FILE_SELECT_DATA.messageInfoIdQueue[0]][0]], dst);

                if (var_0 == TEXT_STATE_ENDED)
                {
                    FILE_SELECT_DATA.processTextStage = FILE_SCREEN_PROCESS_TEXT_STAGE_UPDATE_QUEUE;
                    break;
                }
                
                if (var_0 == TEXT_STATE_NEW_LINE)
                {
                    FILE_SELECT_DATA.processTextStage++;
                    break;
                }
            }
            break;
#endif // REGION_EU

        case FILE_SCREEN_PROCESS_TEXT_STAGE_UPDATE_QUEUE:
            FILE_SELECT_DATA.messageInfoIdQueue[0] = UCHAR_MAX;
            var_0 = 0;

            newIdQueue[0] = UCHAR_MAX;
            newIdQueue[1] = UCHAR_MAX;

            if (FILE_SELECT_DATA.messageInfoIdQueue[1] != UCHAR_MAX)
            {
                newIdQueue[0] = FILE_SELECT_DATA.messageInfoIdQueue[1];
                FILE_SELECT_DATA.messageInfoIdQueue[1] = UCHAR_MAX;
                var_0++;
            }

            if (FILE_SELECT_DATA.messageInfoIdQueue[2] != UCHAR_MAX)
            {
                newIdQueue[var_0] = FILE_SELECT_DATA.messageInfoIdQueue[2];
                FILE_SELECT_DATA.messageInfoIdQueue[2] = UCHAR_MAX;
            }

            FILE_SELECT_DATA.messageInfoIdQueue[0] = newIdQueue[0];
            FILE_SELECT_DATA.messageInfoIdQueue[1] = newIdQueue[1];

            FILE_SELECT_DATA.processTextStage = FILE_SELECT_DATA.messageInfoIdQueue[0] != UCHAR_MAX;
    }
}

/**
 * @brief 790cc | ac | Update the message info ID queue
 * 
 * @param updateOption Option for update, 0: add to queue, 1: check if not in queue, 2: reset queue
 * @param newMessageInfoId ID for new message info
 * @return u32 Bool, result of update, option 0: added to queue, option 1: not in queue, option 2: always true
 */
static u32 FileScreenUpdateMessageInfoIdQueue(u8 updateOption, u8 newMessageInfoId)
{
    u32 result;

    result = TRUE;

    if (updateOption == 0)
    {
        if (FILE_SELECT_DATA.messageInfoIdQueue[0] == UCHAR_MAX)
        {
            FILE_SELECT_DATA.messageInfoIdQueue[0] = newMessageInfoId;
            FILE_SELECT_DATA.processTextStage = FILE_SCREEN_PROCESS_TEXT_STAGE_INIT;
        }
        else if (FILE_SELECT_DATA.messageInfoIdQueue[1] == UCHAR_MAX)
        {
            FILE_SELECT_DATA.messageInfoIdQueue[1] = newMessageInfoId;
        }
        else if (FILE_SELECT_DATA.messageInfoIdQueue[2] == UCHAR_MAX)
        {
            FILE_SELECT_DATA.messageInfoIdQueue[2] = newMessageInfoId;
        }
        else
            result = FALSE;
    }

    else if (updateOption == 1)
    {
        if (FILE_SELECT_DATA.messageInfoIdQueue[0] == newMessageInfoId)
            result = FALSE;

        if (FILE_SELECT_DATA.messageInfoIdQueue[1] == newMessageInfoId)
            result = FALSE;

        if (FILE_SELECT_DATA.messageInfoIdQueue[2] == newMessageInfoId)
            result = FALSE;
    }

    else
    {
        FILE_SELECT_DATA.messageInfoIdQueue[0] = UCHAR_MAX;
        FILE_SELECT_DATA.messageInfoIdQueue[1] = UCHAR_MAX;
        FILE_SELECT_DATA.messageInfoIdQueue[2] = UCHAR_MAX;
        FILE_SELECT_DATA.processTextStage = FILE_SCREEN_PROCESS_TEXT_STAGE_NONE;
        result = TRUE;
    }
    
    return result;
}

/**
 * @brief 79178 | 98 | Selects a file to be used as a copy destination
 * 
 */
static void FileSelectFileCopyChooseBaseDestinationFile(void)
{
    s32 file;

    file = -1;

    // Find first empty file
    if (!gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].exists && !gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].introPlayed)
        file = FILE_SELECT_CURSOR_POSITION_FILE_A;
    else if (!gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].exists && !gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].introPlayed)
        file = FILE_SELECT_CURSOR_POSITION_FILE_B;
    else if (!gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].exists && !gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].introPlayed)
        file = FILE_SELECT_CURSOR_POSITION_FILE_C;

    if (file < 0)
    {
        // No file found -> every file already exists, thus use the "highest" one (A -> B ; B -> A ; C -> A)

        file = FILE_SELECT_DATA.currentFile;
        if (file == 0x80 || FILE_SELECT_DATA.copySourceFile == file)
        {
            // Swap files A and B
            if (FILE_SELECT_DATA.copySourceFile == FILE_SELECT_CURSOR_POSITION_FILE_B)
                file = FILE_SELECT_CURSOR_POSITION_FILE_A;
            else
                file = FILE_SELECT_CURSOR_POSITION_FILE_B;
        }
    }

    FILE_SELECT_DATA.currentFile = file;
}

u32 FileSelectCopyFileHandler(void)
{
    u32 ended;
    s32 action;
    u8 subroutineStage;
    
    ended = FALSE;

    FILE_SELECT_DATA.subroutineTimer++;
    
    subroutineStage = FILE_SELECT_DATA.subroutineStage;
    switch (subroutineStage)
    {
        case 0:
            unk_790cc(0, 1);
            unk_790cc(0, 2);

            FileSelectFindFirstNonEmptyFile(&FILE_SELECT_DATA.copySourceFile);
            FileSelectUpdateCopyCursor(CURSOR_COPY_POSE_DEFAULT, FILE_SELECT_DATA.copySourceFile);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_SPAWN_INIT);

            FILE_SELECT_DATA.fileSelectData.unk_0 = sFileSelectionData_Empty.unk_0;
            FILE_SELECT_DATA.unk_3A = 0;

            FILE_SELECT_DATA.subroutineStage++;
            break;

        case 1:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_SPAWN))
                FILE_SELECT_DATA.subroutineStage++;
            break;

        case 2:
            if (unk_790cc(1, 2))
                FILE_SELECT_DATA.subroutineStage++;
            break;

        case 3:
            action = 0;

            if (gChangedInput)
            {
                if (FileSelectApplyMenuSelectInput(FILE_SELECT_SELECTION_SET_ONLY_FILES, &FILE_SELECT_DATA.copySourceFile))
                {
                    action = 1;
                }
                else if (gChangedInput & KEY_B)
                {
                    goto lbl_case_14;
                }
                else if (gChangedInput & KEY_A)
                {
                    action = (FILE_SELECT_DATA.enabledMenus >> FILE_SELECT_DATA.copySourceFile) & 1 ? 2 : 0;
                }
            }

            if (action == 0)
                break;

            if (action == 1)
            {
                FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CURSOR);
                FileSelectUpdateCopyCursor(ARROW_COPY_POSE_MOVING, FILE_SELECT_DATA.copySourceFile);
            }
            else if (action == 2)
            {
                FileSelectUpdateCopyCursor(ARROW_COPY_POSE_COPYING, FILE_SELECT_DATA.copySourceFile);
                
                if (gSaveFilesInfo[FILE_SELECT_DATA.copySourceFile].timeAttack)
                {
                    SoundPlay(0x209);
                    unk_790cc(0, 4);
                    FILE_SELECT_DATA.subroutineStage = 9;
                }
                else
                {
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_COPY_DELETE);
                    FileSelectFileCopyChooseBaseDestinationFile();
                    FileSelectUpdateCopyArrow(ARROW_COPY_POSE_DEFAULT, FILE_SELECT_DATA.currentFile);
                    FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_DESTINATION_SPAWN);
                    FILE_SELECT_DATA.subroutineStage++;
                }
            }
            break;

        case 4:
            action = 0;

            if (gChangedInput)
            {
                if (FileSelectApplyMenuSelectInput(FILE_SELECT_SELECTION_SET_ALL_FILES_NO_COPY, &FILE_SELECT_DATA.currentFile))
                    action = 1;
                else if (gChangedInput & KEY_B)
                    action = -1;
                else if (gChangedInput & KEY_A)
                    action = 2;
            }

            if (action == 0)
                break;

            if (action < 0)
            {
                SoundPlay(0x209);
                FileSelectUpdateCopyCursor(CURSOR_COPY_POSE_DEFAULT, FILE_SELECT_DATA.copySourceFile);
                FileSelectUpdateCopyArrow(ARROW_COPY_POSE_KILL, FILE_SELECT_DATA.currentFile);
                FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_DESTINATION_DESPAWN);
                FILE_SELECT_DATA.subroutineStage = 3;
                break;
            }

            if (action == 1)
            {
                FileSelectUpdateCopyArrow(ARROW_COPY_POSE_MOVING, FILE_SELECT_DATA.currentFile);
                FileSelectPlayMenuSound(MENU_SOUND_REQUEST_COPY_DELETE_MOVING);
            }
            else if (action == 2)
            {
                FILE_SELECT_DATA.fileSelectData.unk_0 = sFileSelectionData_Empty.unk_0;
                FileSelectUpdateCopyArrow(ARROW_COPY_POSE_COPYING, FILE_SELECT_DATA.currentFile);

                if ((FILE_SELECT_DATA.enabledMenus >> FILE_SELECT_DATA.currentFile) & 1)
                {
                    unk_790cc(0, 3);
                    FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_SPAWN_INIT);
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_COPY_DELETE);
                    FILE_SELECT_DATA.subroutineStage = 5;
                }
                else
                {
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_COPY_CONFIRM);
                    FILE_SELECT_DATA.subroutineStage = 11;
                }
            }
            break;

        case 5:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_SPAWN))
            {
                unk_7e3fc(4, FILE_SELECT_DATA.fileSelectData.unk_0);
                FILE_SELECT_DATA.subroutineStage++;
            }
            break;

        case 6:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    if (FILE_SELECT_DATA.fileSelectData.unk_0 != 0)
                        FILE_SELECT_DATA.subroutineStage = 7;
                    else
                    {
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_COPY_CONFIRM);
                        action = 0x80;
                        FILE_SELECT_DATA.subroutineStage = 11;
                    }
                }
                else if (gChangedInput & KEY_B)
                {
                    FILE_SELECT_DATA.subroutineStage = 7;
                }
                else if (gChangedInput & KEY_LEFT)
                {
                    if (FILE_SELECT_DATA.fileSelectData.unk_0 != 0)
                    {
                        action = 0;
                        FILE_SELECT_DATA.fileSelectData.unk_0 = 0;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_RIGHT)
                {
                    if (FILE_SELECT_DATA.fileSelectData.unk_0 == 0)
                    {
                        action = 1;
                        FILE_SELECT_DATA.fileSelectData.unk_0 = 1;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }
            
            if (action + 1 != 0)
                unk_7e3fc(4, action);
            break;

        case 7:
            SoundPlay(0x209);
            unk_7e3fc(4, 0x81);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN_INIT);
            FILE_SELECT_DATA.subroutineStage++;
            break;

        case 8:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN))
            {
                FileSelectUpdateCopyArrow(ARROW_COPY_POSE_DEFAULT, FILE_SELECT_DATA.currentFile);
                FILE_SELECT_DATA.subroutineStage = 4;
            }
            break;

        case 9:
            if (unk_790cc(1, 4))
            {
                FileSelectUpdateTilemap(0x1C);
                FILE_SELECT_DATA.subroutineStage++;
            }
            break;

        case 10:
            if (gChangedInput & (KEY_A | KEY_B))
            {
                FileSelectUpdateCopyCursor(CURSOR_COPY_POSE_DEFAULT, FILE_SELECT_DATA.copySourceFile);
                FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_DESTINATION_DESPAWN);
                FILE_SELECT_DATA.subroutineStage = 3;
            }
            break;

        case 11:
            if (SramCopyFile(FILE_SELECT_DATA.copySourceFile, FILE_SELECT_DATA.currentFile))
            {
                FILE_SELECT_DATA.unk_3A = 1;
                if (FILE_SELECT_DATA.fileSelectData.unk_0 != 0)
                    FILE_SELECT_DATA.subroutineStage = 14;
                else
                    FILE_SELECT_DATA.subroutineStage = 12;
            }
            break;

        case 12:
            unk_7e3fc(4, 0x81);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN_INIT);
            FILE_SELECT_DATA.subroutineStage++;
            break;

        case 13:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN))
            {
                lbl_case_14:
                FILE_SELECT_DATA.subroutineStage = 14;
            }
            break;

        case 14:
            if (FILE_SELECT_DATA.unk_3A != 0)
            {
                FileSelectDisplaySaveFileHealth(FILE_SELECT_DATA.currentFile);
                FileSelectDisplaySaveFileTimer(FILE_SELECT_DATA.currentFile);
                FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_DATA.currentFile], FILE_SELECT_DATA.currentFile);

                FILE_SELECT_DATA.fileScreenOam[sFileSelect_760b79[FILE_SELECT_DATA.currentFile][1]].exists =
                    gSaveFilesInfo[FILE_SELECT_DATA.currentFile].completedGame ? OAM_ID_CHANGED_FLAG : FALSE;

                FileScreenSetEnabledMenuFlags();
                DmaTransfer(3, (void*)sEwramPointer + 0x800, VRAM_BASE + 0xD800, 0x800, 16);
            }

            FileSelectUpdateCopyCursor(CURSOR_COPY_POSE_COPIED, FILE_SELECT_DATA.copySourceFile);
            FileSelectUpdateCopyArrow(ARROW_COPY_POSE_KILL, FILE_SELECT_DATA.currentFile);
            
            FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_DESPAWN_INIT);
            FILE_SELECT_DATA.subroutineStage++;
            break;

        case 15:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_COPY_DESPAWN))
            {
                unk_790cc(2, UCHAR_MAX);
                ended = TRUE;
            }
            break;
    }

    return ended;
}

/**
 * @brief 797a0 | 3b0 | File erasure main loop
 * 
 * @return u32 bool, ended
 */
static u32 FileSelectEraseFileHandler(void)
{
    u32 ended;
    u32 action;

    ended = FALSE;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);
    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case FILE_SELECT_ERASE_FILE_STAGE_INIT:
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_ERASE_CHOOSE);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_ERASE_CONFIRM);

            FileSelectFindFirstNonEmptyFile(&FILE_SELECT_DATA.eraseFile);
            FileSelectUpdateEraseCursor(CURSOR_ERASE_POSE_DEFAULT, FILE_SELECT_DATA.eraseFile);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_SPAWN_INIT);

            FILE_SELECT_DATA.unk_3A = 0;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_WAIT_FOR_CHOOSE:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_SPAWN))
                FILE_SELECT_DATA.subMenuStage++;
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_UPDATE_CHOOSE:
            action = 0;

            if (gChangedInput)
            {
                if (FileSelectApplyMenuSelectInput(FILE_SELECT_SELECTION_SET_ONLY_FILES, &FILE_SELECT_DATA.eraseFile))
                {
                    action = 1;
                }
                else if (gChangedInput & KEY_B)
                {
#ifdef NON_MATCHING
                    FILE_SELECT_DATA.subMenuStage = 10;
                    break;
#else // !NON_MATCHING
                    goto lbl_case_9;
#endif // NON_MATCHING
                }
                else if (gChangedInput & KEY_A)
                {
                    action = (FILE_SELECT_DATA.enabledMenus >> FILE_SELECT_DATA.eraseFile) & 1 ? 2 : 0;
                }
            }

            if (action == 0)
                break;

            if (action == 1)
            {
                FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CURSOR);
                FileSelectUpdateEraseCursor(CURSOR_ERASE_POSE_MOVING, FILE_SELECT_DATA.eraseFile);
            }
            else if (action == 2)
            {
                SoundPlay(SOUND_SELECTING_FILE_TO_DELETE);
                FileSelectUpdateEraseCursor(CURSOR_ERASE_POSE_SELECTING_FILE, FILE_SELECT_DATA.eraseFile);
                FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_SPAWN_INIT);
                FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_WAIT_FOR_CONFIRM;
            }
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_WAIT_FOR_CONFIRM:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_SPAWN))
            {
                FILE_SELECT_DATA.fileSelectCursors.confirmErase = sFileSelectMenuCursors_Empty.confirmErase;
                unk_7e3fc(3, FILE_SELECT_DATA.fileSelectCursors.confirmErase);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_UPDATE_CONFIRM:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.confirmErase != 0)
                    {
                        FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_CANCEL_CONFIRM;
                    }
                    else
                    {
                        action = 0x80;
                        SoundPlay(SOUND_FILE_DELETE);
                        FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_ERASE_FILE;
                    }
                }
                else if (gChangedInput & KEY_B)
                {
                    FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_CANCEL_CONFIRM;
                }
                else if (gChangedInput & KEY_LEFT)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.confirmErase != 0)
                    {
                        action = 0;
                        FILE_SELECT_DATA.fileSelectCursors.confirmErase = 0;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_RIGHT)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.confirmErase == 0)
                    {
                        action = 1;
                        FILE_SELECT_DATA.fileSelectCursors.confirmErase = 1;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }

            // action never -1, so condition always true
            if (action + 1 != 0)
                unk_7e3fc(3, action);
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_CANCEL_CONFIRM:
            SoundPlay(SOUND_REFUSE_MENU);
            unk_7e3fc(3, 0x81);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN_INIT);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_RESTART_CHOOSE:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN))
            {
                FileSelectUpdateEraseCursor(CURSOR_ERASE_POSE_DEFAULT, FILE_SELECT_DATA.eraseFile);
                FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_UPDATE_CHOOSE;
            }
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_ERASE_FILE:
            if (SramDeleteFile(FILE_SELECT_DATA.eraseFile))
            {
                FILE_SELECT_DATA.unk_3A = 1;
                FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_BEGIN_EXIT;
            }
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_BEGIN_EXIT:
            unk_7e3fc(3, 0x81);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN_INIT);
            FILE_SELECT_DATA.subMenuStage++;
            break;
        
        case FILE_SELECT_ERASE_FILE_STAGE_WAIT_FOR_EXIT:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN))
            {
                lbl_case_9:
                FILE_SELECT_DATA.subMenuStage = FILE_SELECT_ERASE_FILE_STAGE_UPDATE_ERASED_FILE;
            }
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_UPDATE_ERASED_FILE:
            if (FILE_SELECT_DATA.unk_3A != 0)
            {
                FileSelectDisplaySaveFileHealth(FILE_SELECT_DATA.eraseFile);
                FileSelectDisplaySaveFileTimer(FILE_SELECT_DATA.eraseFile);
                FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_DATA.eraseFile], FILE_SELECT_DATA.eraseFile);

                if (!gSaveFilesInfo[FILE_SELECT_DATA.eraseFile].completedGame)
                    FILE_SELECT_DATA.fileScreenOam[sFileSelectFileOamOffsets[FILE_SELECT_DATA.eraseFile][1]].exists = FALSE;
            
                FileScreenSetEnabledMenuFlags();
                DmaTransfer(3, (void*)sEwramPointer + 0x800, VRAM_BASE + 0xD800, 0x800, 16);
            }

            FileSelectUpdateEraseCursor(CURSOR_ERASE_POSE_ERASED, FILE_SELECT_DATA.eraseFile);
            FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_DESPAWN_INIT);

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case FILE_SELECT_ERASE_FILE_STAGE_EXIT:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_ERASE_DESPAWN))
            {
                FileScreenUpdateMessageInfoIdQueue(2, UCHAR_MAX);
                ended = TRUE;
            }
            break;
    }

    return ended;
}

/**
 * @brief 79b50 | 37c | Corrupted file main loop
 * 
 * @return u32 bool, ended
 */
static u32 FileSelectCorruptedFileHandler(void)
{
    u8 done;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
            FILE_SELECT_DATA.corruptMessageFileA = FILE_SELECT_DATA.corruptMessageFileB = FILE_SELECT_DATA.corruptMessageFileC = 0;

            if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT)
                FILE_SELECT_DATA.corruptMessageFileA = FILE_SCREEN_MESSAGE_INFO_ID_FILE_A_CORRUPT_REVERT;
            else if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT_AND_BACKUP)
                FILE_SELECT_DATA.corruptMessageFileA = FILE_SCREEN_MESSAGE_INFO_ID_FILE_A_CORRUPT_ERASE;

            if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT)
                FILE_SELECT_DATA.corruptMessageFileB = FILE_SCREEN_MESSAGE_INFO_ID_FILE_B_CORRUPT_REVERT;
            else if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT_AND_BACKUP)
                FILE_SELECT_DATA.corruptMessageFileB = FILE_SCREEN_MESSAGE_INFO_ID_FILE_B_CORRUPT_ERASE;

            if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT)
                FILE_SELECT_DATA.corruptMessageFileC = FILE_SCREEN_MESSAGE_INFO_ID_FILE_C_CORRUPT_REVERT;
            else if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT_AND_BACKUP)
                FILE_SELECT_DATA.corruptMessageFileC = FILE_SCREEN_MESSAGE_INFO_ID_FILE_C_CORRUPT_ERASE;

            if (FILE_SELECT_DATA.corruptMessageFileA)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SELECT_DATA.corruptMessageFileA);

            if (FILE_SELECT_DATA.corruptMessageFileB)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SELECT_DATA.corruptMessageFileB);

            if (FILE_SELECT_DATA.corruptMessageFileC)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SELECT_DATA.corruptMessageFileC);

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 1:
            done = FALSE;
            if (FILE_SELECT_DATA.corruptMessageFileA)
            {
                if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SELECT_DATA.corruptMessageFileA))
                {
                    FILE_SELECT_DATA.corruptFile = 0;
                    done = TRUE;
                }
            }
            else if (FILE_SELECT_DATA.corruptMessageFileB)
            {
                if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SELECT_DATA.corruptMessageFileB))
                {
                    FILE_SELECT_DATA.corruptFile = 1;
                    done = TRUE;
                }
            }
            else if (FILE_SELECT_DATA.corruptMessageFileC)
            {
                if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SELECT_DATA.corruptMessageFileC))
                {
                    FILE_SELECT_DATA.corruptFile = 2;
                    done = TRUE;
                }
            }
            else
            {
                FILE_SELECT_DATA.subMenuStage = 10;
                break;
            }

            if (!done)
                break;

            FileSelectUpdateTilemap(TILEMAP_REQUEST_38);
            FILE_SELECT_DATA.subMenuStage = 2;
            break;

        case 2:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_39))
            {
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 3:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(.5f))
                FILE_SELECT_DATA.subMenuStage++;
            break;

        case 4:
            if (gChangedInput & KEY_A)
            {
                gUnk_3000C20 = 0;

                gMostRecentSaveFile = FILE_SELECT_DATA.corruptFile;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 5:
            if (unk_fbc(0x0))
            {
                gUnk_3000C20 = 0;

                if (gSaveFilesInfo[gMostRecentSaveFile].corruptionFlag == CORRUPTED_FILE_FLAG_CURRENT)
                {
                    gSaveFilesInfo[gMostRecentSaveFile].corruptionFlag = 0;
                    FILE_SELECT_DATA.subMenuStage = 7;
                    break;
                }
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 6:
            if (unk_fbc(0x1))
            {
                gUnk_3000C20 = 0;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 7:
            FileSelectDisplaySaveFileHealth(FILE_SELECT_DATA.corruptFile);
            FileSelectDisplaySaveFileTimer(FILE_SELECT_DATA.corruptFile);
            FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_DATA.corruptFile], FILE_SELECT_DATA.corruptFile);

            FileScreenSetEnabledMenuFlags();

            if (FILE_SELECT_DATA.corruptFile == 0)
                FILE_SELECT_DATA.corruptMessageFileA = 0;
            else if (FILE_SELECT_DATA.corruptFile == 1)
                FILE_SELECT_DATA.corruptMessageFileB = 0;
            else
                FILE_SELECT_DATA.corruptMessageFileC = 0;

            DmaTransfer(3, (void*)sEwramPointer + 0x800, VRAM_BASE + 0xD800, 0x800, 16);
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_DATA.corruptFile + FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = FALSE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 8:
            FileSelectUpdateTilemap(TILEMAP_REQUEST_40);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 9:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_41))
                FILE_SELECT_DATA.subMenuStage = 1;
            break;

        case 10:
            FileScreenUpdateMessageInfoIdQueue(2, UCHAR_MAX);
            gMostRecentSaveFile = FILE_SELECT_DATA.unk_24;
            FILE_SELECT_DATA.subMenuStage++;
            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 11:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(.25f) + 1 * DELTA_TIME)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].notDrawn = FALSE;
                FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.unk_24);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

/**
 * @brief 79ecc | ec | To document
 * 
 */
static void unk_79ecc(void)
{
    DmaTransfer(3, (void*)sEwramPointer + 0x5100, VRAM_BASE + 0xF000, 0x800, 16);

    FILE_SELECT_DATA.bg2cnt = FILE_SELECT_DATA.unk_20;
    FILE_SELECT_DATA.dispcnt |= (DCNT_BG2 | DCNT_OBJ);

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].objMode = 1;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].objMode = 1;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER ].objMode = 1;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].objMode = 1;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = TRUE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = TRUE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = TRUE;

    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].notDrawn = TRUE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].notDrawn = TRUE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER ].notDrawn = TRUE;
    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].notDrawn = TRUE;
}

static u16 sOptionsOptionsTilemapOffsets[OPTION_COUNT] = {
    [OPTION_NONE] = 0,
    [OPTION_STEREO_SELECT] = 0,
    [OPTION_SOUND_TEST] = BLOCK_SIZE * 10,
    [OPTION_TIME_ATTACK] = BLOCK_SIZE * 11,
    [OPTION_GALLERY] = BLOCK_SIZE * 12,
    [OPTION_FUSION_GALLERY] = BLOCK_SIZE * 14,
    [OPTION_FUSION_LINK] = BLOCK_SIZE * 13,
    [OPTION_NES_METROID] = BLOCK_SIZE * 15
};

/**
 * @brief 79fb8 | 1ec | Sets up the options tile table and determines which options are unlocked
 * 
 */
static void OptionsSetupTiletable(void)
{
    u8 i;
    s32 j;
    u8* pOptions;
    u16* dst;

    // Decomp tile table
#ifdef REGION_EU
    CallLZ77UncompWram(sFileSelectMenuTileTable, (void*)sEwramPointer + 0x5100);
#else // !REGION_EU
    CallLZ77UncompWram(sFileSelectOptionsTileTable, (void*)sEwramPointer + 0x5100);
#endif // REGION_EU

    // Clear all the options
    for (i = 0; i < ARRAY_SIZE(FILE_SELECT_DATA.optionsUnlocked); i++)
        FILE_SELECT_DATA.optionsUnlocked[i] = OPTION_NONE;

    pOptions = FILE_SELECT_DATA.optionsUnlocked;
    // Allow stereo select by default
    *pOptions++ = OPTION_STEREO_SELECT;

    for (i = OPTION_SOUND_TEST; i <= ARRAY_SIZE(FILE_SELECT_DATA.optionsUnlocked); i++)
    {
        switch (i)
        {
            case OPTION_SOUND_TEST:
                // Check beat the game in hard mode
                if (gFileScreenOptionsUnlocked.soundTestAndOrigMetroid & (1 << DIFF_HARD))
                    *pOptions++ = i;
                break;

            case OPTION_TIME_ATTACK:
                FILE_SELECT_DATA.timeAttackRecordFlags = 0;
                FILE_SELECT_DATA.timeAttack100Only = FALSE;

                // Check has 100% record
                if (gTimeAttackRecord.igt100.hours != UCHAR_MAX)
                {
                    FILE_SELECT_DATA.timeAttackRecordFlags |= 2;
                    FILE_SELECT_DATA.timeAttack100Only = TRUE;
                }

                // Check has any% record
                if (gTimeAttackRecord.igt.hours != UCHAR_MAX)
                {
                    FILE_SELECT_DATA.timeAttackRecordFlags |= 1;
                    FILE_SELECT_DATA.timeAttack100Only = FALSE;
                }

                if (FILE_SELECT_DATA.timeAttackRecordFlags)
                    *pOptions++ = i;
                break;

            case OPTION_GALLERY:
                // Check has any image
                if (gFileScreenOptionsUnlocked.galleryImages == 0)
                    break;

                *pOptions++ = i;

                if (gFileScreenOptionsUnlocked.galleryImages == UCHAR_MAX)
                {
                    // All the images are unlocked, change text palette to yellow
                    dst = (u16*)((void*)sEwramPointer + 0x5100) + sOptionsOptionsTilemapOffsets[OPTION_GALLERY];
                    j = 0;
                    while (j < 64)
                    {
                        *dst = (*dst & 0xFFF) | 0xE000;
                        j++;
                        dst++;
                    }
                }
                break;

            case OPTION_FUSION_GALLERY:
                // Check has any image
                if (gFileScreenOptionsUnlocked.fusionGalleryImages)
                    *pOptions = i;
                break;

            case OPTION_FUSION_LINK:
                if (*pOptions == OPTION_FUSION_GALLERY)
                    pOptions++; // Has the fusion gallery unlocked, don't enable link
                else
                    *pOptions++ = i; // Doesn't have the fusion gallery unlocked, enable link
                break;

            case OPTION_NES_METROID:
                // Check has either any image beat the game in any difficulty
                if (gFileScreenOptionsUnlocked.galleryImages)
                    *pOptions++ = i;
                else if (gFileScreenOptionsUnlocked.soundTestAndOrigMetroid)
                    *pOptions++ = i;
                break;
        }

        // Check not overflowing
        if (pOptions >= FILE_SELECT_DATA.optionsUnlocked + ARRAY_SIZE(FILE_SELECT_DATA.optionsUnlocked) - 1)
            break;
    }

    // Setup tiletable for the whole screen
    for (i = 0; FILE_SELECT_DATA.optionsUnlocked[i] != OPTION_NONE; i++)
    {
        if (sOptionsOptionsTilemapOffsets[FILE_SELECT_DATA.optionsUnlocked[i]] != 0)
        {
            DmaTransfer(3, (u16*)((void*)sEwramPointer + 0x5100) + sOptionsOptionsTilemapOffsets[FILE_SELECT_DATA.optionsUnlocked[i]],
                (void*)sEwramPointer + 0x5200 + i * 0x80, 0x80, 16);
        }
    }
}

/**
 * @brief 7a1a4 | 118 | Copies the time attack best times to RAM
 * 
 */
static void FileSelectCopyTimeAttackTime(void)
{
    s32 value;

    if (FILE_SELECT_DATA.timeAttackRecordFlags == 0)
        return;

    unk_7f60c(FILE_SELECT_DATA.timeAttackPassword);

    if (gTimeAttackRecord.igt.hours < 99)
        value = gTimeAttackRecord.igt.hours;
    else
        value = 99;
    FILE_SELECT_DATA.timeAttackBestTimes[0][0] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[0][1] = value % 10;

    if (gTimeAttackRecord.igt.minutes < 59)
        value = gTimeAttackRecord.igt.minutes;
    else
        value = 59;
    FILE_SELECT_DATA.timeAttackBestTimes[0][2] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[0][3] = value % 10;

    if (gTimeAttackRecord.igt.seconds < 59)
        value = gTimeAttackRecord.igt.seconds;
    else
        value = 59;
    FILE_SELECT_DATA.timeAttackBestTimes[0][4] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[0][5] = value % 10;

    if (gTimeAttackRecord.igt100.hours < 99)
        value = gTimeAttackRecord.igt100.hours;
    else
        value = 99;
    FILE_SELECT_DATA.timeAttackBestTimes[1][0] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[1][1] = value % 10;

    if (gTimeAttackRecord.igt100.minutes < 59)
        value = gTimeAttackRecord.igt100.minutes;
    else
        value = 59;
    FILE_SELECT_DATA.timeAttackBestTimes[1][2] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[1][3] = value % 10;

    if (gTimeAttackRecord.igt100.seconds < 59)
        value = gTimeAttackRecord.igt100.seconds;
    else
        value = 59;
    FILE_SELECT_DATA.timeAttackBestTimes[1][4] = value / 10;
    FILE_SELECT_DATA.timeAttackBestTimes[1][5] = value % 10;
}

/**
 * @brief 7a2bc | 528 | Handles the transition between the file select screen and the options screen
 * 
 * @param leavingOptions Leaving options flag
 * @return u8 bool, ended
 */
static u8 FileSelectOptionTransition(u8 leavingOptions)
{
    u16 bgPos;
    u32 fadeEnded;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
            if (leavingOptions)
                FILE_SELECT_DATA.subMenuStage = 7; // Options -> file select
            else
                FILE_SELECT_DATA.subMenuStage = 1; // File select -> options
            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 1:
            if (FILE_SELECT_DATA.subMenuTimer < CONVERT_SECONDS(1.f / 6))
                break;
            
            gWrittenToBldalpha_H = 0;
            gWrittenToBldalpha_L = BLDALPHA_MAX_VALUE;

            FILE_SELECT_DATA.bldcnt = BLDCNT_BG1_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].objMode = 1;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].objMode = 1;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].objMode = 1;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].objMode = 1;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = TRUE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = TRUE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = TRUE;

            BitFill(3, 0, VRAM_BASE + 0xE000, 0x800, 16);
            DmaTransfer(3, (void*)sEwramPointer + 0x5100, VRAM_BASE + 0xE000, 0xC0, 16);

            FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
            
            gBg0HOFS_NonGameplay = BLOCK_SIZE * 32;
            gBg0VOFS_NonGameplay = BLOCK_SIZE * 24;

            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 2:
            // Update background fade
            fadeEnded = TRUE;

            if (gWrittenToBldalpha_L != 0)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_L - 2 < 0)
                    gWrittenToBldalpha_L = 0;
                else
                    gWrittenToBldalpha_L -= 2;
            }

            if (gWrittenToBldalpha_H != 16)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_H + 2 > 16)
                    gWrittenToBldalpha_H = 16;
                else
                    gWrittenToBldalpha_H += 2;
            }

            if (!fadeEnded)
                break;

            // Hide objects
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].notDrawn = TRUE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].notDrawn = TRUE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].notDrawn = TRUE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].notDrawn = TRUE;

            // Remove BG1
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 3:
            // Scroll BG0 up until it's all the way to the top
            if (gBg0VOFS_NonGameplay < BLOCK_SIZE * 32)
            {
                gBg0VOFS_NonGameplay += BLOCK_SIZE;
                if (gBg0VOFS_NonGameplay < BLOCK_SIZE * 32)
                    break;
            }

            gBg0VOFS_NonGameplay = BLOCK_SIZE * 32;

            // Initializes options
            OptionsSetupTiletable();
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 4:
            unk_79ecc();
            FILE_SELECT_DATA.bldcnt = BLDCNT_BG2_FIRST_TARGET_PIXEL | BLDCNT_OBJ_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET; 
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 5:
            // Update background fade
            fadeEnded = TRUE;

            if (gWrittenToBldalpha_L != 16)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_L + 2 > 16)
                    gWrittenToBldalpha_L = 16;
                else
                    gWrittenToBldalpha_L += 2;
            }

            if (gWrittenToBldalpha_H != 0)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_H - 2 < 0)
                    gWrittenToBldalpha_H = 0;
                else
                    gWrittenToBldalpha_H -= 2;
            }

            if (!fadeEnded)
                break;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 6:
            // Remove BG0 and BG1
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
            FILE_SELECT_DATA.bldcnt = 0;
            return TRUE;


        case 7:
            // todo: seconds or delta time?
            if (FILE_SELECT_DATA.subMenuTimer < 4)
                break;

            BitFill(3, 0, VRAM_BASE + 0xE000, 0x800, 16);
            DmaTransfer(3, (void*)sEwramPointer + 0x5100, VRAM_BASE + 0xE000, 0xC0, 16);

            gBg0HOFS_NonGameplay = gBg0VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;

            FILE_SELECT_DATA.bldcnt = BLDCNT_BG2_FIRST_TARGET_PIXEL | BLDCNT_OBJ_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET; 

            gWrittenToBldalpha_H = 0;
            gWrittenToBldalpha_L = 16;

            gBg1HOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;
            gBg1VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;

            FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 8:
            // Update background fade
            fadeEnded = TRUE;

            if (gWrittenToBldalpha_L != 0)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_L - 2 < 0)
                    gWrittenToBldalpha_L = 0;
                else
                    gWrittenToBldalpha_L -= 2;
            }

            if (gWrittenToBldalpha_H != 16)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_H + 2 > 16)
                    gWrittenToBldalpha_H = 16;
                else
                    gWrittenToBldalpha_H += 2;
            }

            if (!fadeEnded)
                break;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 9:
            // Scroll BG0 down until the "options" text is at the bottom
            if (gBg0VOFS_NonGameplay > BLOCK_SIZE * 24)
            {
                gBg0VOFS_NonGameplay -= BLOCK_SIZE;
                if (gBg0VOFS_NonGameplay > BLOCK_SIZE * 24)
                    break;
            }

            gBg0VOFS_NonGameplay = BLOCK_SIZE * 24;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 10:
            // Set cursor on the options position
            FILE_SELECT_DATA.fileSelectCursorPosition = FILE_SELECT_CURSOR_POSITION_OPTIONS;

            // Hide BG2, display BG1 and OBJ, setup BG1 control
            FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_18;
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG2;
            FILE_SELECT_DATA.dispcnt |= (DCNT_BG1 | DCNT_OBJ);

            FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.fileSelectCursorPosition);

            // Display objects
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].notDrawn = FALSE;

            FILE_SELECT_DATA.bldcnt = BLDCNT_BG1_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 11:
            // Update background fade
            fadeEnded = TRUE;

            if (gWrittenToBldalpha_H != 0)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_H - 2 < 0)
                    gWrittenToBldalpha_H = 0;
                else
                    gWrittenToBldalpha_H -= 2;
            }

            if (gWrittenToBldalpha_L != 16)
            {
                fadeEnded = FALSE;
                if (gWrittenToBldalpha_L + 2 > 16)
                    gWrittenToBldalpha_L = 16;
                else
                    gWrittenToBldalpha_L += 2;
            }

            if (!fadeEnded)
                break;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].objMode = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].objMode = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].objMode = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].objMode = FALSE;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = FALSE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 12:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;

            gBg0HOFS_NonGameplay = BLOCK_SIZE * 32;
            gBg0VOFS_NonGameplay = BLOCK_SIZE * 32;

            gSubGameMode2 = 0;
            return TRUE;
    }

    return FALSE;
}

static struct OptionsSubMenuInfo sOptionsSubMenuInfo[OPTION_COUNT + 1] = {
    [OPTION_NONE] = {
        .pFunction = OptionsSubMenu_Empty,
        .gameMode = 0
    },
    [OPTION_STEREO_SELECT] = {
        .pFunction = OptionsStereoHandler,
        .gameMode = 0
    },
    [OPTION_SOUND_TEST] = {
        .pFunction = OptionsSoundTestHandler,
        .gameMode = 0
    },
    [OPTION_TIME_ATTACK] = {
        .pFunction = OptionsTimeAttackRecordsHandler,
        .gameMode = 0
    },
    [OPTION_GALLERY] = {
        .pFunction = OptionsGalleryHandler,
        .gameMode = 5
    },
    [OPTION_FUSION_GALLERY] = {
        .pFunction = OptionsMetroidFusionLinkHandler,
        .gameMode = 4
    },
    [OPTION_FUSION_LINK] = {
        .pFunction = OptionsMetroidFusionLinkHandler,
        .gameMode = 0
    },
    [OPTION_NES_METROID] = {
        .pFunction = OptionsNesMetroidHandler,
        .gameMode = 0
    },
    [8] = {
        .pFunction = OptionsSubMenu_Empty,
        .gameMode = 0
    }
};

/**
 * @brief 7a7e4 | 248 | Main loop for the options
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsHandler(void)
{
    u8 result;

    switch (FILE_SELECT_DATA.currentSubMenu)
    {
        case 0:
            // Check in bounds/not on a non existant option
            if (FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected] == 0 || gOptionsOptionSelected < 0 || (u8)gOptionsOptionSelected > 6)
                gOptionsOptionSelected = 0;

            // Update cursor
            OptionsUpdateCursor(CURSOR_OPTIONS_POSE_MOVING);
            
            // Check for sound test
            OptionsSoundTestCheckNotAlreadyPlaying();
            FILE_SELECT_DATA.dispcnt |= (DCNT_BG2 | DCNT_OBJ);
            FILE_SELECT_DATA.currentSubMenu = 2;
            break;

        case 1:
            if (FILE_SELECT_DATA.timer > CONVERT_SECONDS(.25f) + 1 * DELTA_TIME)
            {
                OptionsUpdateCursor(CURSOR_OPTIONS_POSE_MOVING);
                FILE_SELECT_DATA.currentSubMenu = 2;
            }
            break;

        case 2:
#ifdef REGION_EU
            CheckForMaintainedInput(MAINTAINED_INPUT_SPEED_FAST);
#else // !REGION_EU
            CheckForMaintainedInput();
#endif // REGION_EU

            if (!gChangedInput)
                break;

            if (gChangedInput & KEY_B)
            {
                // Set exiting 
                SoundPlay(SOUND_REFUSE_MENU);
                FILE_SELECT_DATA.currentSubMenu = 4;
            }
            else if (gChangedInput & KEY_A)
            {
                // Check option exists
                if (FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected])
                {
                    // Update cursor
                    OptionsUpdateCursor(CURSOR_OPTIONS_POSE_SELECTING);
                    
                    // Setup sub menu info
                    FILE_SELECT_DATA.subMenuStage = 0;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.unk_68 = 0;
                    FILE_SELECT_DATA.unk_6A = 0;

                    FileSelectResetIOTransferInfo();
                    FILE_SELECT_DATA.currentSubMenu = 3;
                }
            }
            else if (gChangedInput & KEY_UP)
            {
                // Check not already on top
                if (gOptionsOptionSelected != 0)
                {
                    gOptionsOptionSelected--;
                    // Update cursor
                    OptionsUpdateCursor(CURSOR_OPTIONS_POSE_ENTERING);
                }
            }
            else if (gChangedInput & KEY_DOWN)
            {
                // Check not already on bottom (has option below)
                if (FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected + 1])
                {
                    gOptionsOptionSelected++;
                    // Update cursor
                    OptionsUpdateCursor(CURSOR_OPTIONS_POSE_ENTERING);
                }
            }
            break;

        case 3:
            // Call main loop of currently selected option
            result = sOptionsSubMenuInfo[FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected]].pFunction();
            if (result == 0)
                break;

            if (result == 2)
                gSubGameMode2 = sOptionsSubMenuInfo[FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected]].gameMode;
            else
                gSubGameMode2 = 0;

            if (gSubGameMode2)
            {
                FILE_SELECT_DATA.currentSubMenu = 5;
            }
            else
            {
                OptionsUpdateCursor(CURSOR_OPTIONS_POSE_DESELECTING);
                FILE_SELECT_DATA.currentSubMenu = 2;
            }
            FILE_SELECT_DATA.timer = 0;
            break;

        case 4:
            FILE_SELECT_DATA.soundTestId = 0;        
            CheckReplayFileSelectMusic(CONVERT_SECONDS(1.f / 6));
            gSubGameMode2 = 0;
            return TRUE;

        case 5:
            if (FILE_SELECT_DATA.timer > CONVERT_SECONDS(1.f / 6))
                return TRUE;
    }

    FileScreenProcessText();
    return FALSE;
}

/**
 * @brief 7aa2c | 48 | Resets the IO transfer info
 * 
 */
static void FileSelectResetIOTransferInfo(void)
{
    switch (FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected])
    {
        case OPTION_FUSION_GALLERY:
        case OPTION_FUSION_LINK:
            gIoTransferInfo = sIoTransferInfo_Empty;
            gIoTransferInfo.pFunction = FileSelectProcessOam;
    }
}

/**
 * @brief 7aa74 | 150 | Main loop for the NES metroid
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsNesMetroidHandler(void)
{
    u8 i;
    NesEmuFunc_T func;
    void* entryPoint;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
            if (gChangedInput & KEY_B)
            {
                // Exit
                FILE_SELECT_DATA.subMenuTimer = 0;
                FILE_SELECT_DATA.subMenuStage = 0;
                return TRUE;
            }

            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(1.f / 6))
            {
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 1:
            // Verify data integrity
            for (i = 0; i < ARRAY_SIZE(sNesMetroidDataCheck); i++)
            {
                if (sNesMetroidData_Text[i] != sNesMetroidDataCheck[i])
                    return TRUE;
            }
            FILE_SELECT_DATA.subMenuStage++;
            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 2:
            // Sound stuff
            RestartSound();

            // Start screen fade
            FILE_SELECT_DATA.bldcnt = BLDCNT_SCREEN_FIRST_TARGET | BLDCNT_BRIGHTNESS_DECREASE_EFFECT;
            gWrittenToBldy_NonGameplay = 0;

            FILE_SELECT_DATA.subMenuStage++;
            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 3:
            // Apply fade
            gWrittenToBldy_NonGameplay += 2;
            if (gWrittenToBldy_NonGameplay >= BLDY_MAX_VALUE)
            {
                SET_BACKDROP_COLOR(COLOR_BLACK);

                FILE_SELECT_DATA.bldcnt = 0;
                gWrittenToBldy_NonGameplay = 0;

                // Black out screen
                FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3 | DCNT_OBJ);
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 4:
            WRITE_16(REG_IME, FALSE);
            WRITE_16(REG_IF, USHORT_MAX);

            // Give control to some sort of bootloader?
            // Signature : void Func_T(void*)
            entryPoint = ROM_BASE;
            func = (NesEmuFunc_T)&sNesEmuBootLoader;
            func(entryPoint);
            break;
    }

    return FALSE;
}

/**
 * @brief 7abc4 | 34 | Main loop for 
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsSubMenu_Empty(void)
{
    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    if (gChangedInput && gChangedInput & KEY_B)
        return TRUE;
    else
        return FALSE;
}

/**
 * @brief 7abf8 | 5c | Main loop for the gallery
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsGalleryHandler(void)
{
    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    if (gChangedInput & KEY_B)
    {
        FILE_SELECT_DATA.subMenuTimer = 0;
        FILE_SELECT_DATA.subMenuStage = 0;
        return TRUE;
    }
    
    if (gChangedInput & KEY_A || FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(1.f / 6))
    {
        FILE_SELECT_DATA.subMenuTimer = 0;
        FILE_SELECT_DATA.subMenuStage = 0;
        return TRUE + 1;
    }

    return FALSE;
}

/**
 * @brief 7ac54 | bc | Main loop for the stereo selection
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsStereoHandler(void)
{
    u8 updatedStereo;

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
            // Update OAM to have the animation
            OptionsUpdateStereoOam(STEREO_UPDATE_FLAGS_ACTIVE);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 1:
            updatedStereo = FALSE;
            if (!gChangedInput)
                break;

            if (gChangedInput & (KEY_A | KEY_B))
            {
                // Exit menu
                SoundPlay(SOUND_ACCEPT_CONFIRM_MENU);
                FILE_SELECT_DATA.subMenuStage++;
                break;
            }

            // Handle input
            if (!gStereoFlag)
            {
                if (gChangedInput & KEY_RIGHT)
                {
                    gStereoFlag = TRUE;
                    updatedStereo = TRUE;
                }
            }
            else
            {
                if (gChangedInput & KEY_LEFT)
                {
                    gStereoFlag = FALSE;
                    updatedStereo = TRUE;
                }
            }

            if (!updatedStereo)
                break;

            // Update OAM and apply new choice
            SoundPlay(SOUND_SUB_MENU_CURSOR);
            OptionsUpdateStereoOam(STEREO_UPDATE_FLAGS_ACTIVE);
            FileSelectApplyStereo();
            break;

        case 2:
            // Remove animated symbols
            OptionsUpdateStereoOam(STEREO_UPDATE_FLAGS_IDLE);
            // Write to SRAM
            SramWrite_SoundMode();
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 3:
            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage = 0;
            return TRUE;
    }

    return FALSE;
}

/**
 * @brief 7ad10 | 2f8 | Main loop for the sound test
 * 
 * @return u8 bool, leaving
 */
static u8 OptionsSoundTestHandler(void)
{
    s32 action;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
            if (OptionsSoundTestCheckNotAlreadyPlaying())
                PlaySoundTest(sSoundTestSoundIds[FILE_SELECT_DATA.soundTestId]);
            
            // Draw number
            OptionsSoundTestUpdateIdGfx();
            SoundPlay(SOUND_OPEN_SUB_MENU);
            
            // Spawn panel
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL], OPTIONS_OAM_ID_SMALL_PANEL_OPEN);

            // Sync position
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].xPosition =
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].xPosition;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].yPosition =
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID].yPosition;

#ifdef REGION_EU
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].priority = BGCNT_LOW_MID_PRIORITY;
#else // !REGION_EU
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].priority = BGCNT_LOW_PRIORITY;
#endif // REGION_EU
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 1:
            // Wait for panel spawn to end
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].ended)
            {
                // Spawn the arrows and the numbers
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID],
                    OPTIONS_OAM_ID_SOUND_TEST_ID);
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW],
                    OPTIONS_OAM_ID_LEFT_ARROW_IDLE);
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW],
                    OPTIONS_OAM_ID_RIGHT_ARROW_IDLE);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 2:
            action = 0;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_B)
                {
                    // Exit
                    FILE_SELECT_DATA.subMenuStage = 6;
                }
                else if (gChangedInput & KEY_A)
                {
                    // Replay current music
                    ReplaySoundTest(sSoundTestSoundIds[FILE_SELECT_DATA.soundTestId]);
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.subMenuStage = 5;
                }
                else if (gChangedInput & KEY_RIGHT)
                {
                    // Try increment number
                    if (FILE_SELECT_DATA.soundTestId < ARRAY_SIZE(sSoundTestSoundIds) - 1)
                        FILE_SELECT_DATA.soundTestId++;
                    else
                        FILE_SELECT_DATA.soundTestId = 1;

                    // Set right arrow to move
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW],
                        OPTIONS_OAM_ID_RIGHT_ARROW_MOVING);
                    FILE_SELECT_DATA.subMenuStage = 3;
                    action = 1;
                }
                else if (gChangedInput & KEY_LEFT)
                {
                    // Try decrement number
                    if (FILE_SELECT_DATA.soundTestId > 1)
                        FILE_SELECT_DATA.soundTestId--;
                    else
                        FILE_SELECT_DATA.soundTestId = ARRAY_SIZE(sSoundTestSoundIds) - 1;

                    // Set left arrow to move
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW],
                        OPTIONS_OAM_ID_LEFT_ARROW_MOVING);
                    FILE_SELECT_DATA.subMenuStage = 4;
                    action = -1;
                }
            }

            if (action)
            {
                SoundPlay(SOUND_SOUND_TEST_CURSOR_MOVING);
                // Play music and update number
                PlaySoundTest(sSoundTestSoundIds[FILE_SELECT_DATA.soundTestId]);
                OptionsSoundTestUpdateIdGfx();
            }
            break;

        case 3:
            // Wait for moving animation to end
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].oamId != OPTIONS_OAM_ID_RIGHT_ARROW_MOVING)
            {
                // Sync with other arrow (makes them blink at the same time)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].animationDurationCounter =
                    FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].animationDurationCounter;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].currentAnimationFrame =
                    FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].currentAnimationFrame;

                FILE_SELECT_DATA.subMenuStage = 2;
            }
            break;

        case 4:
            // Wait for moving animation to end
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].oamId != OPTIONS_OAM_ID_LEFT_ARROW_MOVING)
            {
                // Sync with other arrow (makes them blink at the same time)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].animationDurationCounter =
                    FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].animationDurationCounter;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW].currentAnimationFrame =
                    FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW].currentAnimationFrame;

                FILE_SELECT_DATA.subMenuStage = 2;
            }
            break;

        case 5:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(.5f))
                FILE_SELECT_DATA.subMenuStage = 2;
            break;

        case 6:
            SoundPlay(SOUND_CLOSE_SUB_MENU);

            // Kill number and arrows
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_ID], 0);
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_LEFT_ARROW], 0);
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_RIGHT_ARROW], 0);

            // Start panel disappearing animation
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].oamId++;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 7:
            // Wait for panel to disappear
            if (!FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_SOUND_TEST_PANEL].exists)
                FILE_SELECT_DATA.subMenuStage++;
            break;

        case 8:
            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage = 0;
            return TRUE;
    }

    return FALSE;
}

/**
 * @brief 7b008 | 8c | Checks if the current music isn't a sound test playing
 * 
 * @return u32 bool, not currently playing
 */
static u32 OptionsSoundTestCheckNotAlreadyPlaying(void)
{
    u32 notCurrentlyPlaying;

    notCurrentlyPlaying = FALSE;

    if (gMusicInfo.musicTrack != sSoundTestSoundIds[FILE_SELECT_DATA.soundTestId])
    {
        notCurrentlyPlaying = TRUE;

        // Start from the end
        FILE_SELECT_DATA.soundTestId = ARRAY_SIZE(sSoundTestSoundIds) - 1;

        // Try to find current music in the array
        while (FILE_SELECT_DATA.soundTestId != 0 && gMusicInfo.musicTrack != sSoundTestSoundIds[FILE_SELECT_DATA.soundTestId])
            FILE_SELECT_DATA.soundTestId--;
    }

    // Couldn't find music, set first id
    if (FILE_SELECT_DATA.soundTestId == 0)
        FILE_SELECT_DATA.soundTestId = 1;

    return notCurrentlyPlaying;
}

/**
 * @brief 7b094 | b0 | Updates the number graphics of the sound test id
 * 
 */
static void OptionsSoundTestUpdateIdGfx(void)
{
    u32 number;
    u32 offset;

    number = FILE_SELECT_DATA.soundTestId / 10 * 32;
    offset = 0xA00;
    DmaTransfer(3, &sCharactersGfx[number + offset], VRAM_BASE + 0x103C0, 32, 16);
    DmaTransfer(3, &sCharactersGfx[number + offset + 0x400], VRAM_BASE + 0x107C0, 32, 16);

    offset = 0;
    number = FILE_SELECT_DATA.soundTestId % 10 * 32;
    offset = 0xA00;
    DmaTransfer(3, &sCharactersGfx[number + offset], VRAM_BASE + 0x103E0, 32, 16);
    DmaTransfer(3, &sCharactersGfx[number + offset + 0x400], VRAM_BASE + 0x107E0, 32, 16);
}

/**
 * @brief 7b144 | 5d8 | Main loop for the time attack records
 * 
 * @return u8 bool, ended
 */
static u8 OptionsTimeAttackRecordsHandler(void)
{
    u32 action;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case OPTIONS_TIME_ATTACK_STAGE_0:
            if (FILE_SELECT_DATA.timeAttackRecordFlags & 1)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_ID_PASSWORD);
            
            if (FILE_SELECT_DATA.timeAttackRecordFlags & 2)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_ID_PASSWORD2);

            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].boundBackground = 1;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].priority = BGCNT_LOW_MID_PRIORITY;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].yPosition = BLOCK_SIZE * 3;

            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].boundBackground = 0;
#ifdef REGION_EU
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].priority = BGCNT_LOW_MID_PRIORITY;
#else // !REGION_EU
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].priority = BGCNT_HIGH_MID_PRIORITY;
#endif // REGION_EU
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].xPosition = BLOCK_SIZE * 4;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].yPosition = BLOCK_SIZE * 2;

            gBg1HOFS_NonGameplay = BLOCK_SIZE * 27 + QUARTER_BLOCK_SIZE + 8;
            gBg1VOFS_NonGameplay = BLOCK_SIZE * 28;
            gBg0HOFS_NonGameplay = BLOCK_SIZE * 25 + HALF_BLOCK_SIZE;
            gBg0VOFS_NonGameplay = BLOCK_SIZE * 32;

            DmaTransfer(3, (void*)sEwramPointer + 0x4800, VRAM_BASE + 0xE000, 0x300, 16);

            FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
            FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;

#ifndef REGION_US_BETA
            if (FILE_SELECT_DATA.timeAttackRecordFlags & 1 && FILE_SELECT_DATA.timeAttackRecordFlags & 2)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW], OPTIONS_OAM_ID_LEFT_ARROW_IDLE);
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].boundBackground = 0;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].xPosition = BLOCK_SIZE + 0x1C;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].yPosition = BLOCK_SIZE + HALF_BLOCK_SIZE;

                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW], OPTIONS_OAM_ID_RIGHT_ARROW_IDLE);
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].boundBackground = 0;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].xPosition = BLOCK_SIZE * 6 + HALF_BLOCK_SIZE + 4;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].yPosition = BLOCK_SIZE + HALF_BLOCK_SIZE;
            }
            else
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW], 0);
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW], 0);
            }

            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].notDrawn = TRUE;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].notDrawn = TRUE;
#endif // !REGION_US_BETA

            FILE_SELECT_DATA.subMenuStage++;
            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_1:
            if (!FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_ID_PASSWORD))
                break;
            
            if (!FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_ID_PASSWORD2))
                break;

            unk_7b854();
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_2:
            if (FILE_SELECT_DATA.timeAttackRecordFlags & 1)
            {
                OptionsTimeAttackLoadPassword(0|0);
                OptionsTimeAttackLoadPassword(0|1);
            }
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_4;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_3:
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_4:
            if (FILE_SELECT_DATA.timeAttackRecordFlags & 2)
            {
                OptionsTimeAttackLoadPassword(2|0);
                OptionsTimeAttackLoadPassword(2|1);
            }
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_6;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_5:
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_6:
#ifndef REGION_US_BETA
            OptionsTimeAttackLoadBestTimeMessage();
#endif // !REGION_US_BETA
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL], OPTIONS_OAM_ID_HUGE_PANEL_OPEN);
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL], OPTIONS_OAM_ID_LARGE_PANEL_OPEN);

            SoundPlay(SOUND_OPEN_SUB_MENU);
#ifdef REGION_US_BETA
            FILE_SELECT_DATA.subMenuStage++;
#else // !REGION_US_BETA
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_7;
#endif // REGION_US_BETA
            break;
        
#ifdef REGION_US_BETA
        case OPTIONS_TIME_ATTACK_STAGE_6B:
            if (FILE_SELECT_DATA.timeAttack100Only)
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME_100);
            else
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME);
            FILE_SELECT_DATA.subMenuStage++;
            break;
#endif // REGION_US_BETA

        case OPTIONS_TIME_ATTACK_STAGE_7:
            action = FALSE;

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].ended &&
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].ended)
            {
                if (FILE_SELECT_DATA.timeAttack100Only)
                {
                    if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME_100))
                    {
                        DmaTransfer(3, (void*)sEwramPointer + 0x4E00, VRAM_BASE + 0xE800, 0x300, 16);
                        action = TRUE;
                    }
                }
                else
                {
                    if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME))
                    {
                        action = TRUE;
                        DmaTransfer(3, (void*)sEwramPointer + 0x4B00, VRAM_BASE + 0xE800, 0x300, 16);
                    }
                }
            }

            if (!action)
                break;

#ifndef REGION_US_BETA
            if (FILE_SELECT_DATA.timeAttack100Only)
            {
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].notDrawn = FALSE;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].notDrawn = TRUE;
            }
            else
            {
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].notDrawn = TRUE;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].notDrawn = FALSE;
            }
#endif // !REGION_US_BETA

            OptionsTimeAttackLoadRecord(FILE_SELECT_DATA.timeAttack100Only);

            FILE_SELECT_DATA.dispcnt |= (DCNT_BG0 | DCNT_BG1);
            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_8;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_8:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(1.f / 6))
            {
                FILE_SELECT_DATA.subMenuTimer = 0;
                FILE_SELECT_DATA.subMenuStage++;
            }

            if (gChangedInput & KEY_B)
                FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_10;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_9:
            if (!gChangedInput)
                break;

            if (gChangedInput & KEY_B)
            {
                FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_10;
                break;
            }
            
            if (FILE_SELECT_DATA.timeAttackRecordFlags != (1 | 2))
                break;

            if (!(gChangedInput & (KEY_LEFT | KEY_RIGHT)))
                break;

#ifndef REGION_US_BETA
            action = FALSE;

            if (FILE_SELECT_DATA.timeAttack100Only)
            {
                if (gChangedInput & KEY_LEFT)
                {
                    FILE_SELECT_DATA.timeAttack100Only = FALSE;
                    action = TRUE;
                }
            }
            else
            {
                if (gChangedInput & KEY_RIGHT)
                {
                    FILE_SELECT_DATA.timeAttack100Only = TRUE;
                    action = TRUE;
                }
            }

            if (!action)
                break;

            OptionsTimeAttackLoadBestTimeMessage();

            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].notDrawn = TRUE;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].notDrawn = TRUE;
#endif // !REGION_US_BETA

            SoundPlay(SOUND_CHANGE_TIME_ATTACK_PANEL);

#ifdef REGION_US_BETA
            FILE_SELECT_DATA.timeAttack100Only ^= TRUE;
#endif // REGION_US_BETA

            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
#ifdef REGION_US_BETA
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_6B;
#else // !REGION_US_BETA
            FILE_SELECT_DATA.subMenuStage = OPTIONS_TIME_ATTACK_STAGE_7;
#endif // REGION_US_BETA
            break;

        case OPTIONS_TIME_ATTACK_STAGE_10:
            SoundPlay(SOUND_CLOSE_SUB_MENU);

#ifndef REGION_US_BETA
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_LEFT_ARROW].notDrawn = TRUE;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_TIME_ATTACK_RIGHT_ARROW].notDrawn = TRUE;
#endif // !REGION_US_BETA

            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId++;
            FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId++;

            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_11:
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].exists |
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].exists)
                break;

            APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case OPTIONS_TIME_ATTACK_STAGE_12:
            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage = 0;
            return TRUE;
    }

    return FALSE;
}

#ifndef REGION_US_BETA
/**
 * @brief 7b71c | 28 | Adds best time or best time 100 message to queue
 * 
 */
static void OptionsTimeAttackLoadBestTimeMessage(void)
{
    if (FILE_SELECT_DATA.timeAttack100Only)
        FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME_100);
    else
        FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_BEST_TIME);
}
#endif // !REGION_US_BETA

/**
 * @brief 7b744 | 110 | Loads a time attack record into VRAM
 * 
 * @param id Id to load : 0 = Any%, 1 = 100%
 */
static void OptionsTimeAttackLoadRecord(u8 id)
{
    u16* dst;
    u16 baseTile;

    // Draw time attack time in this format :
    // HH:MM:SS

    dst = VRAM_BASE + 0xE108;
    // Palette 5
    baseTile = 5 << 12;

    // Hours tenths
    dst[0] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][0] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[0 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][0] + FILE_SELECT_TILE_NUMBER_LOW);

    // Hours seconds
    dst[1] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][1] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[1 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][1] + FILE_SELECT_TILE_NUMBER_LOW);

    // :
    dst[2] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
    dst[2 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

    // Minutes tenths
    dst[3] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][2] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[3 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][2] + FILE_SELECT_TILE_NUMBER_LOW);

    // Minutes seconds
    dst[4] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][3] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[4 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][3] + FILE_SELECT_TILE_NUMBER_LOW);

    // :
    dst[5] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
    dst[5 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

    // Seconds tenths
    dst[6] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][4] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[6 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][4] + FILE_SELECT_TILE_NUMBER_LOW);

    // Seconds seconds
    dst[7] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][5] + FILE_SELECT_TILE_NUMBER_HIGH);
    dst[7 + 32] = baseTile | (FILE_SELECT_DATA.timeAttackBestTimes[id][5] + FILE_SELECT_TILE_NUMBER_LOW);
}

static u8 sFileSelectDefaultPassword[8] = "--------";

/**
 * @brief 7b854 | d8 | To document
 * 
 */
static void unk_7b854(void)
{
    const u8* password;
    s32 i;
    u8 character;
    u32 high;
    u32 low;
    u8* dstHigh;
    u8* dstLow;
    u8 bitSize;

    password = FILE_SELECT_DATA.timeAttackPassword;
    if (!(gFileScreenOptionsUnlocked.timeAttack & 1))
        password = sFileSelectDefaultPassword;

    i = 0;
    bitSize = 16;
    dstHigh = VRAM_BASE + 0x6100;
    dstLow = dstHigh + 0x400;
    for (; i < 8; i++)
    {
        character = password[i];
        if (character == '1')
            character = '>';

        high = (character / 32) * 0x800;
        low = (character % 32) * 0x20;

        DmaTransfer(3, &sCharactersGfx[high + low], &dstHigh[i * 32], 32, bitSize);
        DmaTransfer(3, &sCharactersGfx[high + low + 1024], &dstLow[i * 32], 32, bitSize);
    }

    DmaTransfer(3, dstHigh, dstHigh + 0x1000, 0x100, 16);
    DmaTransfer(3, dstHigh + 0x400, dstHigh + 0x1400, 0x100, 16);
}

#ifdef DEBUG
static u8 sFileSelectBlankPassword_Debug[20] = "--------------------";
static u8 sFileSelectBlank100Password_Debug[20] = "====================";
#endif // DEBUG

/**
 * @brief 7b92c | bc | Loads a part of the time attack password to VRAM
 * 
 * @param part 2 bits : XY, where X is id and Y which half
 */
static void OptionsTimeAttackLoadPassword(u8 part)
{
    const u8* password;
    s32 i;
    u8 character;
    u32 high;
    u32 low;
    u8* dst;

    if (part >= 2)
    {
        i = 0x78C0;
        password = gTimeAttackRecord.password100;
#ifdef DEBUG
        if (password[0] == UCHAR_MAX)
            password = sFileSelectBlank100Password_Debug;
#endif // DEBUG
    }
    else
    {
        i = 0x68C0;
        password = gTimeAttackRecord.password;
#ifdef DEBUG
        if (password[0] == UCHAR_MAX)
            password = sFileSelectBlankPassword_Debug;
#endif // DEBUG
    }

    if (part & 1)
    {
        password += 10;
        i += 0x200;
    }
    
    dst = VRAM_BASE + i;
    for (i = 0; i < 10; i++)
    {
        character = password[i];
        if (character == '1')
            character = '>';

        high = (character / 32) * 0x800;
        low = (character % 32) * 0x20;

        DmaTransfer(3, &sCharactersGfx[high + low], &dst[i * 32], 32, 16);
        DmaTransfer(3, &sCharactersGfx[high + low + 0x400], &dst[0x400 + i * 32], 32, 16);
    }
}

/**
 * @brief 7b9e8 | 73c | Main loop for the metroid fusion link
 * 
 * @return u8 bool, ended
 */
static u8 OptionsMetroidFusionLinkHandler(void)
{
    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.subMenuTimer);

    if (gIoTransferInfo.active == ACTIVE_TRANSFER_CONNECT)
        FusionGalleryConnectProcess();
    else if (gIoTransferInfo.active == ACTIVE_TRANSFER_LINK)
        FusionGalleryLinkProcess();
    else
        gIoTransferInfo.result = TRANSFER_RESULT_NONE;

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0:
#ifdef DEBUG
            // Holding L forces attempt to link
            if (gFileScreenOptionsUnlocked.fusionGalleryImages && !(gButtonInput & KEY_L))
#else // !DEBUG
            if (gFileScreenOptionsUnlocked.fusionGalleryImages)
#endif // DEBUG
            {
                FILE_SELECT_DATA.subMenuStage = 1;
            }
            else
            {
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].boundBackground = 1;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].priority = BGCNT_LOW_MID_PRIORITY;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].xPosition = BLOCK_SIZE * 5;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].yPosition = BLOCK_SIZE * 3;

                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS].boundBackground = 1;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS].xPosition = BLOCK_SIZE * 5;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS].yPosition = BLOCK_SIZE * 3 + HALF_BLOCK_SIZE;

                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].boundBackground = 0;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].priority = BGCNT_LOW_MID_PRIORITY;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].xPosition = BLOCK_SIZE * 4;
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].yPosition = BLOCK_SIZE * 2;

                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_DO_NOT_TURN_POWER_OFF);
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_PLEASE_WAIT);

                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL], OPTIONS_OAM_ID_HUGE_PANEL_OPEN);
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL], OPTIONS_OAM_ID_LARGE_PANEL_OPEN);

                gBg1HOFS_NonGameplay = 0x6D8;
                gBg1VOFS_NonGameplay = 0x6F8;
                gBg0HOFS_NonGameplay = 0x660,
                gBg0VOFS_NonGameplay = 0x7E0;

                FILE_SELECT_DATA.subMenuStage = 2;
            }

            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 1:
            if (gChangedInput & KEY_B)
            {
                FILE_SELECT_DATA.subMenuTimer = 0;
                FILE_SELECT_DATA.subMenuStage = 0;
                return TRUE;
            }

            if (gChangedInput & KEY_A || FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(1.f / 6))
            {
                FILE_SELECT_DATA.subMenuTimer = 0;
                FILE_SELECT_DATA.subMenuStage = 0;
                return 2;
            }
            break;

        case 2:
            if ((FILE_SELECT_DATA.dispcnt & (DCNT_BG0 | DCNT_BG1)) == (DCNT_BG0 | DCNT_BG1))
            {
                gIoTransferInfo.active = ACTIVE_TRANSFER_CONNECT;
                FILE_SELECT_DATA.subMenuStage++;
                break;
            }

            if (!(FILE_SELECT_DATA.dispcnt & DCNT_BG1) && FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].ended)
            {
                if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_PLEASE_WAIT))
                {
                    DmaTransfer(3, (void*)sEwramPointer + 0x3C00, VRAM_BASE + 0xE800, 0x300, 16);
                    FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
                    FILE_SELECT_DATA.dispcnt |= DCNT_BG1;

                    UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], OPTIONS_OAM_ID_GBA_LINKING);
                }
            }

            if (!(FILE_SELECT_DATA.dispcnt & DCNT_BG0) && FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].ended)
            {
                if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_DO_NOT_TURN_POWER_OFF))
                {
                    DmaTransfer(3, (void*)sEwramPointer + 0x4800, VRAM_BASE + 0xE000, 0x300, 16);
                    FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                    FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                }
            }
            break;

        case 3:
            switch (gIoTransferInfo.result)
            {
                case TRANSFER_RESULT_SUCCESS:
                    SramWrite_FileScreenOptionsUnlocked();
                    FILE_SELECT_DATA.subMenuStage = 8;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    break;

                case TRANSFER_RESULT_FAILURE:
                    FILE_SELECT_DATA.subMenuStage = 4;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    break;

                case TRANSFER_RESULT_SUCCESS2:
                    FILE_SELECT_DATA.subMenuStage = 14;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    break;

                case TRANSFER_RESULT_TIMED_OUT:
                    FILE_SELECT_DATA.subMenuStage = 18;
                    break;

                case TRANSFER_RESULT_BACKED_OUT:
                    FILE_SELECT_DATA.subMenuStage = 21;
                    break;
            }
            break;

        case 4:
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_ERROR);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_TURN_OFF_CONFIRM_LINK);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], 0);

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId == 0)
                UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL], OPTIONS_OAM_ID_LARGE_PANEL_OPEN);

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 5:
            if (!FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].ended)
                break;

            if (!FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].ended)
                break;

            if (!FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_TURN_OFF_CONFIRM_LINK))
                break;

            if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_ERROR))
            {
                FILE_SELECT_DATA.dispcnt |= (DCNT_BG0 | DCNT_BG1);
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 6:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(.5f))
                FILE_SELECT_DATA.subMenuStage++;
            break;

        case 7:
            if (gChangedInput & KEY_START)
                gMainGameMode = GM_START_SOFT_RESET;
            break;

        case 8:
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);

            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], 0);

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId != 0)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId++;

            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_DATA_UPLOAD_COMPLETE);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 9:
            if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_DATA_UPLOAD_COMPLETE))
            {
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 10:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(1.f))
                FILE_SELECT_DATA.subMenuStage++;
            break;

        case 11:
            if (gChangedInput & (KEY_A | KEY_START))
            {
                FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId++;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 12:
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId == 0)
            {
                FILE_SELECT_DATA.subMenuStage++;
                FILE_SELECT_DATA.subMenuTimer = 0;
            }
            break;

        case 13:
            if (FILE_SELECT_DATA.subMenuTimer > CONVERT_SECONDS(.5f))
            {
                OptionsSetupTiletable();
                DmaTransfer(3, (void*)sEwramPointer + 0x5100, VRAM_BASE + 0xF000, 0x800, 16);
                FILE_SELECT_DATA.subMenuStage = 23;
            }
            break;

        case 14:
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], 0);
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_UNABLE_TO_DETECT_FUSION);

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId != 0)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId++;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 15:
            if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_UNABLE_TO_DETECT_FUSION))
            {
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 16:
            if (gIoTransferInfo.result == TRANSFER_RESULT_FAILURE)
            {
                FILE_SELECT_DATA.subMenuStage = 4;
                break;
            }

            if (gChangedInput & (KEY_A | KEY_START))
            {
                gIoTransferInfo.linkInProgress = 0;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 17:
            if (gIoTransferInfo.result == TRANSFER_RESULT_FAILURE)
                FILE_SELECT_DATA.subMenuStage = 21;
            break;

        case 18:
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], 0);

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId != 0)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId++;

            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_ERROR_CHECK_CONNECTION);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 19:
            if (FileScreenUpdateMessageInfoIdQueue(1, FILE_SCREEN_MESSAGE_INFO_ID_LINKING_ERROR_CHECK_CONNECTION))
            {
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 20:
            if (gChangedInput & (KEY_A | KEY_START))
                FILE_SELECT_DATA.subMenuStage = 21;
            break;

        case 21:
            FILE_SELECT_DATA.dispcnt &= ~(DCNT_BG0 | DCNT_BG1);
            UpdateMenuOamDataId(&FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_FUSION_LINK_GBAS], 0);

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId != 0)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId++;

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId != 0)
                FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId++;
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 22:
            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_HUGE_PANEL].oamId != 0)
                break;

            if (FILE_SELECT_DATA.optionsOam[OPTIONS_OAM_LARGE_PANEL].oamId != 0)
                break;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 23:
            FILE_SELECT_DATA.subMenuTimer = 0;
            FILE_SELECT_DATA.subMenuStage = 0;
            return TRUE;
    }

    return FALSE;
}

/**
 * @brief 7c124 | 1dc | Main loop for the file select menu
 * 
 * @return u32 bool, leaving
 */
u32 FileSelectMenuHandler(void)
{
    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.timer);

    switch (gSubGameMode1)
    {
        case 0: // Init
            gSubGameMode2 = 0;
            gCutsceneToSkip = 0;
            FileSelectInit(); // Sets SubGameMode1 to 2
            gSubGameMode1--; // Then decrease it to 1, so the next frame it will apply the fading
            break;

        case 1:
        case 9:
            FileSelectApplyFading();
            if (FileSelectUpdateFading())
            {
                gSubGameMode1++;
                gSubGameMode2 = 0;
            }
            break;

        case 2:
            if (FileSelectUpdateSubMenu())
            {
                FILE_SELECT_DATA.timer = 0;
                if (gSubGameMode2 == 1)
                    gSubGameMode1 = 7;
                else if (gSubGameMode2 == 2)
                    gSubGameMode1 = 4;
                else if (gSubGameMode2 == 3)
                    gSubGameMode1 = 3;
                else
                {
                    FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
                    gSubGameMode1 = 10;
                }

                if (gSubGameMode2 == 0)
                    break;

                FileSelectInitFading(TRUE);
                gOptionsOptionSelected = 0;
            }
            break;

        case 10:
            if (OptionsHandler())
            {
                if (gSubGameMode2)
                {
                    FileSelectInitFading(TRUE);
                    gSubGameMode1 = 11;
                }
                else
                {
                    gSubGameMode1 = 2;
                    FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_LEAVING_OPTIONS;
                    FILE_SELECT_DATA.timer = 0;
                    FILE_SELECT_DATA.subMenuStage = 0;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                }
            }
            break;

        case 3:
        case 11:
            FileSelectApplyFading();
            if (FileSelectUpdateFading())
                return TRUE;
            break;

        case 4:
        case 7:
            FileSelectApplyFading();
            if (FileSelectUpdateFading())
            {
                FILE_SELECT_DATA.timer = 0;
                gSubGameMode1++;
            }
            break;

        case 5:
            if (FILE_SELECT_DATA.timer > CONVERT_SECONDS(.5f))
                gSubGameMode1++;
            break;

        case 6:
            unk_75c04(0);
            return TRUE;

        case 8:
            unk_75c04(0);
#ifdef REGION_JP
            SramWrite_Language();
#endif // REGION_JP
            return TRUE;
    }

    FileSelectProcessOam();
    return FALSE;
}

/**
 * @brief 7c300 | 1b0 | Handles the fadings (in and out) of the file select menu
 * 
 * @return u32 bool, ended
 */
static u32 FileSelectUpdateFading(void)
{
    u32 ended;
    u16* src;
    u16* dst;

    ended = FALSE;

    APPLY_DELTA_TIME_INC(FILE_SELECT_DATA.fadingTimer);

    switch (FILE_SELECT_DATA.fadingStage)
    {
        case FILE_SELECT_FADING_STAGE_FADE_IN:
            if (FILE_SELECT_DATA.paletteUpdated)
                break;

            // frequency always 0?
            if (FILE_SELECT_DATA.fadingTimer < FILE_SELECT_DATA.fadingFrequency)
                break;

            if (FILE_SELECT_DATA.colorToApply < 32)
            {
                src = (void*)sEwramPointer + 0x000;
                dst = (void*)sEwramPointer + 0x400;
                ApplySpecialBackgroundFadingColor(COLOR_FADING_TYPE_IN, FILE_SELECT_DATA.colorToApply, &src, &dst, USHORT_MAX);

                src = (void*)sEwramPointer + 0x200;
                dst = (void*)sEwramPointer + 0x600;
                ApplySpecialBackgroundFadingColor(COLOR_FADING_TYPE_IN, FILE_SELECT_DATA.colorToApply, &src, &dst, USHORT_MAX);

                FILE_SELECT_DATA.paletteUpdated = TRUE;
                if (FILE_SELECT_DATA.colorToApply == 31)
                {
                    FILE_SELECT_DATA.colorToApply++;
                }
                else
                {
                    if (FILE_SELECT_DATA.colorToApply + FILE_SELECT_DATA.fadingIntensity > 31)
                        FILE_SELECT_DATA.colorToApply = 31;
                    else
                        FILE_SELECT_DATA.colorToApply += FILE_SELECT_DATA.fadingIntensity;
                }
                break;
            }

            DmaTransfer(3, (void*)sEwramPointer, (void*)sEwramPointer + 0x400, PALRAM_SIZE, 16);
            FILE_SELECT_DATA.paletteUpdated = TRUE;
            FILE_SELECT_DATA.fadingStage++;
            break;

        case FILE_SELECT_FADING_STAGE_CHECK_FADE_IN:
            if (!FILE_SELECT_DATA.paletteUpdated)
            {
                FILE_SELECT_DATA.colorToApply = 0;
                FILE_SELECT_DATA.fadingStage = 0;
                ended = TRUE;
            }
            break;

        case FILE_SELECT_FADING_STAGE_FADE_OUT:
            if (FILE_SELECT_DATA.paletteUpdated)
                break;

            if (FILE_SELECT_DATA.fadingTimer < FILE_SELECT_DATA.fadingFrequency)
                break;

            FILE_SELECT_DATA.fadingTimer = 0;
            if (FILE_SELECT_DATA.colorToApply < 32)
            {
                src = (void*)sEwramPointer + 0x000;
                dst = (void*)sEwramPointer + 0x400;
                ApplySpecialBackgroundFadingColor(COLOR_FADING_TYPE_OUT, FILE_SELECT_DATA.colorToApply, &src, &dst, USHORT_MAX);
                src = (void*)sEwramPointer + 0x200;
                dst = (void*)sEwramPointer + 0x600;
                ApplySpecialBackgroundFadingColor(COLOR_FADING_TYPE_OUT, FILE_SELECT_DATA.colorToApply, &src, &dst, USHORT_MAX);

                FILE_SELECT_DATA.paletteUpdated = TRUE;
                if (FILE_SELECT_DATA.colorToApply == 31)
                {
                    FILE_SELECT_DATA.colorToApply++;
                }
                else
                {
                    if (FILE_SELECT_DATA.colorToApply + FILE_SELECT_DATA.fadingIntensity > 31)
                        FILE_SELECT_DATA.colorToApply = 31;
                    else
                        FILE_SELECT_DATA.colorToApply += FILE_SELECT_DATA.fadingIntensity;
                }
                break;
            }

            BitFill(3, 0, (void*)sEwramPointer + 0x400, PALRAM_SIZE, 16);
            FILE_SELECT_DATA.paletteUpdated = TRUE;
            FILE_SELECT_DATA.fadingStage++;
            break;

        case FILE_SELECT_FADING_STAGE_CHECK_FADE_OUT:
            if (!FILE_SELECT_DATA.paletteUpdated)
            {
                FILE_SELECT_DATA.colorToApply = 0;
                FILE_SELECT_DATA.fadingStage = 0;
                ended = TRUE;
            }
            break;
    }

    return ended;
}

/**
 * @brief 7c4b0 | b8 | To document
 * 
 * @param fadingOut Bool, file select fading out
 */
static void FileSelectInitFading(u8 fadingOut)
{
    FILE_SELECT_DATA.colorToApply = 0;
    FILE_SELECT_DATA.paletteUpdated = 0;
    FILE_SELECT_DATA.fadingTimer = 0;

    if (!fadingOut)
    {
        DmaTransfer(3, PALRAM_BASE, (void*)sEwramPointer, PALRAM_SIZE, 16);
        BitFill(3, 0, PALRAM_BASE, PALRAM_SIZE, 16);
        DmaTransfer(3, PALRAM_BASE, (void*)sEwramPointer + 0x400, PALRAM_SIZE, 16);

        FILE_SELECT_DATA.fadingStage = FILE_SELECT_FADING_STAGE_FADE_IN;
        FILE_SELECT_DATA.fadingIntensity = 8;
        FILE_SELECT_DATA.fadingFrequency = 0;
    }
    else
    {
        DmaTransfer(3, PALRAM_BASE, (void*)sEwramPointer, PALRAM_SIZE, 16);

        FILE_SELECT_DATA.fadingStage = FILE_SELECT_FADING_STAGE_FADE_OUT;
        FILE_SELECT_DATA.fadingIntensity = 8;
        FILE_SELECT_DATA.fadingFrequency = 0;
    }
}

/**
 * @brief 7c568 | 3c | Transfers the fading palette to palette RAM
 * 
 */
static void FileSelectApplyFading(void)
{
    if (FILE_SELECT_DATA.paletteUpdated)
    {
        DmaTransfer(3, (void*)sEwramPointer + 0x400, PALRAM_BASE, PALRAM_SIZE, 16);
        FILE_SELECT_DATA.paletteUpdated = FALSE;
    }
}

/**
 * @brief 7c5a4 | 3c | Sets the language to the default for the game's region
 * 
 */
static void FileSelectSetLanguage(void)
{
    s32 i;

    i = FALSE;

#if defined(REGION_EU)
    if (INVALID_EU_LANGUAGE(gLanguage))
#elif defined(REGION_JP)
    if (gLanguage > LANGUAGE_HIRAGANA)
#else // !REGION_JP
    if (gLanguage != LANGUAGE_ENGLISH)
#endif // REGION_JP
    {
        gLanguage = LANGUAGE_DEFAULT;
        i = TRUE;
    }

    if (i)
        SramWrite_Language();

    for (i = 0; i < ARRAY_SIZE(gSaveFilesInfo); i++)
    {
#ifdef REGION_JP
        if (gSaveFilesInfo[i].language > LANGUAGE_HIRAGANA)
            gSaveFilesInfo[i].language = LANGUAGE_JAPANESE;
#else // !REGION_JP
        gSaveFilesInfo[i].language = gLanguage;
#endif // REGION_JP
    }
}

/**
 * @brief 7c5e0 | 448 | Initializes the file select menu 
 * 
 */
static void FileSelectInit(void)
{
    CallbackSetVblank(FileSelectVBlank_Empty);

    BitFill(3, 0, &gNonGameplayRam, sizeof(gNonGameplayRam), 32);

    WRITE_16(REG_DISPCNT, FILE_SELECT_DATA.dispcnt = 0);
    WRITE_16(REG_BLDY, gWrittenToBldy_NonGameplay = BLDY_MAX_VALUE);
    WRITE_16(REG_BLDCNT, FILE_SELECT_DATA.bldcnt = BLDCNT_SCREEN_FIRST_TARGET | BLDCNT_BRIGHTNESS_DECREASE_EFFECT);

    gNextOamSlot = 0;
    ClearGfxRam();
    ResetFreeOam();

    gOamXOffset_NonGameplay = gOamYOffset_NonGameplay = 0;

    gBootDebugActive = FALSE;
    gDebugMode = FALSE;

    BitFill(3, 0, (void*)sEwramPointer + 0x1000, 0x800, 16);
    SramWrite_FileInfo();
    FileSelectSetLanguage();

    FILE_SELECT_DATA.unk_24 = gMostRecentSaveFile;
    FILE_SELECT_DATA.unk_25 = gMostRecentSaveFile;
    FILE_SELECT_DATA.copySourceFile = gMostRecentSaveFile;
    FILE_SELECT_DATA.currentFile = gMostRecentSaveFile;
    FILE_SELECT_DATA.eraseFile = gMostRecentSaveFile;
    FILE_SELECT_DATA.fileSelectCursorPosition = gMostRecentSaveFile;

    DmaTransfer(3, sFileSelectPal, PALRAM_BASE, sizeof(sFileSelectPal), 16);
    DmaTransfer(3, sFileSelect_4548f8, PALRAM_BASE + (PAL_SIZE - 2 * PAL_ROW_SIZE), sizeof(sFileSelect_4548f8), 16);
    DmaTransfer(3, sFileSelectIconsPal, PALRAM_OBJ, sizeof(sFileSelectIconsPal), 16);
    SET_BACKDROP_COLOR(COLOR_BLACK);

#ifdef REGION_EU
    CallLZ77UncompVram(sFileSelectAreaNamesGfx, VRAM_BASE + 0x2400);
    CallLZ77UncompVram(sFileSelectBgIconsGfx, VRAM_BASE + 0x3800);
    CallLZ77UncompVram(sFileSelectLargeTextGfxPointers[gLanguage - LANGUAGE_ENGLISH], VRAM_BASE + 0x4800);
    CallLZ77UncompVram(sFileSelectDifficultyTextGfxPointers[gLanguage - LANGUAGE_ENGLISH], VRAM_BASE + 0x3400);
#else // !REGION_EU
    CallLZ77UncompVram(sFileSelectCharactersGfx, VRAM_BASE + 0x400);
#endif // REGION_EU

    CallLZ77UncompVram(sFileSelectChozoBackgroundGfx, VRAM_BASE + 0x8000);
    CallLZ77UncompVram(sFileSelectObjIconsGfx, VRAM_OBJ);

    // If not on JP, the translations for "Copy", "Erase", and "Options" are blanked out,
    // and the options menu text graphics are replaced with the appropriate language.
    // Debug allows any language, so it has an extra check.
#if defined(DEBUG) || !defined(REGION_JP)
#if defined(DEBUG)
    if (gLanguage >= LANGUAGE_ENGLISH)
#endif // DEBUG
    {
#if defined(DEBUG) || !defined(REGION_EU)
        BitFill(3, 0, VRAM_BASE + 0x400, 0x800, 16);
#endif // DEBUG || !REGION_EU
        CallLZ77UncompVram(sFileSelectOptionsTextGfxPointers[gLanguage - LANGUAGE_ENGLISH], VRAM_BASE + 0xC00);
    }
#endif // DEBUG || !REGION_JP

    CallLZ77UncompVram(sFileSelectChozoBackgroundTileTable, VRAM_BASE + 0xF800);

#ifdef REGION_EU
    CallLZ77UncompWram(sFileSelectOptionsTileTable, (void*)sEwramPointer + 0x800);
#else // !REGION_EU
    CallLZ77UncompWram(sFileSelectMenuTileTable, (void*)sEwramPointer + 0x800);
#endif // REGION_EU
    CallLZ77UncompWram(sFileSelect3BigPanelsTileTable, (void*)sEwramPointer + 0x2800);
    CallLZ77UncompWram(sFileSelect1Small2BigPanelsTileTable, (void*)sEwramPointer + 0x1800);
    CallLZ77UncompWram(sFileSelect2Big1SmallPanelsTileTable, (void*)sEwramPointer + 0x2000);

    OptionsSetupTiletable();
    FileSelectCopyTimeAttackTime();
    FileSelectDisplaySaveFileInfo();
    DmaTransfer(3, (void*)sEwramPointer + 0x800, VRAM_BASE + 0xD800, 0x800, 16);
#ifndef BUGFIX
    SramRead_SoundMode();
    FileSelectApplyStereo();
#endif // !BUGFIX

    gSubGameMode1 = 2;

    if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].corruptionFlag ||
        gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].corruptionFlag ||
        gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].corruptionFlag)
    {
        FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_CORRUPTED;
    }
    else
    {
        if (FILE_SELECT_DATA.optionsUnlocked[gOptionsOptionSelected] && gOptionsOptionSelected > 0 && (u8)gOptionsOptionSelected < 7)
        {
            gSubGameMode1 = 10;
            OptionsSoundTestCheckNotAlreadyPlaying();
            FILE_SELECT_DATA.currentSubMenu = 1;
        }
        else
        {
            PlayMusic(MUSIC_FILE_SELECT, 2);
            gOptionsOptionSelected = 0;
            FILE_SELECT_DATA.currentSubMenu = 0;
        }
    }

    gBg0HOFS_NonGameplay = gBg0VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;
    gBg1HOFS_NonGameplay = gBg1VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;
    gBg2HOFS_NonGameplay = gBg2VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;
    gBg3HOFS_NonGameplay = gBg3VOFS_NonGameplay = NON_GAMEPLAY_START_BG_POS;

    FILE_SELECT_DATA.dispcnt = DCNT_BG1 | DCNT_BG3 | DCNT_OBJ;
    FILE_SELECT_DATA.unk_14 = CREATE_BGCNT(2, 31, BGCNT_LOW_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_16 = CREATE_BGCNT(0, 30, BGCNT_LOW_MID_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_18 = CREATE_BGCNT(0, 27, BGCNT_HIGH_MID_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_1A = CREATE_BGCNT(0, 27, BGCNT_LOW_MID_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_1C = CREATE_BGCNT(0, 29, BGCNT_HIGH_MID_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_1E = CREATE_BGCNT(0, 28, BGCNT_HIGH_PRIORITY, BGCNT_SIZE_256x256);
    FILE_SELECT_DATA.unk_20 = CREATE_BGCNT(0, 30, BGCNT_LOW_MID_PRIORITY, BGCNT_SIZE_256x256);

    WRITE_16(REG_BG0CNT, FILE_SELECT_DATA.bg0cnt = 0);
    WRITE_16(REG_BG1CNT, FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_18);
    WRITE_16(REG_BG2CNT, FILE_SELECT_DATA.bg2cnt = FILE_SELECT_DATA.unk_16);
    WRITE_16(REG_BG3CNT, FILE_SELECT_DATA.bg3cnt = FILE_SELECT_DATA.unk_14);

    FILE_SELECT_DATA.fileSelectCursors = sFileSelectMenuCursors_Empty;

    FILE_SELECT_DATA.processTextStage = FILE_SCREEN_PROCESS_TEXT_STAGE_NONE;
    FILE_SELECT_DATA.messageInfoIdQueue[0] = UCHAR_MAX;
    FILE_SELECT_DATA.messageInfoIdQueue[1] = UCHAR_MAX;
    FILE_SELECT_DATA.messageInfoIdQueue[2] = UCHAR_MAX;

    FileSelectResetOam();
    FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.unk_24);

    if (gSubGameMode1 == 2)
    {
        if (FILE_SELECT_DATA.currentSubMenu == FILE_SELECT_SUB_MENU_CORRUPTED)
        {
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].notDrawn = TRUE;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].oamId = FILE_SELECT_OAM_ID_FILE_A_MARKER_IDLE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].oamId = FILE_SELECT_OAM_ID_FILE_B_MARKER_IDLE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].oamId = FILE_SELECT_OAM_ID_FILE_C_MARKER_IDLE;
        }
    }
    else
    {
        unk_79ecc();
        OptionsUpdateCursor(CURSOR_OPTIONS_POSE_MOVING);
        OptionsUpdateCursor(4);

        FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
        FILE_SELECT_DATA.dispcnt |= (DCNT_BG2 | DCNT_OBJ);
    }

    FileSelectProcessOam();
    FileSelectInitFading(FALSE);

    WRITE_16(REG_BLDY, gWrittenToBldy_NonGameplay = 0);
    WRITE_16(REG_BLDCNT, FILE_SELECT_DATA.bldcnt = 0);

    FileSelectVBlank();
    CallbackSetVblank(FileSelectVBlank);
}

/**
 * @brief 7ca28 | f8 | File select menu V-blank code
 * 
 */
static void FileSelectVBlank(void)
{
    if (gIoTransferInfo.linkInProgress)
        LinkVSync();

    DMA3_COPY_32(gOamData, OAM_BASE, OAM_SIZE / sizeof(u32))

    WRITE_16(REG_BG0HOFS, gBg0HOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG0VOFS, gBg0VOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG1HOFS, gBg1HOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG1VOFS, gBg1VOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG2HOFS, gBg2HOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG2VOFS, gBg2VOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG3HOFS, gBg3HOFS_NonGameplay / PIXEL_SIZE);
    WRITE_16(REG_BG3VOFS, gBg3VOFS_NonGameplay / PIXEL_SIZE);

    WRITE_16(REG_DISPCNT, FILE_SELECT_DATA.dispcnt);
    WRITE_16(REG_BLDY, gWrittenToBldy_NonGameplay);
    WRITE_16(REG_BLDALPHA, C_16_2_8(gWrittenToBldalpha_H, gWrittenToBldalpha_L));
    WRITE_16(REG_BLDCNT, FILE_SELECT_DATA.bldcnt);
    WRITE_16(REG_BG0CNT, FILE_SELECT_DATA.bg0cnt);
    WRITE_16(REG_BG1CNT, FILE_SELECT_DATA.bg1cnt);
    WRITE_16(REG_BG2CNT, FILE_SELECT_DATA.bg2cnt);
}

/**
 * @brief 7cb20 | c | Empty v-blank for file select
 * 
 */
static void FileSelectVBlank_Empty(void)
{
    vu8 c = 0;
}

/**
 * @brief 7cb2c | 224 | Displays the info of every save file
 * 
 */
static void FileSelectDisplaySaveFileInfo(void)
{
    FileSelectDisplaySaveFileHealth(FILE_SELECT_CURSOR_POSITION_FILE_A);
    FileSelectDisplaySaveFileTimer(FILE_SELECT_CURSOR_POSITION_FILE_A);
    FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A], FILE_SELECT_CURSOR_POSITION_FILE_A);

    FileSelectDisplaySaveFileHealth(FILE_SELECT_CURSOR_POSITION_FILE_B);
    FileSelectDisplaySaveFileTimer(FILE_SELECT_CURSOR_POSITION_FILE_B);
    FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B], FILE_SELECT_CURSOR_POSITION_FILE_B);

    FileSelectDisplaySaveFileHealth(FILE_SELECT_CURSOR_POSITION_FILE_C);
    FileSelectDisplaySaveFileTimer(FILE_SELECT_CURSOR_POSITION_FILE_C);
    FileSelectDisplaySaveFileMiscInfo(&gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C], FILE_SELECT_CURSOR_POSITION_FILE_C);

    FileScreenSetEnabledMenuFlags();

    BitFill(3, 0, (void*)sEwramPointer + 0x3000, 0x300, 16);
    BitFill(3, 0, (void*)sEwramPointer + 0x3300, 0x300, 16);
    BitFill(3, 0, (void*)sEwramPointer + 0x3600, 0x300, 16);

    DmaTransfer(3, (void*)sEwramPointer + 0x1800, (void*)sEwramPointer + 0x3000, 0x200, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x1A00, (void*)sEwramPointer + 0x3300, 0x300, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x1D00, (void*)sEwramPointer + 0x3600, 0x300, 16);

    BitFill(3, 0, (void*)sEwramPointer + 0x3900, 0x300, 16);
    BitFill(3, 0, (void*)sEwramPointer + 0x3C00, 0x300, 16);

    DmaTransfer(3, (void*)sEwramPointer + 0x2000, (void*)sEwramPointer + 0x3900, 0x280, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x2280, (void*)sEwramPointer + 0x3C00, 0x300, 16);

    BitFill(3, 0, (void*)sEwramPointer + 0x3F00, 0x300, 16);
    BitFill(3, 0, (void*)sEwramPointer + 0x4200, 0x300, 16);
    BitFill(3, 0, (void*)sEwramPointer + 0x4500, 0x300, 16);

    DmaTransfer(3, (void*)sEwramPointer + 0x2800, (void*)sEwramPointer + 0x3F00, 0x280, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x2A80, (void*)sEwramPointer + 0x4200, 0x280, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x2D00, (void*)sEwramPointer + 0x4500, 0x280, 16);

    BitFill(3, 0, (void*)sEwramPointer + 0x4800, 0x300, 16);

    DmaTransfer(3, (void*)sEwramPointer + 0x2580, (void*)sEwramPointer + 0x4800, 0x200, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x3300, (void*)sEwramPointer + 0x4B00, 0x300, 16);
    DmaTransfer(3, (void*)sEwramPointer + 0x3600, (void*)sEwramPointer + 0x4E00, 0x300, 16);
}

/**
 * @brief 7cd50 | 74 | Draws the energy on a file
 * 
 * @param file File number
 */
static void FileSelectDisplaySaveFileHealth(u8 file)
{
    u32 offset;

    // Why do that instead of file + 1?
    if (file == 0)
        offset = 1;
    else if (file == 1)
        offset = 2;
    else
        offset = 3;

    // Check file exists
    if ((gSaveFilesInfo[file].exists || gSaveFilesInfo[file].introPlayed) && gSaveFilesInfo[file].corruptionFlag == 0)
    {
        // Transfer energy for the hud draw function
        gEquipment.currentEnergy = gSaveFilesInfo[file].currentEnergy;
        gEquipment.maxEnergy = gSaveFilesInfo[file].maxEnergy;
        
        // Draw energy
        HudDrawEnergy(offset);
    }
    else
    {
        // Clear graphics
        BitFill(3, 0, VRAM_BASE + 0x20 + (offset - 1) * 0xE0, 0xE0, 16);
    }
}

/**
 * @brief 7cdc4 | 1d4 | Displays the timer on a save file
 * 
 * @param file File number
 */
static void FileSelectDisplaySaveFileTimer(u8 file)
{
    u16 baseTile;
    u16* dst;
    u8 number;

    dst = FILE_SELECT_EWRAM.menuTilemap;

    if (file == 0)
        number = 1;
    else if (file == 1)
        number = 2;
    else
        number = 3;

    // Get destination
    dst = &dst[(number - 1) * 96 + 0x6E];

    // Get palette, 6 if time attack, 5 if not
    baseTile = gSaveFilesInfo[file].timeAttack ? 6 << 12 : 5 << 12;

    if (gSaveFilesInfo[file].exists && gSaveFilesInfo[file].corruptionFlag == 0)
    {
        // File exists, draw HH:MM:SS

        // Draw hours
        number = gSaveFilesInfo[file].igtHours / 10; // Tenths
        dst[0] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[0 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);
        
        number = gSaveFilesInfo[file].igtHours % 10; // Seconds
        dst[1] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[1 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);

        // :
        dst[2] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
        dst[2 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

        // Draw minutes
        number = gSaveFilesInfo[file].igtMinutes / 10; // Tenths
        dst[3] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[3 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);
        
        number = gSaveFilesInfo[file].igtMinutes % 10; // Seconds
        dst[4] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[4 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);

        // :
        dst[5] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
        dst[5 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

        // Draw seconds
        number = gSaveFilesInfo[file].igtSeconds / 10; // Tenths
        dst[6] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[6 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);
        
        number = gSaveFilesInfo[file].igtSeconds % 10; // Seconds
        dst[7] = baseTile | (number + FILE_SELECT_TILE_NUMBER_HIGH);
        dst[7 + 32] = baseTile | (number + FILE_SELECT_TILE_NUMBER_LOW);
    }
    else
    {
        // File doesn't exists, draw --:--:--

        // --
        dst[0] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[0 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;
        dst[1] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[1 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;

        // :
        dst[2] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
        dst[2 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

        // --
        dst[3] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[3 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;
        dst[4] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[4 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;

        // :
        dst[5] = baseTile | FILE_SELECT_TILE_TWO_DOTS_HIGH;
        dst[5 + 32] = baseTile | FILE_SELECT_TILE_TWO_DOTS_LOW;

        // --
        dst[6] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[6 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;
        dst[7] = baseTile | FILE_SELECT_TILE_MINUS_HIGH;
        dst[7 + 32] = baseTile | FILE_SELECT_TILE_MINUS_LOW;
    }
}

/**
 * @brief 7cf98 | 118 | Displays the misc. info of a file (difficulty, area, time attack)
 * 
 * @param pFile Save file info pointer
 * @param file Save file number
 */
static void FileSelectDisplaySaveFileMiscInfo(struct SaveFileInfo* pFile, FileSelectCursorPosition file)
{
    s32 offset;
    s32 temp;
    s32 palette;
    u16* dst;
    u16 tile;
    s32 i;

    if (file == FILE_SELECT_CURSOR_POSITION_FILE_C)
        offset = 2 * 96;
    else if (file == FILE_SELECT_CURSOR_POSITION_FILE_B)
        offset = 1 * 96;
    else if (file == FILE_SELECT_CURSOR_POSITION_FILE_A)
        offset = 0 * 96;
    else
        return;

    if (pFile->timeAttack)
        temp = 6 << 12;
    else
        temp = 5 << 12;
    palette = temp;

    dst = FILE_SELECT_EWRAM.menuTilemap;
    dst = &dst[offset + 102];

    if ((pFile->exists || pFile->introPlayed) && pFile->corruptionFlag == 0)
    {
        if (pFile->timeAttack)
        {
            tile = 0x1AF;
#if defined(DEBUG) || defined(REGION_JP)
            if (pFile->language == LANGUAGE_HIRAGANA)
                tile += 5;
#endif // DEBUG || REGION_JP
        }
        else
        {
#if defined(DEBUG) || defined(REGION_JP)
            tile = pFile->difficulty * 5;
            if (pFile->language == LANGUAGE_HIRAGANA)
                tile += 0x13D;
            else
                tile += 0x1A0;
#else // !(DEBUG || REGION_JP)
            tile = 0x1A0 + pFile->difficulty * 5;
#endif // DEBUG || REGION_JP
        }

        for (i = 0; i < 5; i++)
        {
            *dst++ = palette | tile++;
        }
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            *dst++ = 0;
        }
    }

    dst = FILE_SELECT_EWRAM.menuTilemap;
    dst = &dst[offset + 150];

    if (pFile->currentArea >= ARRAY_SIZE(sSaveFileAreasId))
        i = -1;
    else
        i = sSaveFileAreasId[pFile->currentArea];

    if ((pFile->exists || pFile->introPlayed) && i >= 0 && pFile->corruptionFlag == 0)
    {
#if defined(DEBUG) || defined(REGION_JP)
        tile = i * 6;
        if (pFile->language == LANGUAGE_HIRAGANA)
            tile += 0x14C;
        else
            tile += 0x176;
#else // !(DEBUG || REGION_JP)
        tile = i * 6 + 0x176;
#endif // DEBUG || REGION_JP

        for (i = 0; i < 6; i++)
        {
            *dst++ = palette | tile++;
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            *dst++ = 0;
        }
    }
}

/**
 * @brief 7d0b0 | ec | Sets the enabled menus flags
 * 
 */
static void FileScreenSetEnabledMenuFlags(void)
{
    u16 palette;
    u16* src;
    u16 i;

    // Clear
    FILE_SELECT_DATA.enabledMenus = 0;

    // Check set file flags
    if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].corruptionFlag == 0 &&
        (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].exists || gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_A].introPlayed))
        FILE_SELECT_DATA.enabledMenus |= 1 << FILE_SELECT_CURSOR_POSITION_FILE_A;

    if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].corruptionFlag == 0 &&
        (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].exists || gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_B].introPlayed))
        FILE_SELECT_DATA.enabledMenus |= 1 << FILE_SELECT_CURSOR_POSITION_FILE_B;

    if (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].corruptionFlag == 0 &&
        (gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].exists || gSaveFilesInfo[FILE_SELECT_CURSOR_POSITION_FILE_C].introPlayed))
        FILE_SELECT_DATA.enabledMenus |= 1 << FILE_SELECT_CURSOR_POSITION_FILE_C;

    src = (void*)sEwramPointer + 0xB40;

    if (FILE_SELECT_DATA.enabledMenus)
    {
        // At least a single file exists, enable copy and erase
        FILE_SELECT_DATA.enabledMenus |= 1 << FILE_SELECT_CURSOR_POSITION_COPY | 1 << FILE_SELECT_CURSOR_POSITION_ERASE;
        palette = 0;
    }
    else
    {
        palette = 1;
    }

    // Update copy and erase text palette
    palette <<= 0xC;
    for (i = 0; i < 0x80; i++)
    {
        *src = (*src & 0xFFF) | palette;
        src++;
    }

    // Enable options
    FILE_SELECT_DATA.enabledMenus |= 1 << FILE_SELECT_CURSOR_POSITION_OPTIONS;
}

/**
 * @brief 7d19c | 94 | Applies the up/down movement
 * 
 * @param set Set allowed
 * @param pFileNumber File number pointer
 * @return u8 Could move
 */
static u8 FileSelectApplyMenuSelectInput(FileSelectSelectionSet set, FileSelectCursorPosition* pFileNumber)
{
    s32 direction;
    u8 position;
    u8 flags;

    // Get direction
    if (gChangedInput & KEY_UP)
        direction = -1;
    else if (gChangedInput & KEY_DOWN)
        direction = 1;
    else
        return 0;

    // Get current position
    position = *pFileNumber;

    // Use enabled menus
    flags = FILE_SELECT_DATA.enabledMenus;

    if (set == FILE_SELECT_SELECTION_SET_ALL_FILES)
    {
        // Add all files
        flags |= 1 << FILE_SELECT_CURSOR_POSITION_FILE_A |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_B |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_C;
    }
    else if (set == FILE_SELECT_SELECTION_SET_ONLY_FILES)
    {
        // Keep only all files
        flags &= 1 << FILE_SELECT_CURSOR_POSITION_FILE_A |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_B |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_C;
    }
    else if (set == FILE_SELECT_SELECTION_SET_ALL_FILES_NO_COPY)
    {
        // Add all files except the copy source
        flags = (1 << FILE_SELECT_CURSOR_POSITION_FILE_A |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_B |
                 1 << FILE_SELECT_CURSOR_POSITION_FILE_C) & ~(1 << FILE_SELECT_DATA.copySourceFile);
    }

    while (TRUE)
    {
        if (direction != 0)
        {
            // Check for limit
            if ((u32)(position + direction) > FILE_SELECT_CURSOR_POSITION_OPTIONS)
            {
                // Couldn't move, abort
                direction = FALSE;
                break;
            }

            // Move the cursor
            position += direction;
        }
        else
        {
            // No direction, break
            break;
        }

        if ((flags >> position) & 1)
        {
            // Write position
            *pFileNumber = position;

            // Could move, exit
            direction = TRUE;
            break;
        }
    }

    return direction;
}

/**
 * @brief 7d230 | 4c | Selects the first non empty file
 * 
 * @param pFileNumber File number pointer
 */
static void FileSelectFindFirstNonEmptyFile(FileSelectCursorPosition* pFileNumber)
{
    u8 file;
    u8 flags;

    flags = FILE_SELECT_DATA.enabledMenus;
    file = *pFileNumber;
    if ((flags >> file) & 1)
        return;

    if (flags & 1 << FILE_SELECT_CURSOR_POSITION_FILE_A)
        file = FILE_SELECT_CURSOR_POSITION_FILE_A;
    else if (flags & 1 << FILE_SELECT_CURSOR_POSITION_FILE_B)
        file = FILE_SELECT_CURSOR_POSITION_FILE_B;
    else if (flags & 1 << FILE_SELECT_CURSOR_POSITION_FILE_C)
        file = FILE_SELECT_CURSOR_POSITION_FILE_C;

    *pFileNumber = file;
}

/**
 * @brief 7d27c | 30c | Updates the current sub menu of the file select screen
 * 
 * @return u8 bool, leaving
 */
static u8 FileSelectUpdateSubMenu(void)
{
    u8 result;
    u8 cursorPose;
    s32 temp;

    switch (FILE_SELECT_DATA.currentSubMenu)
    {
        case FILE_SELECT_SUB_MENU_MAIN:
            result = 0;
#ifdef REGION_EU
            CheckForMaintainedInput(MAINTAINED_INPUT_SPEED_FAST);
#else // !REGION_EU
            CheckForMaintainedInput();
#endif // REGION_EU

            if (gChangedInput)
            {
                if (FileSelectApplyMenuSelectInput(FILE_SELECT_SELECTION_SET_ALL_FILES,
                    &FILE_SELECT_DATA.fileSelectCursorPosition))
                {
                    result = 1;
                }
                else
                {
                    if (gChangedInput & KEY_B)
                    {
                        result = 2;
                    }
                    else if (gChangedInput & KEY_A)
                    {
                        result = 3;
                    }
#ifdef DEBUG
                    else if (gButtonInput & KEY_SELECT)
                    {
                        if (FILE_SELECT_DATA.fileSelectCursorPosition < FILE_SELECT_CURSOR_POSITION_COPY &&
                            (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].exists ||
                            gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].introPlayed))
                        {
                            if (gChangedInput & KEY_RIGHT)
                            {
                                gDifficulty = DIFF_NORMAL;
                                result = 1;
                            }
                            else if (gChangedInput & KEY_LEFT)
                            {
                                gDifficulty = DIFF_HARD;
                                result = 1;
                            }
                            else if (gChangedInput & KEY_L)
                            {
                                gDifficulty = DIFF_EASY;
                                result = 1;
                            }

                            if (result != 0)
                            {
                                // Mark file as completed
                                gMostRecentSaveFile = FILE_SELECT_DATA.fileSelectCursorPosition;
                                gLanguage = gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].language; 
                                gGameCompletion.completedGame = FALSE;

                                if (FILE_SELECT_DATA.fileSelectCursorPosition == 0)
                                    result = 0xC;
                                else if (FILE_SELECT_DATA.fileSelectCursorPosition == 1)
                                    result = 0xD;
                                else
                                    result = 0xE;
                                FILE_SELECT_DATA.fileScreenOam[result].exists = OAM_ID_CHANGED_FLAG;
                                gSramOperationStage = 0;

                                while (!SramProcessEndingSave_Debug());

                                SoundPlay(SOUND_GAME_BOY_BOOT);
                            }
                        }
                        
                        result = 0;
                    }
#endif // DEBUG
                }
            }

            if (!result) // 0 = no input
                break;

            if (result == 1) // 1 = moved cursor
            {
                FileSelectUpdateCursor(CURSOR_POSE_MOVING, FILE_SELECT_DATA.fileSelectCursorPosition);
                if (FILE_SELECT_DATA.fileSelectCursorPosition < FILE_SELECT_CURSOR_POSITION_COPY)
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CURSOR);
                else
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
            }
            else if (result == 2) // 2 = pressed B, go back to title
            {
                FadeMusic(0);
#ifdef REGION_JP
                if (FILE_SELECT_DATA.fileSelectCursorPosition < FILE_SELECT_CURSOR_POSITION_COPY &&
                    (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].exists ||
                    gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].introPlayed))
                {
                    gLanguage = gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].language;
                }
#endif // REGION_JP
                gSubGameMode2 = 3; // Go to title screen
                return TRUE;
            }
            else if (result == 3) // 3 = pressed A, open sub menu
            {
                result = FALSE;

                switch (FILE_SELECT_DATA.fileSelectCursorPosition)
                {
                    case FILE_SELECT_CURSOR_POSITION_FILE_A:
                    case FILE_SELECT_CURSOR_POSITION_FILE_B:
                    case FILE_SELECT_CURSOR_POSITION_FILE_C:
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_FILE_SELECT);
                        cursorPose = CURSOR_POSE_SELECTING_FILE;
                        result = TRUE;
                        FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_FILE;
                        break;

                    case FILE_SELECT_CURSOR_POSITION_COPY:
                        if (FILE_SELECT_DATA.enabledMenus & 1 << FILE_SELECT_CURSOR_POSITION_COPY)
                        {
                            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);
                            cursorPose = CURSOR_POSE_OPENING_OPTIONS;
                            result = TRUE;
                            FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_COPY;
                        }
                        break;

                    case FILE_SELECT_CURSOR_POSITION_ERASE:
                        if (FILE_SELECT_DATA.enabledMenus & 1 << FILE_SELECT_CURSOR_POSITION_ERASE)
                        {
                            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);
                            cursorPose = CURSOR_POSE_OPENING_OPTIONS;
                            result = TRUE;
                            FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_ERASE;
                        }
                        break;

                    case FILE_SELECT_CURSOR_POSITION_OPTIONS:
                        if (FILE_SELECT_DATA.enabledMenus & 1 << FILE_SELECT_CURSOR_POSITION_OPTIONS)
                        {
                            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);
                            cursorPose = CURSOR_POSE_OPENING_OPTIONS;
                            result = TRUE;
                            FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_ENTERING_OPTIONS;
                        }
                        break;
                }

                if (result)
                {
                    FileSelectUpdateCursor(cursorPose, FILE_SELECT_DATA.fileSelectCursorPosition);
                    FILE_SELECT_DATA.subMenuStage = 0;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                }
            }
            break;

        case FILE_SELECT_SUB_MENU_FILE:
            result = FileSelectProcessFileSelection();
            if (result == FALSE)
                break;

            if (result == 2)
            {
                temp = FILE_SELECT_DATA.unk_3A;
                if (temp >= 0)
                {
                    if (temp >= 2)
                    {
                        if (temp < 5)
                        {
                            gSaveFilesInfo[gMostRecentSaveFile].exists = FALSE;
                            gSaveFilesInfo[gMostRecentSaveFile].difficulty = FILE_SELECT_DATA.fileSelectCursors.difficulty;
                            gSaveFilesInfo[gMostRecentSaveFile].timeAttack = FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 2;

                            // On JP, the language is updated based on whether Japanese or hiragana was chosen.
                            // Debug allows any language, so it has an extra check.
#if defined(DEBUG) || defined(REGION_JP)
#if defined(DEBUG)
                            if (gSaveFilesInfo[gMostRecentSaveFile].language <= LANGUAGE_HIRAGANA)
#endif // DEBUG
                            {
                                gSaveFilesInfo[gMostRecentSaveFile].language = FILE_SELECT_DATA.fileSelectCursors.japaneseText;
                            }
#endif // DEBUG || REGION_JP
                        }
                    }
                }
                
                gSubGameMode2 = gSaveFilesInfo[gMostRecentSaveFile].exists ? 1 : 2;
                FadeMusic(ONE_THIRD_SECOND);
                return TRUE;
            }

            FileSelectUpdateCursor(CURSOR_POSE_DESELECTING_FILE, FILE_SELECT_DATA.fileSelectCursorPosition);
            FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
            break;

        case FILE_SELECT_SUB_MENU_COPY:
            if (FileSelectCopyFileHandler())
            {
                FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.fileSelectCursorPosition);
                FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
            }
            break;

        case FILE_SELECT_SUB_MENU_ERASE:
            if (FileSelectEraseFileHandler())
            {
                FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.fileSelectCursorPosition);
                FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
            }
            break;

        case FILE_SELECT_SUB_MENU_CORRUPTED:
            if (FileSelectCorruptedFileHandler())
            {
                FileSelectUpdateCursor(CURSOR_POSE_DEFAULT, FILE_SELECT_DATA.fileSelectCursorPosition);
                FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
            }
            break;

        case FILE_SELECT_SUB_MENU_ENTERING_OPTIONS:
            if (FileSelectOptionTransition(FALSE))
            {
                gSubGameMode2 = 0;
                return TRUE;
            }
            break;

        case FILE_SELECT_SUB_MENU_LEAVING_OPTIONS:
            if (FileSelectOptionTransition(TRUE))
                FILE_SELECT_DATA.currentSubMenu = FILE_SELECT_SUB_MENU_MAIN;
            break;
    }

    FileScreenProcessText();

    return FALSE;
}

/**
 * @brief 7d588 | a4 | Handles checking for time attack code input
 * 
 * @return u32 bool, fully entered
 */
static u32 FileSelectCheckInputtingTimeAttackCode(void)
{
    u16 input;

    input = gChangedInput;

    if (!FILE_SELECT_DATA.inputtingTimeAttack)
    {
        // Not inputting, update cooldown and check for first input
        if (FILE_SELECT_DATA.timeAttackInputCooldown != 0)
            APPLY_DELTA_TIME_DEC(FILE_SELECT_DATA.timeAttackInputCooldown);
        else
        {
            if (input == sTimeAttackButtonCode[0])
            {
                // Started inputting
                FILE_SELECT_DATA.inputtingTimeAttack = TRUE;
                FILE_SELECT_DATA.numberOfTimeAttackInputs = 1;
            }
            else if (input)
                FILE_SELECT_DATA.timeAttackInputCooldown = CONVERT_SECONDS(.5f);
        }
    }
    else
    {
        // Check is correct input
        if (input == sTimeAttackButtonCode[FILE_SELECT_DATA.numberOfTimeAttackInputs])
        {
            // Increment input counter
            FILE_SELECT_DATA.numberOfTimeAttackInputs++;
            
            // Check reached the end
            if (sTimeAttackButtonCode[FILE_SELECT_DATA.numberOfTimeAttackInputs] == USHORT_MAX)
                return TRUE;
        }
        else if (input)
        {
            // Invalid input detected, reset
            FILE_SELECT_DATA.inputtingTimeAttack = FALSE;
            FILE_SELECT_DATA.numberOfTimeAttackInputs = 0;
            FILE_SELECT_DATA.timeAttackInputCooldown = CONVERT_SECONDS(.5f);
        }
    }

    return FALSE;
}

/**
 * @brief 7d62c | dd0 | Handles sub menu when a file is selected (CurrentSubMenu = 1)
 * 
 * @return u8 Leaving
 */
static u8 FileSelectProcessFileSelection(void)
{
    u32 leaving;
    u32 offset;
    s32 action;
    struct FileSelectData *pFSD;
    struct SaveFileInfo *pSFI;
    u8 tmp;

    leaving = FALSE;
    FILE_SELECT_DATA.subMenuTimer++;

    switch (FILE_SELECT_DATA.subMenuStage)
    {
        case 0: // Initialize file sub menu
            gMostRecentSaveFile = FILE_SELECT_DATA.fileSelectCursorPosition;

            offset = (FILE_SELECT_DATA.fileSelectCursorPosition + 1) * 3;
            BitFill(3,0, (void*)sEwramPointer + 0x10C0, 0x240, 16);
            DmaTransfer(3, (void*)sEwramPointer + 0x800 + offset * 0x40, (void*)sEwramPointer + 0x1000 + offset * 0x40, 0xC0, 16);
            DmaTransfer(3, (void*)sEwramPointer + 0x1000, VRAM_BASE + 0xF000, 0x800, 16);

            FILE_SELECT_DATA.dispcnt |= DCNT_BG2;
            FILE_SELECT_DATA.dispcnt |= DCNT_WIN0;

#ifdef REGION_EU
            WRITE_16(REG_WIN0H, C_16_2_8(40, 200));
#else // !REGION_EU
            WRITE_16(REG_WIN0H, C_16_2_8(70, 170));
#endif // REGION_EU
            WRITE_16(REG_WIN0V, C_16_2_8(0, 23));
            WRITE_16(REG_WINOUT, C_16_2_8(0, WIN0_ALL));
            WRITE_8(REG_WININ, C_16_2_8(0, WIN0_ALL_NO_COLOR_EFFECT));

            gWrittenToBldalpha_H = 0;
            gWrittenToBldalpha_L = 16;

            FILE_SELECT_DATA.bldcnt = BLDCNT_BG1_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET;

            FILE_SELECT_DATA.unk_3A = 0;

            FILE_SELECT_DATA.fileSelectCursors = sFileSelectMenuCursors_Empty;

            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_A)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = TRUE;
            
            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_B)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = TRUE;

            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_C)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = TRUE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 1:
            action = TRUE;

            if (gWrittenToBldalpha_L != 0)
            {
                action = FALSE;
                if (gWrittenToBldalpha_L - 2 < 0)
                    gWrittenToBldalpha_L = 0;
                else
                    gWrittenToBldalpha_L -= 2;
            }

            if (gWrittenToBldalpha_H != 16)
            {
                action = FALSE;
                if (gWrittenToBldalpha_H + 2 > 16)
                    gWrittenToBldalpha_H = 16;
                else
                    gWrittenToBldalpha_H += 2;
            }

            if (!action)
                break;

            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_A)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].notDrawn = TRUE;
            
            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_B)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].notDrawn = TRUE;

            if (FILE_SELECT_DATA.fileSelectCursorPosition != FILE_SELECT_CURSOR_POSITION_FILE_C)
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].notDrawn = TRUE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 2:
            action = FILE_SELECT_DATA.fileSelectCursorPosition * 0x60 + 0x800;
            if (action != gBg2VOFS_NonGameplay)
            {
                if (gBg2VOFS_NonGameplay + 12 > action)
                    gBg2VOFS_NonGameplay = action;
                else
                    gBg2VOFS_NonGameplay += 12;

                break;
            }
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 3:
            DmaTransfer(3, (void*)sEwramPointer + 0x800, VRAM_BASE + 0xF000 + FILE_SELECT_DATA.fileSelectCursorPosition * 0xC0, 0xC0, 16);
            FILE_SELECT_DATA.dispcnt &= ~DCNT_WIN0;

            FileSelectUpdateTilemap(TILEMAP_REQUEST_START_GAME_INIT);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_START_GAME);
            
            FILE_SELECT_DATA.subMenuStage = 4;
            break;

        case 4:
            if (!FileSelectUpdateTilemap(TILEMAP_REQUEST_START_GAME))
                break;

        case 5:
            unk_7e3fc(0, FILE_SELECT_DATA.fileSelectCursors.startGame);
            FILE_SELECT_DATA.subMenuStage = 6;
            break;

        case 6:
            FILE_SELECT_DATA.inputtingTimeAttack = FALSE;
            FILE_SELECT_DATA.numberOfTimeAttackInputs = 0;
            FILE_SELECT_DATA.timeAttackInputCooldown = 0;
            FILE_SELECT_DATA.inputtedTimeAttack = FALSE;
            FILE_SELECT_DATA.subMenuStage = 7;

        case 7: // "start game" sub menu
            action = UCHAR_MAX;

            if (gChangedInput & KEY_A)
            {
                action = 0x80;
                FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);

                pSFI = (struct SaveFileInfo*)gSaveFilesInfo;
                pFSD = &FILE_SELECT_DATA;
                if (!pSFI[pFSD->fileSelectCursorPosition].completedGame &&
                    (pSFI[pFSD->fileSelectCursorPosition].exists ||
                     pSFI[pFSD->fileSelectCursorPosition].introPlayed))
                {
                    FILE_SELECT_DATA.unk_3A = 0;
                    FILE_SELECT_DATA.subMenuStage = 34;
                }
                else
                {
                    pFSD->subMenuStage = 8;
                }
            }
            else if (gChangedInput & KEY_B)
            {
                action = 0x81;
                FILE_SELECT_DATA.subMenuStage = 39;
            }
            else if (gFileScreenOptionsUnlocked.timeAttack & TRUE && gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].completedGame)
            {
                if (FileSelectCheckInputtingTimeAttackCode())
                {
                    action = 0x80;
                    FILE_SELECT_DATA.fileSelectCursors.completedFileOptions = 2;
                    FILE_SELECT_DATA.inputtedTimeAttack = TRUE;
                    FILE_SELECT_DATA.subMenuStage = 8;
                }
            }

            if (action + 1 != 0)
                unk_7e3fc(0, (u8)action);
            break;

        case 8: // Start game pressed, check if continue or new
            if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].exists || gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].introPlayed)
            {
                if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].timeAttack || FILE_SELECT_DATA.inputtedTimeAttack)
                    FILE_SELECT_DATA.continueNewMessage = FILE_SCREEN_MESSAGE_INFO_ID_CONTINUE_NEW_TIME_ATTACK;
                else
                    FILE_SELECT_DATA.continueNewMessage = FILE_SCREEN_MESSAGE_INFO_ID_CONTINUE_NEW;
                FILE_SELECT_DATA.subMenuStage = 9;
            }
            else
                FILE_SELECT_DATA.subMenuStage = 21;

            FILE_SELECT_DATA.subMenuTimer = 0;
            break;

        case 9:
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SELECT_DATA.continueNewMessage);
            FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_ERASE_FILE_CONFIRM);

            FileSelectUpdateTilemap(TILEMAP_REQUEST_29);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 10:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_30))
            {
                unk_7e3fc(5, FILE_SELECT_DATA.fileSelectCursors.completedFileOptions);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 11:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    action = 0x80;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);
                    FILE_SELECT_DATA.subMenuStage = 13;
                }
                else if (gChangedInput & KEY_B)
                {
                    FileSelectUpdateTilemap(TILEMAP_REQUEST_31);
                    action = 0x81;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.subMenuStage = 12;
                }
                else if (gChangedInput & KEY_UP)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions != 0)
                    {
                        action = --FILE_SELECT_DATA.fileSelectCursors.completedFileOptions;

                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_DOWN)
                {
                    if (FILE_SELECT_DATA.continueNewMessage == FILE_SCREEN_MESSAGE_INFO_ID_CONTINUE_NEW_TIME_ATTACK)
                    {
                        if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions < 2)
                        {
                            action = ++FILE_SELECT_DATA.fileSelectCursors.completedFileOptions;

                            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                        }
                    }
                    else if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 0)
                    {
                        action = ++FILE_SELECT_DATA.fileSelectCursors.completedFileOptions;

                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }

            if (action + 1 != 0)
                unk_7e3fc(5, (u8)action);
            break;

        case 12:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_32))
            {
                if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 2)
                    FILE_SELECT_DATA.fileSelectCursors.completedFileOptions = 0;
                FILE_SELECT_DATA.subMenuStage = 5;
            }
            break;

        case 13:
            FILE_SELECT_DATA.subMenuTimer = 0;

            if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 0)
            {
                FILE_SELECT_DATA.unk_3A = 1;
                FILE_SELECT_DATA.subMenuStage = 34;
            }
            else
            {
                FILE_SELECT_DATA.subMenuStage = 14;
            }
            break;

        case 14:
            if (FILE_SELECT_DATA.subMenuTimer > 10)
                FILE_SELECT_DATA.subMenuStage = 15;
            break;

        case 15:
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 16:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_34))
            {
                FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted = sFileSelectMenuCursors_Empty.confirmOverwritingCompleted;
                unk_7e3fc(6, FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 17:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    action = 0x80;

                    if (FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted != 0)
                        SoundPlay(SOUND_REFUSE_MENU);
                    else
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);

                    FILE_SELECT_DATA.subMenuStage = 18;
                }
                else if (gChangedInput & KEY_B)
                {
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    action = 0x81;
                    SoundPlay(SOUND_REFUSE_MENU);
                    FILE_SELECT_DATA.subMenuStage = 19;
                }
                else if (gChangedInput & KEY_LEFT)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted != 0)
                    {
                        action = 0;
                        FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted = 0;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_RIGHT)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted == 0)
                    {
                        action = 1;
                        FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted = 1;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }

            if (action + 1 != 0)
                unk_7e3fc(6, (u8)action);
            break;

        case 18:
            if (FILE_SELECT_DATA.subMenuTimer <= 10)
                break;

            if (FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted != 0)
            {
                FILE_SELECT_DATA.subMenuStage = 19;
                break;
            }

            // When starting a new game on a completed file, JP always shows another menu
            // (for Japanese/hiragana), but on non-JP the game will start if on a time
            // attack file (since the difficulty menu is skipped). Debug allows any language,
            // so it has an extra check.
#if defined(DEBUG) || !defined(REGION_JP)
#if defined(DEBUG)
            action = TRUE;
            if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].language >= LANGUAGE_ENGLISH)
#endif // DEBUG
            {
                action = FILE_SELECT_DATA.fileSelectCursors.completedFileOptions != 2;
            }

            if (action)
#endif // DEBUG || !REGION_JP
            {
                unk_7e3fc(6, 0x81);
                FileSelectUpdateTilemap(TILEMAP_REQUEST_35);
                FILE_SELECT_DATA.subMenuStage = 20;
            }
#if defined(DEBUG) || !defined(REGION_JP)
            else
            {
                FILE_SELECT_DATA.unk_3A = 2;
                FILE_SELECT_DATA.subMenuStage = 34;
            }
#endif // DEBUG || !REGION_JP
            break;

        case 19:
            FileSelectUpdateTilemap(TILEMAP_REQUEST_37);
            unk_7e3fc(5, FILE_SELECT_DATA.fileSelectCursors.completedFileOptions);
            FILE_SELECT_DATA.subMenuStage = 11;
            break;

        case 20:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_36))
                FILE_SELECT_DATA.subMenuStage = 21;
            break;

        case 21:
            // JP goes to the Japanese/hiragana menu, while non-JP goes to the difficulty menu.
            // Debug allows any language, so it checks the language to decide the next menu.
#if defined(DEBUG) || defined(REGION_JP)
#if defined(DEBUG)
            if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].language <= LANGUAGE_HIRAGANA)
#endif // DEBUG
            {
                FILE_SELECT_DATA.subMenuStage = 22;
                FileScreenUpdateMessageInfoIdQueue(0, FILE_SCREEN_MESSAGE_INFO_ID_MESSAGE_OPTION);
            }
#endif // DEBUG || REGION_JP
#if defined(DEBUG) || !defined(REGION_JP)
#if defined(DEBUG)
            else
#endif // DEBUG
            {
                FILE_SELECT_DATA.subMenuStage = 28;
            }
#endif // DEBUG || !REGION_JP

            if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 2)
            {
                FILE_SELECT_DATA.fileSelectCursors.difficulty = 1;
            }
            else
            {
                if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].completedGame & 0x36)
                    FILE_SELECT_DATA.difficultyMessage = 0x13;
                else
                    FILE_SELECT_DATA.difficultyMessage = 0x12;

                FileScreenUpdateMessageInfoIdQueue(0, FILE_SELECT_DATA.difficultyMessage);
            }
            break;

        case 22:
            FileSelectUpdateTilemap(TILEMAP_REQUEST_2);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 23:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_3))
            {
                unk_7e3fc(1, FILE_SELECT_DATA.fileSelectCursors.japaneseText);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 24:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    action = 0x80;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FileSelectPlayMenuSound(MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU);
                    FILE_SELECT_DATA.subMenuStage = 26;
                }
                else if (gChangedInput & KEY_B)
                {
                    FileSelectUpdateTilemap(TILEMAP_REQUEST_4);
                    action = 0x81;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.subMenuStage = 25;
                }
                else if (gChangedInput & KEY_UP)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.japaneseText != 0)
                    {
                        action = 0;
                        FILE_SELECT_DATA.fileSelectCursors.japaneseText = 0;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_DOWN)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.japaneseText == 0)
                    {
                        action = 1;
                        FILE_SELECT_DATA.fileSelectCursors.japaneseText = 1;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }

            if (action + 1 != 0)
                unk_7e3fc(1, (u8)action);
            break;

        case 25:
            if (!FileSelectUpdateTilemap(TILEMAP_REQUEST_5))
                break;

            if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].completedGame == 0)
            {
                FILE_SELECT_DATA.subMenuStage = 5;
                break;
            }

            FILE_SELECT_DATA.subMenuStage = 8;
            break;

        case 26:
            if (FILE_SELECT_DATA.subMenuTimer <= 10)
                break;

            if (FILE_SELECT_DATA.fileSelectCursors.completedFileOptions == 2)
            {
                FILE_SELECT_DATA.unk_3A = 3;
                FILE_SELECT_DATA.subMenuStage = 34;
                break;
            }

            FileSelectUpdateTilemap(TILEMAP_REQUEST_4);
            unk_7e3fc(1, 0x81);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 27:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_5))
            {
                FILE_SELECT_DATA.subMenuStage = 28;
                FILE_SELECT_DATA.fileSelectCursors.difficulty = FILE_SELECT_DATA.fileSelectCursors.japaneseText ^ 1;
            }
            break;

        case 28:
            FileSelectUpdateTilemap(TILEMAP_REQUEST_DIFFICULTY_SPAWN_INIT);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 29:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_DIFFICULTY_SPAWN))
            {
                unk_7e3fc(2, FILE_SELECT_DATA.fileSelectCursors.difficulty);
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 30:
            action = UCHAR_MAX;

            if (gChangedInput)
            {
                if (gChangedInput & KEY_A)
                {
                    action = 0x80;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.unk_3A = 4;
                    FILE_SELECT_DATA.subMenuStage = 34;
                }
                else if (gChangedInput & KEY_B)
                {
                    FileSelectUpdateTilemap(TILEMAP_REQUEST_DIFFICULTY_DESPAWN_INIT);
                    action = 0x81;
                    FILE_SELECT_DATA.subMenuTimer = 0;
                    FILE_SELECT_DATA.subMenuStage = 31;
                }
                else if (gChangedInput & KEY_UP)
                {
                    if (FILE_SELECT_DATA.fileSelectCursors.difficulty != 0)
                    {
                        action = --FILE_SELECT_DATA.fileSelectCursors.difficulty;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
                else if (gChangedInput & KEY_DOWN)
                {
                    if (FILE_SELECT_DATA.difficultyMessage == 0x13)
                    {
                        if (FILE_SELECT_DATA.fileSelectCursors.difficulty <= 1)
                        {
                            action = ++FILE_SELECT_DATA.fileSelectCursors.difficulty;
                            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                        }
                    }
                    else if (FILE_SELECT_DATA.fileSelectCursors.difficulty == 0)
                    {
                        action = ++FILE_SELECT_DATA.fileSelectCursors.difficulty;
                        FileSelectPlayMenuSound(MENU_SOUND_REQUEST_SUB_MENU_CURSOR);
                    }
                }
            }

            if (action + 1 != 0)
                unk_7e3fc(2, (u8)action);
            break;

        case 31:
            if (FileSelectUpdateTilemap(TILEMAP_REQUEST_DIFFICULTY_DESPAWN))
            {
                // When returning from the difficulty menu, JP goes to the Japanese/hiragana menu,
                // while non-JP goes to the "Start Game" menu. Debug allows any language, so it
                // checks the language to decide the next menu.
#if defined(DEBUG) || defined(REGION_JP)
#if defined(DEBUG)
                if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].language <= LANGUAGE_HIRAGANA)
#endif // DEBUG
                {
                    FILE_SELECT_DATA.subMenuStage = 22;
                }
#endif // DEBUG || REGION_JP
#if defined(DEBUG) || !defined(REGION_JP)
#if defined(DEBUG)
                else
#endif // DEBUG
                {
                    if (gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].exists)
                        FILE_SELECT_DATA.subMenuStage = 8;
                    else
                        FILE_SELECT_DATA.subMenuStage = 6;
                }
#endif // DEBUG || !REGION_JP
            }
            break;

        case 34:
            FileSelectUpdateCursor(CURSOR_POSE_STARTING_GAME, FILE_SELECT_DATA.fileSelectCursorPosition);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_START_GAME);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 35:
            if (gChangedInput & KEY_B)
            {
                FILE_SELECT_DATA.subMenuStage = 37;
                break;
            }

            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].ended)
            {
                FILE_SELECT_DATA.subMenuTimer = 0;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 36:
            if (gChangedInput & KEY_B)
            {
                FILE_SELECT_DATA.subMenuStage = 37;
                break;
            }

            if (FILE_SELECT_DATA.subMenuTimer > 16)
                leaving = 2;
            break;

        case 37:
            FileSelectUpdateCursor(0x8, FILE_SELECT_DATA.fileSelectCursorPosition);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU2);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 38:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_CURSOR].oamId != sFileSelectCursorOamData[FILE_SELECT_DATA.fileSelectCursorPosition].oamIds[gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].suitType] + 3)
                break;

            switch (FILE_SELECT_DATA.unk_3A)
            {
                case 0:
                    FILE_SELECT_DATA.subMenuStage = 5;
                    break;

                case 1:
                    unk_7e3fc(5, FILE_SELECT_DATA.fileSelectCursors.completedFileOptions);
                    FILE_SELECT_DATA.subMenuStage = 11;
                    break;

                case 2:
                    unk_7e3fc(6, FILE_SELECT_DATA.fileSelectCursors.confirmOverwritingCompleted);
                    FILE_SELECT_DATA.subMenuStage = 17;
                    break;

                case 3:
                    unk_7e3fc(1, FILE_SELECT_DATA.fileSelectCursors.japaneseText);
                    FILE_SELECT_DATA.subMenuStage = 24;
                    break;

                case 4:
                    unk_7e3fc(2, FILE_SELECT_DATA.fileSelectCursors.difficulty);
                    FILE_SELECT_DATA.subMenuStage = 30;
                    break;
            }

            break;

        case 39:
            FILE_SELECT_DATA.bldcnt = BLDCNT_BG1_FIRST_TARGET_PIXEL | BLDCNT_ALPHA_BLENDING_EFFECT | BLDCNT_SCREEN_SECOND_TARGET;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].oamId = FILE_SELECT_OAM_ID_SMALL_PANEL_CLOSE;

            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 40:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].oamId == 0)
            {
                FILE_SELECT_DATA.dispcnt |= DCNT_WIN0;
                FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_18;
                gBg1HOFS_NonGameplay = BLOCK_SIZE * 32;
                gBg1VOFS_NonGameplay = BLOCK_SIZE * 32;
                FILE_SELECT_DATA.subMenuStage++;
            }
            break;

        case 41:
            BitFill(3, 0, VRAM_BASE + 0xF000 + FILE_SELECT_DATA.fileSelectCursorPosition * 0xC0, 0xC0, 16);
            FILE_SELECT_DATA.subMenuStage++;

        case 42:
            action = BLOCK_SIZE * 32;
            if (action != gBg2VOFS_NonGameplay)
            {
                if (gBg2VOFS_NonGameplay - 12 < action)
                    gBg2VOFS_NonGameplay = action;
                else
                    gBg2VOFS_NonGameplay -= 12;
                break;
            }

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_MARKER].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_MARKER].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_MARKER].notDrawn = FALSE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 43:
            action = TRUE;

            if (gWrittenToBldalpha_H != 0)
            {
                action = FALSE;
                if (gWrittenToBldalpha_H - 2 < 0)
                    gWrittenToBldalpha_H = 0;
                else
                    gWrittenToBldalpha_H -= 2;
            }

            if (gWrittenToBldalpha_L != 16)
            {
                action = FALSE;
                if (gWrittenToBldalpha_L + 2 > 16)
                    gWrittenToBldalpha_L = 16;
                else
                    gWrittenToBldalpha_L += 2;
            }
            if (!action)
                break;

            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG2;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_A_LOGO].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_B_LOGO].notDrawn = FALSE;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_FILE_C_LOGO].notDrawn = FALSE;

            FILE_SELECT_DATA.subMenuStage++;
            break;

        case 44:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_WIN0;
            leaving = TRUE;
            break;
    }

    return leaving;
}

/**
 * @brief 7e3fc | 2dc | To document
 * 
 * @param param_1 To document
 * @param param_2 To document
 */
static void unk_7e3fc(u8 param_1, u8 param_2)
{
    switch (param_1)
    {
        case 0:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
                break;
            }
            
            if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].exists = FALSE;
                break;
            }

            if (param_2 == 0x0)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_MID_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].boundBackground = 1;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].xPosition = BLOCK_SIZE * 2;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].yPosition = BLOCK_SIZE * 2 - 8;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SUB_CURSOR], FILE_SELECT_OAM_ID_CURSOR_IDLE);
            }
            break;

        case 1:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
                break;
            }
            
            if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].exists = FALSE;
                break;
            }

            if (param_2 == 0x0 || param_2 == 0x1)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].boundBackground = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 2;

                if (param_2 == 0x0)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 2 + 24;
                else
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 3 + 24;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_IDLE);
            }
            break;

        case 2:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
            }
            
            else if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].exists = FALSE;
            }

            else if (param_2 == 0 || param_2 == 1 || param_2 == 2)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].boundBackground = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 2;

                if (param_2 == 0)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 2 + 24;
                else if (param_2 == 1)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 3 + 24;
                else
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 4 + 24;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_IDLE);
            }
            break;

        case 3:
        case 4:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
            }
            
            else if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].exists = FALSE;
            }

            else if (param_2 == 0 || param_2 == 1)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].boundBackground = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 3 + 24;

                if (param_2 == 0)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 3 + QUARTER_BLOCK_SIZE;
                else
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 6 - QUARTER_BLOCK_SIZE;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_IDLE);
            }
            break;

        case 5:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
            }
            
            else if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].exists = FALSE;
            }

            else if (param_2 == 0 || param_2 == 1 || param_2 == 2)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].boundBackground = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE + HALF_BLOCK_SIZE + 12;

                if (param_2 == 0)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE + 24;
                else if (param_2 == 1)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 2 + 24;
                else
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 3 + 24;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_IDLE);
            }
            break;

        case 6:
            if (param_2 == 0x80)
            {
                UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_SELECTED);
            }
            
            else if (param_2 == 0x81)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].exists = FALSE;
            }

            else if (param_2 == 0 || param_2 == 1)
            {
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].priority = BGCNT_HIGH_PRIORITY;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].boundBackground = 0;
                FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].yPosition = BLOCK_SIZE * 4 + HALF_BLOCK_SIZE - 8;

                if (param_2 == 0)
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 3 - 8;
                else
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].xPosition = BLOCK_SIZE * 6 - QUARTER_BLOCK_SIZE;

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3].oamId != FILE_SELECT_OAM_ID_CURSOR_IDLE)
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_3], FILE_SELECT_OAM_ID_CURSOR_IDLE);

                if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0].oamId != FILE_SELECT_OAM_ID_METROID_LOGO)
                {
                    UpdateMenuOamDataId(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0], FILE_SELECT_OAM_ID_METROID_LOGO);
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0].xPosition = BLOCK_SIZE * 2 - 8;
                    FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_0].yPosition = BLOCK_SIZE * 3 + QUARTER_BLOCK_SIZE;
                }
            }
            break;
    }
}

u32 FileSelectUpdateTilemap(u8 request)
{
    u32 ended;

    ended = TRUE;

    switch (request)
    {
        case TILEMAP_REQUEST_START_GAME_INIT:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].xPosition = BLOCK_SIZE * 4;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].yPosition = BLOCK_SIZE * 2;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL], FILE_SELECT_OAM_ID_SMALL_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);
            gBG1HOFS_NonGameplay = BLOCK_SIZE * 31;
            gBG1VOFS_NonGameplay = BLOCK_SIZE * 29 + HALF_BLOCK_SIZE + 8;

            DMATransfer(3, (void*)sEwramPointer + 0x3000, VRAM_BASE + 0xE800, 0x300, 16);
            break;

        case TILEMAP_REQUEST_START_GAME:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].ended && unk_790cc(1, 0x0))
            {
                FILE_SELECT_DATA.bldcnt = 0;
                FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
                break;
            }
            ended = FALSE;
            break;

        case 2:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].yPosition = BLOCK_SIZE * 2 + HALF_BLOCK_SIZE;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL], FILE_SELECT_OAM_ID_MEDIUM_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);
            gBG0HOFS_NonGameplay = BLOCK_SIZE * 28;
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 28 + HALF_BLOCK_SIZE;

            DMATransfer(3, (void*)sEwramPointer + 0x3900, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case 3:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].ended && unk_790cc(1, 0x11))
            {
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                break;
            }
            ended = FALSE;
            break;

        case 4:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID = FILE_SELECT_OAM_ID_MEDIUM_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case TILEMAP_REQUEST_DIFFICULTY_SPAWN_INIT:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].yPosition = BLOCK_SIZE * 3;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL], FILE_SELECT_OAM_ID_LARGE_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);
            gBG0HOFS_NonGameplay = BLOCK_SIZE * 28;
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 28 + HALF_BLOCK_SIZE;
            break;

        case TILEMAP_REQUEST_DIFFICULTY_SPAWN:
            if (!FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].ended)
            {
                ended = FALSE;
                break;
            }
            if (unk_790cc(1, FILE_SELECT_DATA.unk_38))
            {
                unk_7eedc((void*)sEwramPointer + 0x3C00);
                DMATransfer(3, (void*)sEwramPointer + 0x3C00, VRAM_BASE + 0xE000, 0x300, 16);
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                break;
            }
            ended = FALSE;
            break;

        case TILEMAP_REQUEST_DIFFICULTY_DESPAWN_INIT:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID = FILE_SELECT_OAM_ID_LARGE_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case TILEMAP_REQUEST_ERASE_SPAWN_INIT:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].yPosition = BLOCK_SIZE * 2 + HALF_BLOCK_SIZE;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL], FILE_SELECT_OAM_ID_MEDIUM_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);

            gBG1HOFS_NonGameplay = BLOCK_SIZE * 27 + 8;
            gBG1VOFS_NonGameplay = BLOCK_SIZE * 26 + HALF_BLOCK_SIZE + 8;

            DMATransfer(3, (void*)sEwramPointer + 0x3F00, VRAM_BASE + 0xE800, 0x300, 16);

            FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
            FILE_SELECT_DATA.bg2cnt = FILE_SELECT_DATA.unk_1A;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG2;
            break;

        case TILEMAP_REQUEST_ERASE_SPAWN:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].ended && unk_790cc(1, 0x5))
            {
                FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
                break;
            }
            ended = FALSE;
            break;

        case TILEMAP_REQUEST_ERASE_DESPAWN_INIT:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].oamID = FILE_SELECT_OAM_ID_MEDIUM_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case TILEMAP_REQUEST_ERASE_YES_NO_SPAWN_INIT:
            /* Both paths intentionally perform the same store. The condition
             * is always false; agbcc's late removal of the redundant path
             * preserves the target's temporary register allocation.
             */
            if (((u32)&gBG0HOFS_NonGameplay << 2) & 3)
            {
                gBG0HOFS_NonGameplay = BLOCK_SIZE * 27 + 8;
            }
            else
            {
                gBG0HOFS_NonGameplay = BLOCK_SIZE * 27 + 8;
            }
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 26 + HALF_BLOCK_SIZE + 8;

            DMATransfer(3, (void*)sEwramPointer + 0x4200, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case TILEMAP_REQUEST_ERASE_YES_NO_SPAWN:
            if (unk_790cc(1, 0x6))
            {
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
                break;
            }
            ended = FALSE;
            break;

        case TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN_INIT:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
            break;

        case TILEMAP_REQUEST_COPY_SPAWN_INIT:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].yPosition = BLOCK_SIZE * 2 + HALF_BLOCK_SIZE;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL], FILE_SELECT_OAM_ID_MEDIUM_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);

            gBG1HOFS_NonGameplay = BLOCK_SIZE * 27 + 8;
            gBG1VOFS_NonGameplay = BLOCK_SIZE * 26 + HALF_BLOCK_SIZE + 8;

            DMATransfer(3, (void*)sEwramPointer + 0x3F00, VRAM_BASE + 0xE800, 0x300, 16);
            FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
            FILE_SELECT_DATA.bg2cnt = FILE_SELECT_DATA.unk_1A;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG2;
            break;

        case TILEMAP_REQUEST_COPY_SPAWN:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].ended && unk_790cc(1, 0x1))
            {
                FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_1C;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
                break;
            }
            ended = FALSE;
            break;

        case TILEMAP_REQUEST_COPY_DESPAWN_INIT:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].oamID = FILE_SELECT_OAM_ID_MEDIUM_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case TILEMAP_REQUEST_ERASE_DESPAWN:
        case TILEMAP_REQUEST_COPY_DESPAWN:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_SMALL_PANEL].oamID == 0)
            {
                FILE_SELECT_DATA.bg1cnt = FILE_SELECT_DATA.unk_18;
                FILE_SELECT_DATA.bg2cnt = FILE_SELECT_DATA.unk_16;

                FILE_SELECT_DATA.dispcnt &= ~DCNT_BG2;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG1;

                gBG1HOFS_NonGameplay = BLOCK_SIZE * 32;
                gBG1VOFS_NonGameplay = BLOCK_SIZE * 32;
                ended = TRUE;
                break;
            }
            else
            {
                ended = FALSE;
                break;
            }

        case TILEMAP_REQUEST_ERASE_YES_NO_DESPAWN:
        case TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN:
            ended = TRUE;
            break;

        case TILEMAP_REQUEST_COPY_OVERRIDE_SPAWN_INIT:
            gBG0HOFS_NonGameplay = BLOCK_SIZE * 27 + 8;
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 26 + HALF_BLOCK_SIZE + 8;

            DMATransfer(3, (void*)sEwramPointer + 0x4200, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case TILEMAP_REQUEST_COPY_OVERRIDE_SPAWN:
            if (unk_790cc(1, 0x3))
            {
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                FILE_SELECT_DATA.dispcnt &= ~DCNT_BG1;
                break;
            }
            ended = FALSE;
            break;

        case TILEMAP_REQUEST_COPY_OVERRIDE_DESPAWN_INIT:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.dispcnt |= DCNT_BG1;
            break;

        case TILEMAP_REQUEST_COPY_DESTINATION_DESPAWN:
            DMATransfer(3, (void*)sEwramPointer + 0x3F00, VRAM_BASE + 0xE800, 0x300, 16);
            break;

        case TILEMAP_REQUEST_COPY_DESTINATION_SPAWN:
            DMATransfer(3, (void*)sEwramPointer + 0x4500, VRAM_BASE + 0xE800, 0x300, 16);
            break;

        case 0x1C:
            DMATransfer(3, (void*)sEwramPointer + 0x4200, VRAM_BASE + 0xE800, 0x300, 16);
            break;

        case 0x1D:
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].yPosition = BLOCK_SIZE * 3;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL], FILE_SELECT_OAM_ID_LARGE_PANEL);
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_OPEN_SUB_MENU);

            gBG0HOFS_NonGameplay = BLOCK_SIZE * 28;
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 28 + HALF_BLOCK_SIZE;

            DMATransfer(3, (void*)sEwramPointer + 0x3300, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case 0x1E:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].ended && unk_790cc(1, FILE_SELECT_DATA.unk_39))
            {
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                break;
            }
            ended = FALSE;
            break;

        case 0x1F:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID = FILE_SELECT_OAM_ID_LARGE_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case 0x22:
            if (unk_790cc(1, FILE_SELECT_DATA.unk_39))
            {
                DMATransfer(3, (void*)sEwramPointer + 0x3600, VRAM_BASE + 0xE000, 0x300, 16);
                break;
            }
            ended = FALSE;
            break;

        case 0x23:
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.fileScreenOam[0].oamID = 0;
            FILE_SELECT_DATA.fileScreenOam[0].exists = FALSE;

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID = FILE_SELECT_OAM_ID_LARGE_PANEL + 1;
            FileSelectPlayMenuSound(MENU_SOUND_REQUEST_CLOSE_SUB_MENU);
            break;

        case 0x25:
            FILE_SELECT_DATA.fileScreenOam[0].oamID = 0;
            FILE_SELECT_DATA.fileScreenOam[0].exists = FALSE;

            DMATransfer(3, (void*)sEwramPointer + 0x3300, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case 0x26:
            SoundPlay(0x1FD);

            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].xPosition = BLOCK_SIZE * 5;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].yPosition = BLOCK_SIZE * 2 + HALF_BLOCK_SIZE;
            UpdateMenuOamDataID(&FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL], FILE_SELECT_OAM_ID_MEDIUM_PANEL);

            gBG0HOFS_NonGameplay = BLOCK_SIZE * 29 + HALF_BLOCK_SIZE;
            gBG0VOFS_NonGameplay = BLOCK_SIZE * 29 + HALF_BLOCK_SIZE;

            if (FILE_SELECT_DATA.unk_47 == 1)
                DMATransfer(3, (void*)sEwramPointer + 0x4200, VRAM_BASE + 0xE000, 0x300, 16);
            else if (FILE_SELECT_DATA.unk_47 == 2)
                DMATransfer(3, (void*)sEwramPointer + 0x4500, VRAM_BASE + 0xE000, 0x300, 16);
            else
                DMATransfer(3, (void*)sEwramPointer + 0x3F00, VRAM_BASE + 0xE000, 0x300, 16);
            break;

        case 0x27:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].ended)
            {
                FILE_SELECT_DATA.bg0cnt = FILE_SELECT_DATA.unk_1E;
                FILE_SELECT_DATA.dispcnt |= DCNT_BG0;
                break;
            }
            ended = FALSE;
            break;

        case 0x28:
            SoundPlay(0x1FE);
            FILE_SELECT_DATA.dispcnt &= ~DCNT_BG0;
            FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID = FILE_SELECT_OAM_ID_MEDIUM_PANEL + 1;
            break;

        /* Keep these identical case bodies separate: merging the labels
         * changes agbcc's allocation of the function's saved registers.
         */
        case 5:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID != 0)
                ended = FALSE;
            else
                ended = TRUE;
            break;
        case TILEMAP_REQUEST_DIFFICULTY_DESPAWN:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID != 0)
                ended = FALSE;
            else
                ended = TRUE;
            break;
        case 0x20:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID != 0)
                ended = FALSE;
            else
                ended = TRUE;
            break;
        case 0x24:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID != 0)
                ended = FALSE;
            else
                ended = TRUE;
            break;
        case 0x29:
            if (FILE_SELECT_DATA.fileScreenOam[FILE_SELECT_OAM_MEDIUM_PANEL].oamID != 0)
                ended = FALSE;
            else
                ended = TRUE;
            break;
    }

    return ended;
}

/**
 * @brief 7eedc | a0 | To document
 * 
 * @param pTilemap Tilemap pointer
 */
static void unk_7eedc(u16* pTilemap)
{
    s32 i;
    s32 j;
    u16 baseTile;
    u16 completion;
    u16 flags;

    completion = gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].completedGame & 7;

    flags = gSaveFilesInfo[FILE_SELECT_DATA.fileSelectCursorPosition].completedGame;
    completion |= (flags >> 3) & 7;

    pTilemap = &pTilemap[128];
    for (i = 0; i < 3; i++)
    {
        baseTile = (completion >> i) & 1 ? 14 << 12 : 15 << 12;

        for (j = 2; j < 2 + 16; j++)
        {
            pTilemap[i * 64 + j] &= 0x3FF;
            pTilemap[i * 64 + j] |= baseTile;

            pTilemap[i * 64 + 32 + j] &= 0x3FF;
            pTilemap[i * 64 + 32 + j] |= baseTile;
        }
    }
}

static u16 sMenuSounds[MENU_SOUND_REQUEST_COUNT] = {
    [MENU_SOUND_REQUEST_SUB_MENU_CURSOR] = SOUND_SUB_MENU_CURSOR,
    [MENU_SOUND_REQUEST_ACCEPT_CONFIRM_MENU] = SOUND_ACCEPT_CONFIRM_MENU,
    [MENU_SOUND_REQUEST_CURSOR] = SOUND_MENU_CURSOR,
    [MENU_SOUND_REQUEST_FILE_SELECT] = SOUND_FILE_SELECT,
    [MENU_SOUND_REQUEST_START_GAME] = SOUND_START_GAME,
    [MENU_SOUND_REQUEST_OPEN_SUB_MENU] = SOUND_OPEN_SUB_MENU,
    [MENU_SOUND_REQUEST_CLOSE_SUB_MENU] = SOUND_CLOSE_SUB_MENU,
    [MENU_SOUND_REQUEST_CLOSE_SUB_MENU2] = SOUND_CLOSE_SUB_MENU,
    [MENU_SOUND_REQUEST_COPY_DELETE] = SOUND_FILE_SELECT_COPY_DELETE,
    [MENU_SOUND_REQUEST_COPY_DELETE_MOVING] = SOUND_FILE_SELECT_COPY_MOVING,
    [MENU_SOUND_REQUEST_COPY_CONFIRM] = SOUND_FILE_SELECT_COPY_CONFIRM,
    [MENU_SOUND_REQUEST_GAME_OVER_MENU_CURSOR] = SOUND_MENU_CURSOR,
    [MENU_SOUND_REQUEST_GAME_OVER_START_GAME] = SOUND_START_GAME
};

/**
 * @brief 7ef7c | 20 | Plays a menu sound
 * 
 * @param request Sound request
 */
void FileSelectPlayMenuSound(MenuSoundRequest request)
{
    if (request < ARRAY_SIZE(sMenuSounds))
        SoundPlay(sMenuSounds[request]);
}
