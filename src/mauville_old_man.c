#include "global.h"
#include "main.h"
#include "constants/songs.h"
#include "constants/easy_chat.h"
#include "constants/map_objects.h"
#include "constants/vars.h"
#include "mauville_old_man.h"
#include "event_data.h"
#include "string_util.h"
#include "text.h"
#include "easy_chat.h"
#include "script.h"
#include "random.h"
#include "event_scripts.h"
#include "task.h"
#include "menu.h"
#include "m4a.h"
#include "bard_music.h"
#include "sound.h"
#include "strings.h"
#include "overworld.h"
#include "field_message_box.h"
#include "script_menu.h"

#define CHAR_SONG_WORD_SEPARATOR 0x37

extern struct MusicPlayerInfo gMPlayInfo_SE2;

void sub_81204DC(void);
void sub_81206C0(bool8 useTemporaryLyrics); // StartBardSong
void sub_8120944(u8 taskId);
void sub_8120B5C(void);
void sub_8120E08(void); // StorytellerSetup
void sub_8120E50(void);
void sub_81339F8(void); // TraderSetup
void sub_8133A60(void);

IWRAM_DATA u8 gUnknown_03001178;

struct BardSong gUnknown_03006130;

EWRAM_DATA u16 gUnknown_0203A128 = 0;
EWRAM_DATA struct MauvilleManStoryteller * gUnknown_0203A12C = NULL;
EWRAM_DATA u8 gUnknown_0203A130 = 0;

static const u16 sDefaultBardSongLyrics[6] = {
    EC_WORD_SHAKE,
    EC_WORD_IT,
    EC_WORD_DO,
    EC_WORD_THE,
    EC_WORD_DIET,
    EC_WORD_DANCE
};

static const u8 * const sGiddyAdjectives[] = {
    gText_SoPretty,
    gText_SoDarling,
    gText_SoRelaxed,
    gText_SoSunny,
    gText_SoDesirable,
    gText_SoExciting,
    gText_SoAmusing,
    gText_SoMagical
};

static const u8 * const sGiddyQuestions[] = {
    gUnknown_08294313,
    gUnknown_08294359,
    gUnknown_08294398,
    gUnknown_082943DA,
    gUnknown_0829441C,
    gUnknown_08294460,
    gUnknown_082944A0,
    gUnknown_082944D5
};

static void SetupBard(void)
{
    u16 i;
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    bard->id = MAUVILLE_MAN_BARD;
    bard->hasChangedSong = FALSE;
    bard->language = gGameLanguage;
    for (i = 0; i < 6; i++)
        bard->songLyrics[i] = sDefaultBardSongLyrics[i];
}

static void SetupHipster(void)
{
    struct MauvilleManHipster *hipster = &gSaveBlock1Ptr->oldMan.hipster;

    hipster->id = MAUVILLE_MAN_HIPSTER;
    hipster->alreadySpoken = FALSE;
    hipster->language = gGameLanguage;
}

static void SetupStoryteller(void)
{
    sub_8120E08();
}

static void SetupGiddy(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    giddy->id = MAUVILLE_MAN_GIDDY;
    giddy->taleCounter = 0;
    giddy->language = gGameLanguage;
}

static void SetupTrader(void)
{
    sub_81339F8();
}

void SetMauvilleOldMan(void)
{
    u16 trainerId = (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];


    // Determine man based on the last digit of the player's trainer ID.
    switch ((trainerId % 10) / 2)
    {
        case MAUVILLE_MAN_BARD:
            SetupBard();
            break;
        case MAUVILLE_MAN_HIPSTER:
            SetupHipster();
            break;
        case MAUVILLE_MAN_TRADER:
            SetupTrader();
            break;
        case MAUVILLE_MAN_STORYTELLER:
            SetupStoryteller();
            break;
        case MAUVILLE_MAN_GIDDY:
            SetupGiddy();
            break;
    }
    sub_8120B5C();
}

u8 sub_81201C8(void) // GetCurrentMauvilleOldMan
{
    struct MauvilleManCommon *common = &gSaveBlock1Ptr->oldMan.common;

    return common->id;
}

void sub_81201DC(void) // ScrSpecial_GetCurrentMauvilleMan
{
    gSpecialVar_Result = sub_81201C8();
}

void sub_81201F4(void) // ScrSpecial_HasBardSongBeenChanged
{
    u16 *scriptResult = &gSpecialVar_Result; // why??
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    *scriptResult = bard->hasChangedSong;
}

void sub_8120210(void) // ScrSpecial_SaveBardSongLyrics
{
    u16 i;
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    StringCopy(bard->playerName, gSaveBlock2Ptr->playerName);

    for (i = 0; i < 4; i++)
        bard->playerTrainerId[i] = gSaveBlock2Ptr->playerTrainerId[i];

    for (i = 0; i < 6; i++)
        bard->songLyrics[i] = bard->temporaryLyrics[i];

    bard->hasChangedSong = TRUE;
}

