#include "funcs.h"

#include <math.h>

#include "util.h"
#include "window.h"
#include "wallpaper.h"
#include "strings.h"

#define PLAYERNAME          "_PlayerName_xxxxxxxxxxxxxxxxxxxx"
#define PLAYERNAME_GUESS    "_PlayerName_guess_xxxxxxxxxxxxxx"
#define PLAYERNAME_SIZE     (32)

#define PLAYERNAME_ENTRY        "\\N[2]"
#define PLAYERNAME_ENTRY_SIZE   (sizeof(PLAYERNAME_ENTRY)-1)

#define NUM_SWITCHES        200
#define NUM_VARIABLES       100

//SPECIAL FUNCTIONS
//init()
//called at the start of the game, used to initialize memory addresses
//and such
void
func_init()
{
    //Find the variable array
    {
        int magicNumber[] = {8008135, 3133710, 1010101, 1234567, 9988777, 4581234, 1548483};
        variables = util_findMemory(NULL, magicNumber, sizeof(magicNumber), 4*NUM_VARIABLES-sizeof(magicNumber));
        //Clear out the magic numbers so we don't find this block of memory next time we
        //start a game
        memset(variables, 0, sizeof(magicNumber));
    }

    //Find the switch array
    {
        char magicNumber[] = {1,1,0,1,0,1,0,0,1,1,0,1,0,1,0,0,1,0,1,0};
        switches = util_findMemory(NULL, magicNumber, sizeof(magicNumber), NUM_SWITCHES-sizeof(magicNumber));
        //Clear out the magic numbers so we don't find this block of memory next time we
        //start a game
        memset(switches, 0, sizeof(magicNumber));
    }

    //Get the user's name to the best of our ability
    if(!GetUserNameExA(NameDisplay, username, &usernameSize))
    {
        usernameSize = sizeof(username);
        GetUserName(username, &usernameSize);
    }
    if(--usernameSize > PLAYERNAME_SIZE)
        usernameSize = PLAYERNAME_SIZE;

    //Replace the generic player name with the first 12 letters of the guess
    int truncatedSize = (usernameSize > 12) ? 12 : usernameSize;
    char* name = NULL;
    while((name = util_findMemory(name, "TPlayerNameT", 12, 0)))
    {
        memcpy(name, username, truncatedSize);
        memset(name + truncatedSize, ' ', 12 - truncatedSize);
    }

    //Replace the quit message if the ending is different
    if(ending > ENDING_DEJAVU)
        util_updateQuitMessage();

    //Initialize some runtime game variables
    variables[VAR_ENDING] = ending;

    //Generate George face
    WCHAR szComputerName[256];
    DWORD sizeComputerName = sizeof(szComputerName)/2;
    GetComputerNameW(szComputerName, &sizeComputerName);
    variables[VAR_GEORGE] = 0;
    int i = 0;
    for(; szComputerName[i]; i++)
        variables[VAR_GEORGE] += szComputerName[i];
    variables[VAR_GEORGE] %= 6;
    variables[VAR_GEORGE] += 1;

    //LPWSTR msg = _aswprintf(L"%ls %d", szComputerName, variables[VAR_GEORGE]);
    //MESSAGEW(msg);
    //free(msg);

    gameStarted = TRUE;
    isInGame = TRUE;
}

//Main window is closed
void
func_Close()
{
    if(!closeEnabled)
        return;

    if(isInGame && ending == ENDING_DEAD)
    {
        if(oneshot)
        {
            //Warn the player
            int choice = MessageBoxW(window, STR_NIKO_WILL_DIE, L"Warning", MB_ICONWARNING | MB_YESNO);
            if(choice == IDYES)
            {
                util_saveEnding();
                DestroyWindow(window);
            }
        }
        else
        {
            //Guilt them
            util_messagebox(STR_YOU_KILLED_NIKO, L"", MESSAGE_ERROR);
            util_saveEnding();
            DestroyWindow(window);
        }
    }
    else
    {
        util_saveEnding();
        DestroyWindow(window);
    }
}

//Main window is destroyed
void
func_Destroy()
{
    isWindowDestroyed = TRUE;
    if(!wndKitty_isActive)
        PostQuitMessage(0);
}

//OTHER FUNCTIONS
void
func_GuessName()
{
    //Replace the guess username
    char* name = NULL;
    while((name = util_findMemory(name, PLAYERNAME_GUESS, PLAYERNAME_SIZE, 1)))
    {
        //Replace the name with the username
        memcpy(name, username, usernameSize);
        //Move the punctuation backwards
        name[usernameSize] = name[PLAYERNAME_SIZE];
        //Fill the rest of the dummy name with spaces
        memset(name + usernameSize + 1, ' ', PLAYERNAME_SIZE - usernameSize);

#if 0
        int offset = PLAYERNAME_SIZE - usernameSize;

        //Move the rest of the message backwards till we hit the @
        for(name += usernameSize; name[offset] != '@'; name++)
            name[0] = name[offset];

        //Fill the end with spaces
        memset(name, ' ', offset + 1);
#endif
    }
}