// Copies lyrics into gStringVar4
void sub_8120280(void)
{
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
    u16 specialVar = gSpecialVar_0x8004;  // It's a bit odd to use this temp variable, but it seems needed to match.
    u16 *lyrics;
    u16 lineNum;
    u8 *wordEnd;
    u8 *str;

    lyrics = bard->temporaryLyrics;
    if (specialVar == 0)
        lyrics = bard->songLyrics;
    wordEnd = gStringVar4;
    str = wordEnd;
    // Put three words on each line
    for (lineNum = 0; lineNum < 2; lineNum++)
    {
        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_SONG_WORD_SEPARATOR;
            str++;
        }

        str++;
        *(wordEnd++) = CHAR_SPACE;

        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_SONG_WORD_SEPARATOR;
            str++;
        }

        str++;
        *(wordEnd++) = CHAR_NEWLINE;

        wordEnd = CopyEasyChatWord(wordEnd, *(lyrics++));
        while (wordEnd != str)
        {
            if (*str == CHAR_SPACE)
                *str = CHAR_SONG_WORD_SEPARATOR;
            str++;
        }

        if (lineNum == 0)
        {
            *(wordEnd++) = EXT_CTRL_CODE_BEGIN;
            *(wordEnd++) = 15;
        }
    }
}

void sub_8120340(void) // ScrSpecial_PlayBardSong
{
    sub_81206C0(gSpecialVar_0x8004);
    ScriptContext1_Stop();
}

void sub_8120358(void) // ScrSpecial_GetHipsterSpokenFlag
{
    u16 *scriptResult = &gSpecialVar_Result; // again??
    struct MauvilleManHipster *hipster = &gSaveBlock1Ptr->oldMan.hipster;

    *scriptResult = hipster->alreadySpoken;
}

void sub_8120374(void) // ScrSpecial_SetHipsterSpokenFlag
{
    struct MauvilleManHipster *hipster = &gSaveBlock1Ptr->oldMan.hipster;

    hipster->alreadySpoken = TRUE;
}

void sub_812038C(void) // ScrSpecial_HipsterTeachWord
{
    u16 var = sub_811F01C();

    if (var == 0xFFFF)
    {
        gSpecialVar_Result = FALSE;
    }
    else
    {
        CopyEasyChatWord(gStringVar1, var);
        gSpecialVar_Result = TRUE;
    }
}

void sub_81203C4(void) // ScrSpecial_GiddyShouldTellAnotherTale
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    if (giddy->taleCounter == 10)
    {
        gSpecialVar_Result = FALSE;
        giddy->taleCounter = 0;
    }
    else
    {
        gSpecialVar_Result = TRUE;
    }
}

void sub_81203FC(void) // ScrSpecial_GenerateGiddyLine
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;

    if (giddy->taleCounter == 0)
        sub_81204DC();

    if (giddy->randomWords[giddy->taleCounter] != 0xFFFF) // is not the last element of the array?
    {
        u8 *stringPtr;
        u32 adjective = Random();

        adjective %= 8;
        stringPtr = CopyEasyChatWord(gStringVar4, giddy->randomWords[giddy->taleCounter]);
        stringPtr = StringCopy(stringPtr, gOtherText_Is);
        stringPtr = StringCopy(stringPtr, sGiddyAdjectives[adjective]);
        StringCopy(stringPtr, gOtherText_DontYouAgree);
    }
    else
    {
        StringCopy(gStringVar4, sGiddyQuestions[giddy->questionList[giddy->questionNum++]]);
    }

    if (!(Random() % 10))
        giddy->taleCounter = 10;
    else
        giddy->taleCounter++;

    gSpecialVar_Result = TRUE;
}

void sub_81204DC(void)
{
    struct MauvilleManGiddy *giddy = &gSaveBlock1Ptr->oldMan.giddy;
    u16 arr[][2] = {
        {EC_GROUP_POKEMON,   0},
        {EC_GROUP_LIFESTYLE, 0},
        {EC_GROUP_HOBBIES,   0},
        {EC_GROUP_MOVE_1,    0},
        {EC_GROUP_MOVE_2,    0},
        {EC_GROUP_POKEMON_2, 0}
    };
    u16 i;
    u16 r10;
    u16 r7;
    u16 r1;

    for (i = 0; i < 8; i++)
        giddy->questionList[i] = i;

    for (i = 0; i < 8; i++)
    {
        r1 = Random() % (i + 1);
        r7 = giddy->questionList[i];
        giddy->questionList[i] = giddy->questionList[r1];
        giddy->questionList[r1] = r7;
    }

    r10 = 0;
    for (i = 0; i < 6; i++)
    {
        arr[i][1] = EasyChat_GetNumWordsInGroup(arr[i][0]);
        r10 += arr[i][1];
    }

    giddy->questionNum = 0;
    r7 = 0;
    for (i = 0; i < 10; i++)
    {
        r1 = Random() % 10;
        if (r1 < 3 && r7 < 8)
        {
            giddy->randomWords[i] = 0xFFFF;
            r7++;
        }
        else
        {
            s16 r2 = Random() % r10;
            for (r1 = 0; i < 6; r1++)
                if ((r2 -= arr[r1][1]) <= 0)
                    break;
            if (r1 == 6)
                r1 = 0;
            giddy->randomWords[i] = sub_811EE90(arr[r1][0]);
        }
    }
}
void sub_8120628(void)
{
    struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;

    bard->hasChangedSong = FALSE;
}

void sub_8120640(void)
{
    struct MauvilleManHipster *hipster = &gSaveBlock1Ptr->oldMan.hipster;

    hipster->alreadySpoken = FALSE;
}

void sub_8120658(void)
{
    sub_8133A60();
}

void sub_8120664(void)
{
    sub_8120E50();
}

void sub_8120670(void) // ResetMauvilleOldManFlag
{
    switch (sub_81201C8())
    {
        case MAUVILLE_MAN_BARD:
            sub_8120628();
            break;
        case MAUVILLE_MAN_HIPSTER:
            sub_8120640();
            break;
        case MAUVILLE_MAN_STORYTELLER:
            sub_8120664();
            break;
        case MAUVILLE_MAN_TRADER:
            sub_8120658();
            break;
        case MAUVILLE_MAN_GIDDY:
            break;
    }
    sub_8120B5C();
}


#define tState data[0]
#define tCharIndex data[3]
#define tCurrWord data[4]
#define tUseTemporaryLyrics data[5]

#define MACRO1(a) (((a) & 3) + (((a) / 8) & 1))
#define MACRO2(a) (((a) % 4) + (((a) / 8) & 1))

void sub_81206C0(bool8 useTemporaryLyrics)
{
    u8 taskId = CreateTask(sub_8120944, 80);

    gTasks[taskId].tUseTemporaryLyrics = useTemporaryLyrics;
}

void sub_81206F0(void)
{
    gUnknown_03002F84 = FALSE;
}

void sub_81206FC(struct TextSubPrinter * printer, u16 a1)
{
    gUnknown_03002F84 = TRUE;
}

void sub_8120708(const u8 * src)
{
    NewMenuHelpers_DrawDialogueFrame(0, 0);
    PrintTextOnWindow(0, 1, src, 0, 1, 1, sub_81206FC);
    gUnknown_03002F84 = TRUE;
    CopyWindowToVram(0, 3);
}

void sub_8120748(struct Task *task, struct BardSong *song) // BardSing
{
    switch (task->tState)
    {
        case 0:  // Initialize song
        {
            struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
            u16 *lyrics;
            s32 i;

            // Copy lyrics
            if (gSpecialVar_0x8004 == 0)
                lyrics = bard->songLyrics;
            else
                lyrics = bard->temporaryLyrics;
            for (i = 0; i < 6; i++)
                song->lyrics[i] = lyrics[i];
            song->currWord = 0;
        }
            break;
        case 1:  // Wait for BGM to end
            break;
        case 2:  // Initialize word
        {
            u16 word = song->lyrics[song->currWord];
            song->sound = GetWordSounds(word);
            GetWordPhonemes(song, MACRO1(word));
            song->currWord++;
            if (song->sound->var00 != 0xFF)
                song->state = 0;
            else
            {
                song->state = 3;
                song->phonemeTimer = 2;
            }
            break;
        }
        case 3:
        case 4:
        {
            const struct BardSound *sound = &song->sound[song->currPhoneme];

            switch (song->state)
            {
                case 0:
                    song->phonemeTimer = song->phonemes[song->currPhoneme].length;
                    if (sound->var00 <= 50)
                    {
                        u8 num = sound->var00 / 3;
                        m4aSongNumStart(PH_TRAP_HELD + 3 * num);
                    }
                    song->state = 2;
                    song->phonemeTimer--;
                    break;
                case 2:
                    song->state = 1;
                    if (sound->var00 <= 50)
                    {
                        song->volume = 0x100 + sound->volume * 16;
                        m4aMPlayVolumeControl(&gMPlayInfo_SE2, 0xFFFF, song->volume);
                        song->pitch = 0x200 + song->phonemes[song->currPhoneme].pitch;
                        m4aMPlayPitchControl(&gMPlayInfo_SE2, 0xFFFF, song->pitch);
                    }
                    break;
                case 1:
                    if (song->voiceInflection > 10)
                        song->volume -= 2;
                    if (song->voiceInflection & 1)
                        song->pitch += 64;
                    else
                        song->pitch -= 64;
                    m4aMPlayVolumeControl(&gMPlayInfo_SE2, 0xFFFF, song->volume);
                    m4aMPlayPitchControl(&gMPlayInfo_SE2, 0xFFFF, song->pitch);
                    song->voiceInflection++;
                    song->phonemeTimer--;
                    if (song->phonemeTimer == 0)
                    {
                        song->currPhoneme++;
                        if (song->currPhoneme != 6 && song->sound[song->currPhoneme].var00 != 0xFF)
                            song->state = 0;
                        else
                        {
                            song->state = 3;
                            song->phonemeTimer = 2;
                        }
                    }
                    break;
                case 3:
                    song->phonemeTimer--;
                    if (song->phonemeTimer == 0)
                    {
                        m4aMPlayStop(&gMPlayInfo_SE2);
                        song->state = 4;
                    }
                    break;
            }
        }
            break;
        case 5:
            break;
    }
}