void
func_SetName()
{
    //Replace the player name with the username we have stored
    char* name = NULL;
    while((name = util_findMemory(name, PLAYERNAME, PLAYERNAME_SIZE, 1)))
    {
        //Replace the name with the username
        memcpy(name, username, usernameSize);
        //Move the punctuation backwards
        name[usernameSize] = name[PLAYERNAME_SIZE];
        //Fill the rest of the dummy name with spaces
        memset(name + usernameSize + 1, ' ', PLAYERNAME_SIZE - usernameSize);

#if 0
        int offset = PLAYERNAME_SIZE - usernameSize;

        //Move the rest of the message backwards till we hit the @
        for(name += usernameSize; name[offset] != '@'; name++)
            name[0] = name[offset];

        //Fill the end with spaces
        memset(name, ' ', offset + 1);
#endif
    }
}

void
func_SetNameEntry()
{
    //Overwrite username guess with real name
    memcpy(username, PLAYERNAME_ENTRY, PLAYERNAME_ENTRY_SIZE + 1);
    usernameSize = PLAYERNAME_ENTRY_SIZE;
}

void
func_ShakeWindow()
{
    //Get position of window
    RECT rect;
    GetWindowRect(window, &rect);

    //Shake it violently
    srand(time(NULL));
    int i = 70;
    for(; i >= 0; i--)
    {
        float angle = (rand() / (float)RAND_MAX) * M_PI * 2;
        int x = rect.left + cos(angle) * i;
        int y = rect.top + sin(angle) * i;
        SetWindowPos(window, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        Sleep(10);
    }

#if 0
    //Get size of screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    //Get size of window
    RECT windowRect;
    GetWindowRect(window, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    //Convert arg percentages to absolute positions
    int x = (screenWidth - windowWidth) * variables[VAR_ARG1] / 100;
    int y = (screenHeight - windowHeight) * variables[VAR_ARG2] / 100;

    SetWindowPos(window, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void
func_SetWallpaper()
{
    if(variables[VAR_ARG1] == 1)
    {
        wallpaper_free(&wallpaperOld);
        wallpaper_get(&wallpaperOld);

        //Set the wallpaper attributes
        Wallpaper paper;
        paper.filename = wcsdup(szDesktopPath);
        paper.style = STYLE_CENTER;
        paper.shading = SHADING_SOLID;
        paper.colorPrimary = 0x1a041f;
        paper.colorSecondary = 0;

        //Get wallpaper data and generate image/filename
        size_t size;
        unsigned char* data;
        util_getResource("WALLPAPER", (void**)&data, &size);
        wallpaper_gen(&paper, 321, 386, data);
        free(data);

        //Set it and free memory
        variables[VAR_RETURN] = wallpaper_set(&paper);
        wallpaper_free(&paper);
    }
    else if(variables[VAR_ARG1] == 0)
    {
        if(wallpaperOld.filename)
        {
            wallpaper_set(&wallpaperOld);
            wallpaper_free(&wallpaperOld);
            wallpaperOld.filename = NULL;
        }
    }
}

void
func_MessageBox()
{
    WCHAR buff[256];
    switch(variables[VAR_ARG1])
    {
    case 0:
        swprintf(buff, 256, STR_ONESHOT, username);
        util_messagebox(buff, L"", MESSAGE_INFO);
        oneshot = TRUE;
        util_updateQuitMessage();

        //Save a dead ending NOW, in case the program improperly terminates
        ending = ENDING_DEAD;
        util_saveEnding();
        break;
    case 1:
        variables[VAR_RETURN] = util_messagebox(STR_DO_YOU_UNDERSTAND, L"", MESSAGE_QUESTION);
        break;
    case 2:
        variables[VAR_RETURN] = util_messagebox(STR_STILL_HAVING_TROUBLE, L"", MESSAGE_QUESTION);
        break;
    case 3:
        variables[VAR_RETURN] = util_messagebox(STR_SURELY_YOU_KNOW, L"", MESSAGE_QUESTION);
        break;
    case 4:
        util_messagebox(STR_STILL_PLANNING, L"", MESSAGE_INFO);
        util_messagebox(STR_SUFFERING, L"", MESSAGE_INFO);
        break;
    case 5:
        util_messagebox(STR_GONE, L"Fatal Error", MESSAGE_ERROR);
        exit(0);
        break;
    case 6:
        util_messagebox(STR_WOULDA_REALIZED, L"", MESSAGE_INFO);
        util_messagebox(STR_HAD_ENOUGH, L"", MESSAGE_WARNING);
        util_messagebox(STR_DESTROY_IT, L"", MESSAGE_WARNING);
        util_messagebox(STR_QUIT_NOW, L"", MESSAGE_ERROR);
        break;
    case 7:
        util_messagebox(STR_BAD_DREAM, L"", MESSAGE_INFO);
        util_messagebox(STR_MISERABLE, L"", MESSAGE_INFO);
        util_messagebox(STR_YOU_MONSTER, L"", MESSAGE_INFO);
        break;
    }
}

void
func_LeaveWindow()
{
    //Update ending and quit message
    ending = ENDING_ESCAPED;

    wndKitty_start();
}

void
func_Save()
{
    saveFile = _wfopen(szSavePath, L"wb");
    if(!saveFile)
        return;

    //Write player name
    if(PLAYERNAME_ENTRY_SIZE == usernameSize && !strcmp(username, PLAYERNAME_ENTRY))
    {
        usernameSize = 0;
        fwrite(&usernameSize, 1, 1, saveFile);
    }
    else
    {
        fwrite(&usernameSize, 1, 1, saveFile);
        fwrite(username, usernameSize, 1, saveFile);
    }

    //Write switches and variables
    fwrite(switches, 200, 1, saveFile);
    fwrite(variables, 100 * 4, 1, saveFile);

    //The file isn't closed, because items will now be written to the file.
}

void
func_WriteItem()
{
    static BOOL firstZero = FALSE;

    fwrite(variables + VAR_ARG1, 1, 1, saveFile);
    if(variables[VAR_ARG1] == 0)
    {
        if(firstZero)
        {
            fclose(saveFile);

            //Don't kill Niko
            ending = ENDING_DEJAVU;
            forceEnding = TRUE;
            //Write the ending here in case it crashes
            util_saveEnding();
            exit(0);
        }
        firstZero = TRUE;
    }
}

void
func_Load()
{
    saveFile = _wfopen(szSavePath, L"rb");
    if(!saveFile)
    {
        variables[VAR_RETURN] = 0;
        return;
    }

    //Read username
    usernameSize = 0;
    fread(&usernameSize, 1, 1, saveFile);
    if(usernameSize)
    {
        fread(username, usernameSize, 1, saveFile);
        username[usernameSize] = 0;
    }

    //Read switches and variables
    fread(switches, 200, 1, saveFile);
    fread(variables, 100 * 4, 1, saveFile);

    if(usernameSize)
    {
        //Override return value
        variables[VAR_RETURN] = 1;
    }
    else
    {
        strcpy(username, PLAYERNAME_ENTRY);
        usernameSize = PLAYERNAME_ENTRY_SIZE;

        //Override return value
        variables[VAR_RETURN] = 2;
    }

    //Trigger Oneshot message
    oneshot = TRUE;
    util_updateQuitMessage();
    ending = ENDING_DEAD;

    //The file isn't closed, because now items will be read from the file
}

void
func_ReadItem()
{
    static BOOL firstZero = FALSE;

    variables[VAR_RETURN] = 0;
    fread(variables + VAR_RETURN, 1, 1, saveFile);
    if(variables[VAR_RETURN] == 0)
    {
        if(firstZero)
        {
            fclose(saveFile);
            DeleteFile(szSavePath);
        }
        else
            firstZero = TRUE;
    }
}

void
func_Document()
{
    //Load the text resource
    unsigned char *data1, *data2;
    size_t size1, size2;
    util_getResource("DOCUMENT1", (void*)&data1, &size1);
    util_getResource("DOCUMENT2", (void*)&data2, &size2);

    //Write the file
    FILE* file = _wfopen(szDocumentPath, L"wt");
    if(!file)
        return;
    fwrite(data1, size1 - 1, 1, file);
    fprintf(file, "%06d", variables[VAR_SAFE_CODE]);
    fwrite(data2, size2 - 1, 1, file);
    fclose(file);

#if 0
    //Write the file
    HANDLE file = CreateFileW(szDocumentPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return;

    if(winver == WIN_WINE)
    {
        WriteFile(file, data, size - 1, NULL, NULL);
    }
    else
    {
        //Write the UTF-8 BOM
        const char bom[3] = {0xEF, 0xBB, 0xBF};
        WriteFile(file, bom, 3, NULL, NULL);

        //Write the text, changing LF to CR+LF
        const char crlf[2] = {'\r', '\n'};
        int i = 0;
        for(; i < size - 1; i++)
        {
            if(data[i] == '\n')
                WriteFile(file, crlf, 2, NULL, NULL);
            else
                WriteFile(file, data + i, 1, NULL, NULL);
        }
    }

    //Write the ending
    char buff[256];
    sprintf(buff, "%06d.", variables[VAR_SAFE_CODE]);
    if(winver == WIN_WINE)
        strcat(buff, "\n");
    WriteFile(file, buff, strlen(buff), NULL, NULL);

    //Change the date to something cool
    FILETIME ft;
    ft.dwLowDateTime = 1;
    ft.dwHighDateTime = 0;
    SetFileTime(file, &ft, &ft, &ft);

    //Close it
    CloseHandle(file);
#endif
}

void
func_End()
{
    ending = variables[VAR_ARG1];
    forceEnding = TRUE;
    util_saveEnding();
}

void
func_SetCloseEnabled()
{
    closeEnabled = variables[VAR_ARG1];
}

//Functions
void (* const funcs[])(void) =
{
    func_GuessName,
    func_SetName,
    func_SetNameEntry,
    func_ShakeWindow,
    func_SetWallpaper,
    func_MessageBox,
    func_LeaveWindow,
    func_Save,
    func_WriteItem,
    func_Load,
    func_ReadItem,
    func_Document,
    func_End,
    func_SetCloseEnabled,
};