void sub_8120944(u8 taskId)
{
    struct Task *task = &gTasks[taskId];  // r5

    sub_8120748(task, &gUnknown_03006130);
    switch (task->tState)
    {
        case 0:  // Initialize song
            sub_8120280();
            sub_8120708(gStringVar4);
            task->data[1] = 0;
            task->data[2] = 0;
            task->tCharIndex = 0;
            task->tCurrWord = 0;
            FadeOutBGMTemporarily(4);
            task->tState = 1;
            break;
        case 1:  // Wait for BGM to end
            if (IsBGMPausedOrStopped())
                task->tState = 2;
            break;
        case 2:  // Initialize word
        {
            struct MauvilleManBard *bard = &gSaveBlock1Ptr->oldMan.bard;
            u8 *str = gStringVar4 + task->tCharIndex;
            u16 wordLen = 0;
            // Can't get it to match without hacking
            u32 temp;
            register s16 zero asm("r1");

            while (*str != CHAR_SPACE
                   && *str != CHAR_NEWLINE
                   && *str != EXT_CTRL_CODE_BEGIN
                   && *str != EOS)
            {
                str++;
                wordLen++;
            }
            if (!task->tUseTemporaryLyrics)
                gUnknown_0203A128 = MACRO2(bard->songLyrics[task->tCurrWord]);
            else
                gUnknown_0203A128 = MACRO2(bard->temporaryLyrics[task->tCurrWord]);
            temp = gUnknown_03006130.length / wordLen;
            zero = 0;
            gUnknown_03006130.length = temp;
            if (gUnknown_03006130.length <= 0)
                gUnknown_03006130.length = 1;
            task->tCurrWord++;
            if (task->data[2] == 0)
                task->tState = 3;
            else
                task->tState = 5;
            task->data[1] = zero;
        }
            break;
        case 5:
            if (task->data[2] == 0)
                task->tState = 3;
            else
                task->data[2]--;
            break;
        case 3:
            if (gStringVar4[task->tCharIndex] == EOS)
            {
                FadeInBGM(6);
                m4aMPlayFadeOutTemporarily(&gMPlayInfo_SE2, 2);
                EnableBothScriptContexts();
                DestroyTask(taskId);
            }
            else if (gStringVar4[task->tCharIndex] == CHAR_SPACE)
            {

                sub_81206F0();
                task->tCharIndex++;
                task->tState = 2;
                task->data[2] = 0;
            }
            else if (gStringVar4[task->tCharIndex] == CHAR_NEWLINE)
            {
                task->tCharIndex++;
                task->tState = 2;
                task->data[2] = 0;
            }
            else if (gStringVar4[task->tCharIndex] == EXT_CTRL_CODE_BEGIN)
            {
                task->tCharIndex += 2;  // skip over control codes
                task->tState = 2;
                task->data[2] = 8;
            }
            else if (gStringVar4[task->tCharIndex] == CHAR_SONG_WORD_SEPARATOR)
            {
                gStringVar4[task->tCharIndex] = CHAR_SPACE;  // restore it back to a space
                sub_81206F0();
                task->tCharIndex++;
                task->data[2] = 0;
            }
            else
            {
                switch (task->data[1])
                {
                    case 0:
                        sub_81206F0();
                        task->data[1]++;
                        break;
                    case 1:
                        task->data[1]++;
                        break;
                    case 2:
                        task->tCharIndex++;
                        task->data[1] = 0;
                        task->data[2] = gUnknown_03006130.length;
                        task->tState = 4;
                        break;
                }
            }
            break;
        case 4:
            task->data[2]--;
            if (task->data[2] == 0)
                task->tState = 3;
            break;
    }
    sub_8197224();
}

void sub_8120B5C(void)
{
    VarSet(VAR_0x4010, MAP_OBJ_GFX_BARD);
}

void sub_8120B70(union OldMan * oldMan)
{
    s32 i;
    u8 sp00[8];

    switch (oldMan->common.id)
    {
        case MAUVILLE_MAN_TRADER:
        {
            struct MauvilleOldManTrader * trader = &oldMan->trader;
            for (i = 0; i < 4; i++)
            {
                if (trader->unk32[i] == LANGUAGE_JAPANESE)
                {
                    ConvertInternationalString(trader->unk5[i], LANGUAGE_JAPANESE);
                }
            }
        }
            break;
        case MAUVILLE_MAN_STORYTELLER:
        {
            struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;
            for (i = 0; i < 4; i++)
            {
                if (storyteller->gameStatIDs[i] != 0)
                {
                    memcpy(sp00, storyteller->trainerNames[i], 7);
                    sp00[7] = EOS;
                    if (IsStringJapanese(sp00))
                    {
                        memset(sp00, CHAR_SPACE, 8);
                        StringCopy(sp00, gText_Friend);
                        memcpy(storyteller->trainerNames[i], sp00, 7);
                        storyteller->unk34[i] = GAME_LANGUAGE;
                    }
                }
            }
        }
            break;
    }
}

void sub_8120C0C(union OldMan * oldMan, u32 r8, u32 r7, u32 r3)
{
    s32 i;

    switch (oldMan->common.id)
    {
        case MAUVILLE_MAN_TRADER:
        {
            struct MauvilleOldManTrader * trader = &oldMan->trader;

            for (i = 0; i < 4; i++)
            {
                if (IsStringJapanese(trader->unk5[i]))
                {
                    trader->unk32[i] = r8;
                }
                else
                {
                    trader->unk32[i] = r7;
                }
            }
        }
            break;
        case MAUVILLE_MAN_STORYTELLER:
        {
            struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;

            for (i = 0; i < 4; i++)
            {
                if (IsStringJapanese(storyteller->trainerNames[i]))
                {
                    storyteller->unk34[i] = r8;
                }
                else
                {
                    storyteller->unk34[i] = r7;
                }
            }
        }
            break;
        case MAUVILLE_MAN_BARD:
        {
            struct MauvilleManBard * bard = &oldMan->bard;

            if (r3 == LANGUAGE_JAPANESE)
                bard->language = r8;
            else
                bard->language = r7;
        }
            break;
        case MAUVILLE_MAN_HIPSTER:
        {
            struct MauvilleManHipster * hipster = &oldMan->hipster;

            if (r3 == LANGUAGE_JAPANESE)
                hipster->language = r8;
            else
                hipster->language = r7;
        }
            break;
        case MAUVILLE_MAN_GIDDY:
        {
            struct MauvilleManGiddy * giddy = &oldMan->giddy;

            if (r3 == LANGUAGE_JAPANESE)
                giddy->language = r8;
            else
                giddy->language = r7;
        }
            break;
    }
}

void sub_8120CD0(union OldMan * oldMan, u32 unused, u32 a2)
{
    u8 sp00[8];
    s32 i;
    if (oldMan->common.id == MAUVILLE_MAN_STORYTELLER && a2 == LANGUAGE_JAPANESE)
    {
        struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;

        for (i = 0; i < 4; i++)
        {
            if (storyteller->gameStatIDs[i] != 0)
            {
                memcpy(sp00, storyteller->trainerNames[i], 7);
                sp00[7] = EOS;
                if (IsStringJapanese(sp00))
                    storyteller->unk34[i] = LANGUAGE_JAPANESE;
                else
                    storyteller->unk34[i] = GAME_LANGUAGE;
            }
        }
    }
}

void sub_8120D34(union OldMan * oldMan, u32 r1, u32 r6)
{
    u32 r2 = (r1 == LANGUAGE_JAPANESE || r1 == LANGUAGE_ENGLISH) ? 1 : 0;
    switch (oldMan->common.id)
    {
        case MAUVILLE_MAN_TRADER:
        {
            struct MauvilleOldManTrader * trader = &oldMan->trader;
            s32 i;

            if (r2)
            {
                for (i = 0; i < 4; i++)
                {
                    u8 * str = trader->unk5[i];
                    if (str[0] == EXT_CTRL_CODE_BEGIN && str[1] == EXT_CTRL_CODE_JPN)
                    {
                        StripExtCtrlCodes(str);
                        trader->unk32[i] = LANGUAGE_JAPANESE;
                    }
                    else
                        trader->unk32[i] = r6;
                }
            }
            else
            {
                for (i = 0; i < 4; i++)
                {
                    if (trader->unk32[i] == LANGUAGE_JAPANESE)
                    {
                        StripExtCtrlCodes(trader->unk5[i]);
                    }
                }
            }
        }
            break;
        case MAUVILLE_MAN_STORYTELLER:
        {

            struct MauvilleManStoryteller * storyteller = &oldMan->storyteller;
            s32 i;

            if (r2)
            {
                for (i = 0; i < 4; i++)
                {
                    if (storyteller->gameStatIDs[i] != 0)
                        storyteller->unk34[i] = r6;
                }
            }
        }
            break;
        case MAUVILLE_MAN_BARD:
        {
            struct MauvilleManBard * bard = &oldMan->bard;

            if (r2)
            {
                bard->language = r6;
            }
        }
            break;
        case MAUVILLE_MAN_HIPSTER:
        {
            struct MauvilleManHipster * hipster = &oldMan->hipster;

            if (r2)
            {
                hipster->language = r6;
            }
        }
            break;
        case MAUVILLE_MAN_GIDDY:
        {
            struct MauvilleManGiddy * giddy = &oldMan->giddy;

            if (r2)
            {
                giddy->language = r6;
            }
        }
            break;
    }
}

struct Story
{
    u8 stat;
    u8 minVal;
    const u8 *title;
    const u8 *action;
    const u8 *fullText;
};

const struct Story gUnknown_0859F048[] = {
    {GAME_STAT_50, 1, MauvilleCity_PokemonCenter_1F_Text_28E930, MauvilleCity_PokemonCenter_1F_Text_28E947, MauvilleCity_PokemonCenter_1F_Text_28E956},
    {GAME_STAT_STARTED_TRENDS, 1, MauvilleCity_PokemonCenter_1F_Text_28E9D7, MauvilleCity_PokemonCenter_1F_Text_28E9EF, MauvilleCity_PokemonCenter_1F_Text_28E9FE},
    {GAME_STAT_PLANTED_BERRIES, 1, MauvilleCity_PokemonCenter_1F_Text_28EA7D, MauvilleCity_PokemonCenter_1F_Text_28EA98, MauvilleCity_PokemonCenter_1F_Text_28EAA8},
    {GAME_STAT_TRADED_BIKES, 1, MauvilleCity_PokemonCenter_1F_Text_28EB19, MauvilleCity_PokemonCenter_1F_Text_28EB31, MauvilleCity_PokemonCenter_1F_Text_28EB3E},
    {GAME_STAT_GOT_INTERVIEWED, 1, MauvilleCity_PokemonCenter_1F_Text_28EBB5, MauvilleCity_PokemonCenter_1F_Text_28EBCD, MauvilleCity_PokemonCenter_1F_Text_28EBDD},
    {GAME_STAT_TRAINER_BATTLES, 1, MauvilleCity_PokemonCenter_1F_Text_28EC60, MauvilleCity_PokemonCenter_1F_Text_28EC79, MauvilleCity_PokemonCenter_1F_Text_28EC81},
    {GAME_STAT_POKEMON_CAPTURES, 1, MauvilleCity_PokemonCenter_1F_Text_28ED04, MauvilleCity_PokemonCenter_1F_Text_28ED21, MauvilleCity_PokemonCenter_1F_Text_28ED30},
    {GAME_STAT_FISHING_CAPTURES, 1, MauvilleCity_PokemonCenter_1F_Text_28EDA1, MauvilleCity_PokemonCenter_1F_Text_28EDB5, MauvilleCity_PokemonCenter_1F_Text_28EDCF},
    {GAME_STAT_HATCHED_EGGS, 1, MauvilleCity_PokemonCenter_1F_Text_28EE45, MauvilleCity_PokemonCenter_1F_Text_28EE5D, MauvilleCity_PokemonCenter_1F_Text_28EE6A},
    {GAME_STAT_EVOLVED_POKEMON, 1, MauvilleCity_PokemonCenter_1F_Text_28EEDD, MauvilleCity_PokemonCenter_1F_Text_28EEF1, MauvilleCity_PokemonCenter_1F_Text_28EF01},
    {GAME_STAT_USED_POKECENTER, 1, MauvilleCity_PokemonCenter_1F_Text_28EF73, MauvilleCity_PokemonCenter_1F_Text_28EF95, MauvilleCity_PokemonCenter_1F_Text_28EFAA},
    {GAME_STAT_RESTED_AT_HOME, 1, MauvilleCity_PokemonCenter_1F_Text_28F045, MauvilleCity_PokemonCenter_1F_Text_28F05A, MauvilleCity_PokemonCenter_1F_Text_28F071},
    {GAME_STAT_ENTERED_SAFARI_ZONE, 1, MauvilleCity_PokemonCenter_1F_Text_28F0F3, MauvilleCity_PokemonCenter_1F_Text_28F10D, MauvilleCity_PokemonCenter_1F_Text_28F125},
    {GAME_STAT_USED_CUT, 1, MauvilleCity_PokemonCenter_1F_Text_28F1BE, MauvilleCity_PokemonCenter_1F_Text_28F1D5, MauvilleCity_PokemonCenter_1F_Text_28F1DE},
    {GAME_STAT_USED_ROCK_SMASH, 1, MauvilleCity_PokemonCenter_1F_Text_28F24F, MauvilleCity_PokemonCenter_1F_Text_28F269, MauvilleCity_PokemonCenter_1F_Text_28F277},
    {GAME_STAT_MOVED_SECRET_BASE, 1, MauvilleCity_PokemonCenter_1F_Text_28F2FC, MauvilleCity_PokemonCenter_1F_Text_28F314, MauvilleCity_PokemonCenter_1F_Text_28F32A},
    {GAME_STAT_USED_SPLASH, 1, MauvilleCity_PokemonCenter_1F_Text_28F3AD, MauvilleCity_PokemonCenter_1F_Text_28F3C6, MauvilleCity_PokemonCenter_1F_Text_28F3D2},
    {GAME_STAT_USED_STRUGGLE, 1, MauvilleCity_PokemonCenter_1F_Text_28F44B, MauvilleCity_PokemonCenter_1F_Text_28F461, MauvilleCity_PokemonCenter_1F_Text_28F47C},
    {GAME_STAT_SLOT_JACKPOTS, 1, MauvilleCity_PokemonCenter_1F_Text_28F50C, MauvilleCity_PokemonCenter_1F_Text_28F51B, MauvilleCity_PokemonCenter_1F_Text_28F538},
    {GAME_STAT_CONSECUTIVE_ROULETTE_WINS, 2, MauvilleCity_PokemonCenter_1F_Text_28F5BE, MauvilleCity_PokemonCenter_1F_Text_28F5D1, MauvilleCity_PokemonCenter_1F_Text_28F5F2},
    {GAME_STAT_ENTERED_BATTLE_TOWER, 1, MauvilleCity_PokemonCenter_1F_Text_28F678, MauvilleCity_PokemonCenter_1F_Text_28F694, MauvilleCity_PokemonCenter_1F_Text_28F6B4},
    {GAME_STAT_POKEBLOCKS, 1, MauvilleCity_PokemonCenter_1F_Text_28F751, MauvilleCity_PokemonCenter_1F_Text_28F76A, MauvilleCity_PokemonCenter_1F_Text_28F776},
    {GAME_STAT_ENTERED_CONTEST, 1, MauvilleCity_PokemonCenter_1F_Text_28F7F6, MauvilleCity_PokemonCenter_1F_Text_28F811, MauvilleCity_PokemonCenter_1F_Text_28F822},
    {GAME_STAT_WON_CONTEST, 1, MauvilleCity_PokemonCenter_1F_Text_28F89C, MauvilleCity_PokemonCenter_1F_Text_28F8AF, MauvilleCity_PokemonCenter_1F_Text_28F8BC},
    {GAME_STAT_SHOPPED, 1, MauvilleCity_PokemonCenter_1F_Text_28F92F, MauvilleCity_PokemonCenter_1F_Text_28F941, MauvilleCity_PokemonCenter_1F_Text_28F949},
    {GAME_STAT_USED_ITEMFINDER, 1, MauvilleCity_PokemonCenter_1F_Text_28F9D1, MauvilleCity_PokemonCenter_1F_Text_28F9EA, MauvilleCity_PokemonCenter_1F_Text_28F9FD},
    {GAME_STAT_GOT_RAINED_ON, 1, MauvilleCity_PokemonCenter_1F_Text_28FA81, MauvilleCity_PokemonCenter_1F_Text_28FA99, MauvilleCity_PokemonCenter_1F_Text_28FAA7},
    {GAME_STAT_CHECKED_POKEDEX, 1, MauvilleCity_PokemonCenter_1F_Text_28FB1D, MauvilleCity_PokemonCenter_1F_Text_28FB35, MauvilleCity_PokemonCenter_1F_Text_28FB47},
    {GAME_STAT_RECEIVED_RIBBONS, 1, MauvilleCity_PokemonCenter_1F_Text_28FBC4, MauvilleCity_PokemonCenter_1F_Text_28FBD9, MauvilleCity_PokemonCenter_1F_Text_28FBEA},
    {GAME_STAT_JUMPED_DOWN_LEDGES, 1, MauvilleCity_PokemonCenter_1F_Text_28FC6B, MauvilleCity_PokemonCenter_1F_Text_28FC85, MauvilleCity_PokemonCenter_1F_Text_28FC98},
    {GAME_STAT_WATCHED_TV, 1, MauvilleCity_PokemonCenter_1F_Text_28FD1D, MauvilleCity_PokemonCenter_1F_Text_28FD35, MauvilleCity_PokemonCenter_1F_Text_28FD40},
    {GAME_STAT_CHECKED_CLOCK, 1, MauvilleCity_PokemonCenter_1F_Text_28FDA2, MauvilleCity_PokemonCenter_1F_Text_28FDBD, MauvilleCity_PokemonCenter_1F_Text_28FDCE},
    {GAME_STAT_WON_POKEMON_LOTTERY, 1, MauvilleCity_PokemonCenter_1F_Text_28FE57, MauvilleCity_PokemonCenter_1F_Text_28FE72, MauvilleCity_PokemonCenter_1F_Text_28FE88},
    {GAME_STAT_USED_DAYCARE, 1, MauvilleCity_PokemonCenter_1F_Text_28FF0C, MauvilleCity_PokemonCenter_1F_Text_28FF27, MauvilleCity_PokemonCenter_1F_Text_28FF44},
    {GAME_STAT_RODE_CABLE_CAR, 1, MauvilleCity_PokemonCenter_1F_Text_28FFDD, MauvilleCity_PokemonCenter_1F_Text_28FFFA, MauvilleCity_PokemonCenter_1F_Text_29000D},
    {GAME_STAT_ENTERED_HOT_SPRINGS, 1, MauvilleCity_PokemonCenter_1F_Text_290097, MauvilleCity_PokemonCenter_1F_Text_2900B5, MauvilleCity_PokemonCenter_1F_Text_2900CB}
};

void sub_8120E08(void) // StorytellerSetup
{
    s32 i;
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;

    gUnknown_0203A12C->id = MAUVILLE_MAN_STORYTELLER;
    gUnknown_0203A12C->alreadyRecorded = FALSE;
    for (i = 0; i < 4; i++)
    {
        gUnknown_0203A12C->gameStatIDs[i] = 0;
        gUnknown_0203A12C->trainerNames[0][i] = EOS;  // Maybe they meant storyteller->trainerNames[i][0] instead?
    }
}

void sub_8120E50(void)
{
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;

    gUnknown_0203A12C->id = MAUVILLE_MAN_STORYTELLER;
    gUnknown_0203A12C->alreadyRecorded = FALSE;
}

u32 sub_8120E74(u8 stat) // StorytellerGetGameStat
{
    if (stat == 50)
        stat = 0;
    return GetGameStat(stat);
}

const struct Story *sub_8120E88(u32 stat) // GetStoryByStat
{
    s32 i;

    for (i = 0; i < 36; i++)
    {
        if (gUnknown_0859F048[i].stat == stat)
            return &gUnknown_0859F048[i];
    }
    return &gUnknown_0859F048[35];
}

const u8 *sub_8120EB4(u32 stat) // GetStoryTitleByStat
{
    return sub_8120E88(stat)->title;
}

const u8 *sub_8120EC0(u32 stat) // GetStoryTextByStat
{
    return sub_8120E88(stat)->fullText;
}

const u8 *sub_8120ECC(u32 stat) // GetStoryActionByStat
{
    return sub_8120E88(stat)->action;
}

u8 sub_8120ED8(void) // GetFreeStorySlot
{
    u8 i;

    for (i = 0; i < 4; i++)
    {
        if (gUnknown_0203A12C->gameStatIDs[i] == 0)
            break;
    }
    return i;
}

u32 sub_8120F08(u32 trainer) // StorytellerGetRecordedTrainerStat
{
    u8 *ptr = gUnknown_0203A12C->statValues[trainer];

    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

void sub_8120F2C(u32 trainer, u32 val) // StorytellerSetRecordedTrainerStat
{
    u8 *ptr = gUnknown_0203A12C->statValues[trainer];

    ptr[0] = val;
    ptr[1] = val >> 8;
    ptr[2] = val >> 16;
    ptr[3] = val >> 24;
}

bool32 sub_8120F4C(u32 trainer) // HasTrainerStatIncreased
{
    if (sub_8120E74(gUnknown_0203A12C->gameStatIDs[trainer]) > sub_8120F08(trainer))
        return TRUE;
    else
        return FALSE;
}

void sub_8120F7C(u32 player, void *dst) // GetStoryByStattellerPlayerName
{
    u8 *name = gUnknown_0203A12C->trainerNames[player];

    memset(dst, EOS, 8);
    memcpy(dst, name, 7);
}

void sub_8120FAC(u32 player, const u8 * src) // StorytellerSetPlayerName
{
    u8 * name = gUnknown_0203A12C->trainerNames[player];
    memset(name, EOS, 7);
    memcpy(name, src, 7);
}


void sub_8120FDC(u32 player, u32 stat) // StorytellerRecordNewStat
{
    gUnknown_0203A12C->gameStatIDs[player] = stat;
    sub_8120FAC(player, gSaveBlock2Ptr->playerName);
    sub_8120F2C(player, sub_8120E74(stat));
    ConvertIntToDecimalStringN(gStringVar1, sub_8120E74(stat), STR_CONV_MODE_LEFT_ALIGN, 10);
    StringCopy(gStringVar2, sub_8120ECC(stat));
    gUnknown_0203A12C->unk34[player] = gGameLanguage;
}

void sub_8121064(u8 * arr, s32 count) // ScrambleStatList
{
    s32 i;

    for (i = 0; i < count; i++)
        arr[i] = i;
    for (i = 0; i < count; i++)
    {
        u32 a = Random() % count;
        u32 b = Random() % count;
        u8 temp = arr[a];
        arr[a] = arr[b];
        arr[b] = temp;
    }
}

struct UnknownStruct_0859F288
{
    u32 length;
    u32 unused2;
};

const struct UnknownStruct_0859F288 gUnknown_0859F288 = {
    36,
    8
};

bool8 sub_81210B8(void) // StorytellerInitializeRandomStat
{
    u8 arr[gUnknown_0859F288.length];
    s32 i;
    s32 j;

    sub_8121064(arr, 36);
    for (i = 0; i < 36; i++)
    {
        u8 stat = gUnknown_0859F048[arr[i]].stat;
        u8 minVal = gUnknown_0859F048[arr[i]].minVal;

        for (j = 0; j < 4; j++)
        {
            if (gUnknown_0203A12C->gameStatIDs[j] == stat)
                break;
        }
        if (j == 4 && sub_8120E74(stat) >= minVal)
        {
            gUnknown_0203A12C->alreadyRecorded = TRUE;
            if (sub_8120ED8() == 4)
                sub_8120FDC(gUnknown_03001178, stat);
            else
                sub_8120FDC(sub_8120ED8(), stat);
            return TRUE;
        }
    }
    return FALSE;
}

void sub_8121178(u32 player) // StorytellerDisplayStory
{
    u8 stat = gUnknown_0203A12C->gameStatIDs[player];

    ConvertIntToDecimalStringN(gStringVar1, sub_8120F08(player), 0, 10);
    StringCopy(gStringVar2, sub_8120ECC(stat));
    sub_8120F7C(player, gStringVar3);
    ConvertInternationalString(gStringVar3, gUnknown_0203A12C->unk34[player]);
    ShowFieldMessage(sub_8120EC0(stat));
}

void sub_81211EC(void) // PrintStoryList
{
    s32 i;
    s32 width = GetStringWidth(1, gText_Exit, 0);
    u8 tileWidth;
    for (i = 0; i < 4; i++)
    {
        s32 curWidth;
        u16 gameStatID = gUnknown_0203A12C->gameStatIDs[i];

        if (gameStatID == 0)
            break;
        curWidth = GetStringWidth(1, sub_8120EB4(gameStatID), 0);
        if (curWidth > width)
            width = curWidth;
    }
    gUnknown_0203A130 = CreateWindowFromRect(0, 0, convert_pixel_width_to_tile_width(width), sub_8120ED8() * 2 + 2);
    SetStandardWindowBorderStyle(gUnknown_0203A130, 0);
    for (i = 0; i < 4; i++)
    {
        u16 gameStatID = gUnknown_0203A12C->gameStatIDs[i];
        if (gameStatID == 0)
            break;
        PrintTextOnWindow(gUnknown_0203A130, 1, sub_8120EB4(gameStatID), 8, 16 * i + 1, 0xFF, NULL);
    }
    PrintTextOnWindow(gUnknown_0203A130, 1, gText_Exit, 8, 16 * i + 1, 0xFF, NULL);
    InitMenuInUpperLeftCornerPlaySoundWhenAPressed(gUnknown_0203A130, sub_8120ED8() + 1, 0);
    CopyWindowToVram(gUnknown_0203A130, 3);
}

void sub_81212FC(u8 taskId) // Task_StoryListMenu
{
    struct Task *task = &gTasks[taskId];
    s32 selection;

    switch (task->data[0])
    {
        case 0:
            sub_81211EC();
            task->data[0]++;
            break;
        case 1:
            selection = ProcessMenuInput();
            if (selection == -2)
                break;
            if (selection == -1 || selection == sub_8120ED8())
            {
                gSpecialVar_Result = 0;
            }
            else
            {
                gSpecialVar_Result = 1;
                gUnknown_03001178 = selection;
            }
            sub_80E2A78(gUnknown_0203A130);
            DestroyTask(taskId);
            EnableBothScriptContexts();
            break;
    }
}

// Sets gSpecialVar_Result to TRUE if player selected a story
void sub_8121388(void) // ScrSpecial_StorytellerStoryListMenu
{
    CreateTask(sub_81212FC, 80);
}

void sub_812139C(void) // ScrSpecial_StorytellerDisplayStory
{
    sub_8121178(gUnknown_03001178);
}

u8 sub_81213B0(void) // ScrSpecial_StorytellerDisplayStory
{
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;
    return sub_8120ED8();
}

// Returns TRUE if stat has increased
bool8 sub_81213D8(void) // ScrSpecial_StorytellerUpdateStat
{
    u8 r4;
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;
    r4 = gUnknown_0203A12C->gameStatIDs[gUnknown_03001178];

    if (sub_8120F4C(gUnknown_03001178) == TRUE)
    {
        sub_8120FDC(gUnknown_03001178, r4);
        return TRUE;
    }
    return FALSE;
}


bool8 sub_8121424(void) // ScrSpecial_HasStorytellerAlreadyRecorded
{
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;

    if (gUnknown_0203A12C->alreadyRecorded == FALSE)
        return FALSE;
    else
        return TRUE;
}

bool8 sub_8121450(void) // ScrSpecial_StorytellerInitializeRandomStat
{
    gUnknown_0203A12C = &gSaveBlock1Ptr->oldMan.storyteller;
    return sub_81210B8();
}

