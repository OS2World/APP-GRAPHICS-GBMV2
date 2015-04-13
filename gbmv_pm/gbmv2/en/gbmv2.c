/*

gbmv2.c - Display a bitmap

History:
--------
(Heiko Nitzsche)

26-Apr-2006: Fix issue with comma separation between file and options.
             Now the file can have quotes and thus clearly separating
             it from the options.
             On OS/2 command line use: "\"file.ext\",options"

27-May-2006: Fix display of 1bpp color bitmap for which no RGB color
             table was created and thus only the closest pure color
             was used. This caused wrong colors.

07-Sep-2007  Add interpolated scaling support with several filter types.
             (Version 1.40)

21-Jan-2008  Add support for 1bpp and 4bpp grayscale resampled scaling.
             (Version 1.42)

23-Mar-2008  (Version 1.50)
             Add Red Eye fix capability
             Use High Memory
             Fix some selection dependent menu activation
             Reorder some menu items to better logical groups
             Increase maximum allowed size for resize to 15000x15000
             Render bitmap creation replaced by direct rendering
              (Reduced memory consumption, large bitmap can now be shown)
             Remember main window size and restore on restart
             Remember last path in file open/save dialog
             Make background colour dark gray to put the bitmap colours more to front

1-Sep-2008   (Version 1.51)
             Adjust worker thread priority to idle time to make the system more responsible.
             Remember open/save format and path separately across sessions

3-Sep-2008   (Version 1.52)
             - Wrong initial filename creation if extension was not yet stored in profile
             - React on WM_xSCROLL in bitmap procedure messages so that mouse wheel scroll
               messages are processed

21-May-2009  (Version 1.54)
             - Fix handling of Black&White bitmaps (visualization, open, save, copy, paste).
               Some B&W bitmaps appeared inverted before.

21-May-2009  (Version 1.55)
             - Fix handling of file dialog when the path previously stored in profile
               does no longer exist. Now the current directory is used then.

21-Sep-2010  (Version 1.56)
             * Quality fine tuning of resampling scalers
               (flat colour areas are now preserved to get better quality)
             * Performance tuning of resampling scalers
               - Multithreading support (fully dynamic CPU core usage)
               - SSE support GCC and Open Watcom Compilers
             * Fix issue with automatic file extension update in file dialog

12-Oct-2010  (Version 1.57)
             - Fix: INI file sometimes not correctly closed which caused a file
                    handle leak
*/

/*...sincludes:0:*/
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DEV
#define  INCL_SPL
#define  INCL_SPLDOSPRINT
#define  INCL_BITMAPFILEFORMAT
#include <os2.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "scroll.h"
#include "gbm.h"
#include "gbmscale.h"
#include "gbmtool.h"
#include "gbmdlg.h"
#include "gbmdlgrc.h"
#include "gbmv2.h"
#include "gbmv2hlp.h"
#include "model.h"
#include "bmputils.h"
#include "help.h"

/*...vscroll\46\h:0:*/
/*...vgbm\46\h:0:*/
/*...vgbmdlg\46\h:0:*/
/*...vgbmdlgrc\46\h:0:*/
/*...vgbmv2\46\h:0:*/
/*...vgbmv2hlp\46\h:0:*/
/*...vmodel\46\h:0:*/
/*...vbmputils\46\h:0:*/
/*...vhelp\46\h:0:*/
/*...e*/

/* Application name */
static CHAR szAppName[] = "GbmV2";

/* Profile data */
static CHAR szProfName[_MAX_PATH+1] = { 0 };
static CHAR szProfKeyMainWindowSize[]          = "WINDOW_POS_MAIN";
static CHAR szProfKeyOpenFileDialogPath[]      = "PATH_FILE_DIALOG";
static CHAR szProfKeySaveFileDialogPath[]      = "PATH_SAVE_FILE_DIALOG";
static CHAR szProfKeyOpenFileDialogExtension[] = "FMT_FILE_DIALOG";
static CHAR szProfKeySaveFileDialogExtension[] = "FMT_SAVE_FILE_DIALOG";

/*...ssuppress warnings:0:*/
/* When using VisualAge C++, it complains about the definition of
   MPFROM2SHORT (well actually MAKELONG), so I've coded a replacement. */

#undef MPFROM2SHORT
#define    MY_MAKELONG(l,h) ((ULONG)(((USHORT)(l)) | ( ((ULONG)((USHORT)(h))) << 16 ) ))
#define    MPFROM2SHORT(l,h) ((MPARAM) MY_MAKELONG(l,h))
/*...e*/
/*...suseful:0:*/
/* Windows has these 2 */

/*...sEnableMenuItem:0:*/
static VOID EnableMenuItem(HWND hWndMenu, SHORT idMenuItem, BOOL fEnabled)
{
    WinSendMsg(hWndMenu, MM_SETITEMATTR,
           MPFROM2SHORT(idMenuItem, TRUE),
           MPFROM2SHORT(MIA_DISABLED, ( fEnabled ) ? 0 : MIA_DISABLED));
}
/*...e*/
/*...sCheckMenuItem:0:*/
static VOID CheckMenuItem(HWND hWndMenu, SHORT idMenuItem, BOOL fChecked)
{
    WinSendMsg(hWndMenu, MM_SETITEMATTR,
           MPFROM2SHORT(idMenuItem, TRUE),
           MPFROM2SHORT(MIA_CHECKED, ( fChecked ) ? MIA_CHECKED : 0));
}
/*...e*/

/* These are generally useful */

/*...sWarning:0:*/
static VOID Warning(HWND hwnd, const CHAR *szFmt, ...)
{
    va_list vars;
    CHAR sz[256+1];

    va_start(vars, szFmt);
    vsprintf(sz, szFmt, vars);
    va_end(vars);
    WinMessageBox(HWND_DESKTOP, hwnd, sz, NULL, 0, MB_OK | MB_WARNING | MB_MOVEABLE);
}
/*...e*/

static BOOL CheckFileExistsAndQueryOverride(HWND hwnd, const CHAR * filename)
{
  struct stat buf;

  if (stat(filename, &buf) != -1)
  {
    if (MBID_YES != WinMessageBox(HWND_DESKTOP,
                                 hwnd,
                                 "File exists already, override?",
                                 szAppName,
                                 0, MB_YESNO | MB_DEFBUTTON2 | MB_QUERY | MB_MOVEABLE))
    {
      return FALSE;
    }
  }
  return TRUE;
}


/*...sTidySysMenu:0:*/
static VOID TidySysMenu(HWND hwndFrame)
{
    HWND hwndSysMenu = WinWindowFromID(hwndFrame, FID_SYSMENU);
    USHORT id        = SHORT1FROMMR(WinSendMsg(hwndSysMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(0), NULL));
    MENUITEM mi;
    HWND hwndSysSubMenu;
    SHORT sInx, cItems;

    WinSendMsg(hwndSysMenu, MM_QUERYITEM, MPFROM2SHORT(id, FALSE), MPFROMP(&mi));
    hwndSysSubMenu = mi.hwndSubMenu;

    cItems = SHORT1FROMMR(WinSendMsg(hwndSysSubMenu, MM_QUERYITEMCOUNT, NULL, NULL));

    for ( sInx = cItems - 1; sInx >= 0; sInx-- )
    {
        id = SHORT1FROMMR(WinSendMsg(hwndSysSubMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(sInx), NULL));

        WinSendMsg(hwndSysSubMenu, MM_QUERYITEM,
               MPFROM2SHORT(id, FALSE), MPFROMP(&mi));
        if ( (mi.afStyle     & MIS_SEPARATOR) != 0 ||
             (mi.afAttribute & MIA_DISABLED ) != 0 ||
             id == (USHORT) SC_TASKMANAGER         )
            WinSendMsg(hwndSysSubMenu, MM_DELETEITEM,
                   MPFROM2SHORT(id, FALSE), NULL);
    }
}
/*...e*/
/*...sRestrictEntryfield:0:*/
/*
This is a function you call on an WC_ENTRYFIELD window that causes the window
to only allow strings in one set to be used and also not allow strings in
another set.
*/

typedef struct /* restr */
{
    const CHAR *szIfIn;
    const CHAR *szMapTo;
    const CHAR *szAllowed;
    const CHAR *szNotAllowed;
    PFNWP pfnwpOld;
} RESTR;

/*...sRestrictSubProc:0:*/
#define L_ENTRYMAX      500

/*...sMapChar:0:*/
static CHAR MapChar(RESTR *prestr, CHAR ch)
{
    CHAR *sz;

    if ( prestr -> szIfIn == NULL || prestr -> szMapTo == NULL )
            return ch;

    if ( (sz = strchr(prestr -> szIfIn, ch)) == NULL )
            return ch;

    return prestr -> szMapTo[sz - prestr -> szIfIn];
}
/*...e*/
/*...sProblemAllowed:0:*/
static BOOL ProblemAllowed(RESTR *prestr, CHAR ch)
{
    return prestr -> szAllowed != NULL && strchr(prestr -> szAllowed, ch) == NULL;
}                                                        /* More >>> */
/*...e*/
/*...sProblemNotAllowed:0:*/
static BOOL ProblemNotAllowed(RESTR *prestr, CHAR ch)
{
    return prestr -> szNotAllowed != NULL && strchr(prestr -> szNotAllowed, ch) != NULL;
}                                                        /* More >>> */
/*...e*/

MRESULT _System RestrictSubProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    RESTR *prestr = (RESTR *) WinQueryWindowULong(hwnd, QWL_USER);
    PFNWP pfnwpOld = prestr -> pfnwpOld;

    switch ( (int) msg )
    {
        /*...sWM_DESTROY \45\ clean up:16:*/
        case WM_DESTROY:
            free(prestr);
            break;
        /*...e*/
        /*...sWM_CHAR    \45\ map incoming chars:16:*/
        case WM_CHAR:
        {
            USHORT fs     = SHORT1FROMMP(mp1);
            CHAR   ch     = (CHAR) SHORT1FROMMP(mp2);
            USHORT vkey   = SHORT2FROMMP(mp2);

            if ( (fs & KC_VIRTUALKEY) != 0 && (fs & KC_ALT) != 0 &&
                 (vkey == VK_LEFT || vkey == VK_RIGHT || vkey == VK_UP || vkey == VK_DOWN) )
                /* If we let the entryfield get these, it won't process them */
                /* It will pass them to its owner, the dialog. */
                /* The dialog will use them to change the focus! */
                /* This is not desired, as Alt+NNN is being used */
                return (MRESULT) 0;

            if ( (fs & KC_CHAR) != 0 && ch != '\b' && ch != '\t' )
            {
                ch = MapChar(prestr, ch);

                if ( ProblemAllowed(prestr, ch) ||
                     ProblemNotAllowed(prestr, ch) )
                {
                    WinAlarm(HWND_DESKTOP, WA_WARNING);
                    return (MRESULT) 0;
                }
                mp2 = MPFROM2SHORT(ch, vkey);
            }
        }
            break;
        /*...e*/
        /*...sEM_PASTE   \45\ paste then alter chars:16:*/
        case EM_PASTE:
        {
            MRESULT mr = (*pfnwpOld)(hwnd, msg, mp1, mp2);
            MRESULT mrSel = WinSendMsg(hwnd, EM_QUERYSEL, NULL, NULL);
            CHAR sz[L_ENTRYMAX+1], *szTmp;

            WinQueryWindowText(hwnd, L_ENTRYMAX, sz);
            for ( szTmp = sz; *szTmp; szTmp++ )
            {
                *szTmp = MapChar(prestr, *szTmp);
                if ( ProblemAllowed(prestr, *szTmp) ||
                     ProblemNotAllowed(prestr, *szTmp) )
                {
                    strcpy(szTmp, szTmp + 1);
                    szTmp--;
                }
            }
            WinSetWindowText(hwnd, sz);
            WinSendMsg(hwnd, EM_SETSEL, mrSel, NULL);

            return mr;
        }
        /*...e*/
    }
    return (*pfnwpOld)(hwnd, msg, mp1, mp2);
}
/*...e*/

static BOOL RestrictEntryfield(
    HWND hwnd,
    const CHAR *szIfIn,
    const CHAR *szMapTo,
    const CHAR *szAllowed,
    const CHAR *szNotAllowed
    )
{
    RESTR *prestr;

    if ( (prestr = malloc(sizeof(RESTR))) == NULL )
            return FALSE;

    prestr -> szIfIn       = szIfIn      ;
    prestr -> szMapTo      = szMapTo     ;
    prestr -> szAllowed    = szAllowed   ;
    prestr -> szNotAllowed = szNotAllowed;
    prestr -> pfnwpOld     = WinSubclassWindow(hwnd, RestrictSubProc);

    WinSetWindowULong(hwnd, QWL_USER, (LONG) prestr);

    return TRUE;
}
/*...e*/
/*...spriority:0:*/
static VOID LowPri(VOID)
{
    DosSetPriority(PRTYS_THREAD, PRTYC_IDLETIME, PRTYD_MAXIMUM, 0);
}

static VOID RegPri(VOID)
{
    DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, 0, 0);
}
/*...e*/

/* ------------------------------------------------------- */
/* ------------------------------------------------------- */

static BOOL ExistsDirName(const char * szFullPath)
{
    FILESTATUS3  configInfo = { { 0 } };
    ULONG        ulBufSize  = sizeof(FILESTATUS3);
    CHAR         fullSystemPath[CCHMAXPATH+1] = { 0 };

    strcpy(fullSystemPath, szFullPath);
    strcat(fullSystemPath, ".");

    if (DosQueryPathInfo(fullSystemPath,
                         FIL_STANDARD,
                         &configInfo,
                         ulBufSize) != NO_ERROR)
    {
        return FALSE;
    }
    return TRUE;
}

static BOOL GetCurrentDir(char * szFullPath, const int szFullPathSize)
{
   CHAR szBuf[CCHMAXPATH+1] = { 0 };
   ULONG length = CCHMAXPATH;
   if (NO_ERROR == DosQueryCurrentDir(0, szBuf, &length))
   {
       /* get the full path including drive name */
       ULONG ulDriveNum = 0; /* Drive number (A=1, B=2, C=3, ...) */
       ULONG ulDriveMap = 0;

       if (NO_ERROR == DosQueryCurrentDisk(&ulDriveNum, &ulDriveMap))
       {
           if (szFullPathSize >= (strlen(szBuf) + 4))
           {
               sprintf(szFullPath, "%c:\\%s\\", (const char)('A' - 1 + ulDriveNum), szBuf);
               return TRUE;
           }
       }
   }
   return FALSE;
}

/* ------------------------------------------------------- */

static VOID ProfileSetProfileNameProgramName(const char * fullProgramName)
{
    char drive[_MAX_DRIVE+1] = { 0 };
    char dir[_MAX_DIR+1]     = { 0 };
    char name[_MAX_PATH+1]   = { 0 };

    strcpy(name, fullProgramName);
    _splitpath(name, drive, dir, NULL, NULL);
    _makepath (szProfName     , drive, dir, "gbmv2.ini", NULL);
}

/* ------------------------------------------------------- */

static VOID ProfileSetWindowPos(
  HWND hwnd, char * szWindowSizeKeyName)
{
    HAB  hab      = WinQueryAnchorBlock(hwnd);
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        /* Query width and height of main window  */
        SWP swp  = { 0 };
        if (WinQueryWindowPos(hwnd, &swp))
        {
            char buf[34*4] = { 0 };
            sprintf(buf, "%li,%li,%li,%li", swp.x, swp.y, swp.cx, swp.cy);
            PrfWriteProfileString(hIniProf, szAppName, szWindowSizeKeyName, buf);
        }
        PrfCloseProfile(hIniProf);
    }
}

static BOOL ProfileGetWindowPos(
  HWND hwnd, char * szWindowSizeKeyName,
  LONG * pX, LONG * pY, LONG * pW, LONG * pH)
{
    HAB  hab      = WinQueryAnchorBlock(hwnd);
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        char buf[34*4] = { 0 };
        ULONG dataLen = PrfQueryProfileString(hIniProf,
                                              szAppName,
                                              szWindowSizeKeyName,
                                              NULL, buf, sizeof(buf));
        if (dataLen > 0)
        {
            if (4 == sscanf(buf, "%li,%li,%li,%li", pX, pY, pW, pH))
            {
                PrfCloseProfile(hIniProf);
                return TRUE;
            }
        }
        PrfCloseProfile(hIniProf);
    }
    return FALSE;
}

/* ------------------------------------------------------- */

static VOID ProfileSetOpenFileDialogPathFromFullFile(
  HAB hab, char * szFullFile)
{
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        /* extract drive and path */
        char fullPath[_MAX_PATH+1] = { 0 };
        char drive[_MAX_DRIVE+1]   = { 0 };
        char dir[_MAX_DIR+1]       = { 0 };
        char ext[_MAX_EXT+1]       = { 0 };

        _splitpath(szFullFile, drive, dir, NULL, ext);
        _makepath (fullPath  , drive, dir, NULL, NULL);

        PrfWriteProfileString(hIniProf, szAppName, szProfKeyOpenFileDialogPath, fullPath);
        if (strlen(ext) > 0)
        {
            PrfWriteProfileString(hIniProf, szAppName, szProfKeyOpenFileDialogExtension, ext);
        }
        PrfCloseProfile(hIniProf);
    }
}

static BOOL ProfileGetOpenFileDialogPath(
  HAB hab, char * szFullPath, const int szFullPathSize, char * szExtension, const int szExtensionSize)
{
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        ULONG dataLen = PrfQueryProfileString(hIniProf,
                                              szAppName,
                                              szProfKeyOpenFileDialogPath,
                                              NULL, szFullPath, szFullPathSize);
        if ((dataLen > 0)          &&
            (dataLen < CCHMAXPATH) &&
            (dataLen < szFullPathSize)) /* must be 1 char unused for . */
        {
            if (! ExistsDirName(szFullPath))
            {
                PrfCloseProfile(hIniProf);
                return FALSE;
            }
        }
        else
        {
            PrfCloseProfile(hIniProf);
            return FALSE;
        }
        if (szExtension != NULL)
        {
            dataLen = PrfQueryProfileString(hIniProf,
                                            szAppName,
                                            szProfKeyOpenFileDialogExtension,
                                            NULL, szExtension, szExtensionSize);
            if (dataLen == 0)
            {
                memset(szExtension, 0, szExtensionSize);
            }
        }

        PrfCloseProfile(hIniProf);
        return TRUE;
    }
    return FALSE;
}

/* ------------------------------------------------------- */

static VOID ProfileSetSaveFileDialogPathFromFullFile(
  HAB hab, char * szFullFile)
{
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        /* extract drive and path */
        char fullPath[_MAX_PATH+1] = { 0 };
        char drive[_MAX_DRIVE+1]   = { 0 };
        char dir[_MAX_DIR+1]       = { 0 };
        char ext[_MAX_EXT+1]       = { 0 };

        _splitpath(szFullFile, drive, dir, NULL, ext);
        _makepath (fullPath  , drive, dir, NULL, NULL);

        PrfWriteProfileString(hIniProf, szAppName, szProfKeySaveFileDialogPath, fullPath);
        if (strlen(ext) > 0)
        {
            PrfWriteProfileString(hIniProf, szAppName, szProfKeySaveFileDialogExtension, ext);
        }
        PrfCloseProfile(hIniProf);
    }
}

static BOOL ProfileGetSaveFileDialogPath(
  HAB hab, char * szFullPath, const int szFullPathSize, char * szExtension, const int szExtensionSize)
{
    HINI hIniProf = PrfOpenProfile(hab, szProfName);
    if (hIniProf != NULLHANDLE)
    {
        ULONG dataLen = PrfQueryProfileString(hIniProf,
                                              szAppName,
                                              szProfKeySaveFileDialogPath,
                                              NULL, szFullPath, szFullPathSize);
        if ((dataLen > 0)          &&
            (dataLen < CCHMAXPATH) &&
            (dataLen < szFullPathSize)) /* must be 1 char unused for . */
        {
            if (! ExistsDirName(szFullPath))
            {
                PrfCloseProfile(hIniProf);
                return FALSE;
            }
        }
        else
        {
            PrfCloseProfile(hIniProf);
            return FALSE;
        }
        if (szExtension != NULL)
        {
           dataLen = PrfQueryProfileString(hIniProf,
                                           szAppName,
                                           szProfKeySaveFileDialogExtension,
                                           NULL, szExtension, szExtensionSize);
           if (dataLen == 0)
           {
               memset(szExtension, 0, szExtensionSize);
           }
        }

        PrfCloseProfile(hIniProf);
        return TRUE;
    }
    return FALSE;
}

/* ------------------------------------------------------- */
/* ------------------------------------------------------- */

static BOOL isGrayscalePalette(const MOD * mod)
{
  if ((mod->gbm.bpp <= 8) && (mod->gbm.bpp > 0))
  {
    const int     entries = 1 << mod->gbm.bpp;
    const GBMRGB * gbmrgb = &(mod->gbmrgb[0]);

    int i;
    for (i = 0; i < entries; i++)
    {
      if ((gbmrgb[i].r != gbmrgb[i].g) ||
          (gbmrgb[i].r != gbmrgb[i].b) ||
          (gbmrgb[i].g != gbmrgb[i].b))
      {
        return FALSE;
      }
    }
    return TRUE;
  }
  return FALSE;
}

/* ------------------------------------------------------- */

/*...svars:0:*/
/* Global single help instance */
static HWND hwndHelp = NULLHANDLE;

/* Whats the bitmap data called */
#define    MAX_FILE_NAME GBMTOOL_FILENAME_MAX
static CHAR szFileName[MAX_FILE_NAME+1] = "";
static CHAR *FileName(VOID) { return szFileName[0] != '\0' ? szFileName : "(untitled)"; }
static BOOL HasNonEmptyFileName(VOID) { return szFileName[0] != '\0' ? TRUE : FALSE; }

/* The bitmap data itself */
static BITMAP drawingBitmap;
static BOOL fGotBitmap = FALSE;
static BOOL fUnsavedChanges = FALSE;

/* How we like to view the bitmaps */
#define    VIEW_NULL    0
#define    VIEW_HALFTONE    1
#define    VIEW_ERRDIFF    2
static BYTE bView = VIEW_NULL;
static LONG lBitCountScreen;

/* General application vars */
static BOOL fBusy = FALSE;
static HWND hwndObject;
static HWND hwndBitmap;
static HWND hwndScroller;

/* Selections in the current bitmap */
static BOOL fSelectionDefined = FALSE;
static RECTL rclSelection;
/*...e*/
/*...sundo:0:*/
/* Most replaced data gets stored here, so we can bring it back later */

static BOOL fCanUndo = FALSE;
static BITMAP bmpUndo;
static const CHAR *szWhatUndo;

/*...sDeleteBitmap:0:*/
static VOID DeleteBitmap(BITMAP * pbmp)
{
    memset(&(pbmp->bmp2), 0, sizeof(sizeof(BITMAPINFOHEADER2)));
    memset(pbmp->argb2Color, 0, sizeof(pbmp->argb2Color));
    ModDelete(&(pbmp->mod));
}
/*...e*/

/*...sDiscardUndo:0:*/
static VOID DiscardUndo(VOID)
{
    if ( fCanUndo )
    {
        DeleteBitmap(&bmpUndo);
        fCanUndo = FALSE;
    }
}
/*...e*/
/*...sKeepForUndo:0:*/
static VOID KeepForUndo(const BITMAP *pbmp, const CHAR *szWhat)
{
    DiscardUndo();
    if ( ModCopy(&(pbmp->mod), &(bmpUndo.mod)) != MOD_ERR_OK )
    {
        return; /* Silently fail to keep undo information */
    }
    bmpUndo.bmp2 = pbmp->bmp2;
    memcpy(bmpUndo.argb2Color, pbmp->argb2Color, sizeof(bmpUndo.argb2Color));

    szWhatUndo = szWhat;
    fCanUndo = TRUE;
}
/*...e*/
/*...sUseUndoBuffer:0:*/
static VOID UseUndoBuffer(
    BITMAP **pbmpToUndoTo,
    const CHAR **pszWhat
    )
{
    *pbmpToUndoTo = &bmpUndo;
    *pszWhat = szWhatUndo;
    fCanUndo = FALSE;
}
/*...e*/
/*...e*/
/*...sCaption:0:*/
static VOID Caption(HWND hwndClient, const CHAR *szFmt, ...)
{
    va_list vars;
    HWND hwndFrame = WinQueryWindow(hwndClient, QW_PARENT);
    CHAR sz[50+MAX_FILE_NAME+50+1], *szAppend;

    strcpy(sz, szAppName);

    szAppend = sz + strlen(sz);

    va_start(vars, szFmt);
    vsprintf(szAppend, szFmt, vars);
    va_end(vars);

    WinSetWindowText(hwndFrame, sz);
}
/*...e*/
/*...sMakeVisual:0:*/
static BOOL MakeVisual(
    MOD *mod,
    BYTE bView,
    BITMAP *pbmp)
{
    MOD_ERR mrc = MOD_ERR_OK; MOD modTmp, *modVisual = mod;
    BOOL fOk;

    switch ( bView )
    {
        /*...sVIEW_HALFTONE:16:*/
        case VIEW_HALFTONE:
            switch ( lBitCountScreen )
            {
                case 1:
                    /* Are there any 1bpp screens still in existence? */
                    mrc = ModBppMap(mod, CVT_BW, CVT_NEAREST, 8,8,8, 2, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 4:
                    mrc = ModBppMap(mod, CVT_VGA, CVT_HALFTONE, 8,8,8, 16, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 8:
                    mrc = ModBppMap(mod, CVT_784, CVT_HALFTONE, 8,8,8, 256, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 16:
                    mrc = ModBppMap(mod, CVT_RGB, CVT_HALFTONE, 5,6,5, 65536, &modTmp);
                    modVisual = &modTmp;
                    break;
            }
            break;
        /*...e*/
        /*...sVIEW_ERRDIFF:16:*/
        case VIEW_ERRDIFF:
            switch ( lBitCountScreen )
            {
                case 1:
                    /* Are there any 1bpp screens still in existence? */
                    mrc = ModBppMap(mod, CVT_BW, CVT_ERRDIFF, 8,8,8, 2, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 4:
                    mrc = ModBppMap(mod, CVT_VGA, CVT_ERRDIFF, 8,8,8, 16, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 8:
                    mrc = ModBppMap(mod, CVT_784, CVT_ERRDIFF, 8,8,8, 256, &modTmp);
                    modVisual = &modTmp;
                    break;
                case 16:
                    mrc = ModBppMap(mod, CVT_RGB, CVT_ERRDIFF, 5,6,5, 65536, &modTmp);
                    modVisual = &modTmp;
                    break;
            }
            break;
        /*...e*/
    }

    if ( mrc != MOD_ERR_OK )
    {
        if ( modVisual != mod )
           ModDelete(modVisual);
        return FALSE; /* Unable to make improved quality bitmap */
    }

    fOk = ( ModMakeBITMAP(modVisual, pbmp) == MOD_ERR_OK );
    if ( modVisual != mod )
        ModDelete(modVisual);
    return fOk;
}
/*...e*/
/*...sMakeVisualBg:0:*/
static BOOL MakeVisualBg(
    HWND hwnd,
    MOD *mod,
    BYTE bView,
    BITMAP *pbmp,
    CHAR *szFileName)
{
    LowPri();
    /* Tentatively use new supplied filename, rather than current */
    Caption(hwnd, " - rendering %s", szFileName);
    if ( !MakeVisual(mod, bView, pbmp) )
    {
        /* Revert to previous filename */
        Caption(hwnd, " - %s", FileName());
        RegPri();
        Warning(hwnd, "Error creating view bitmap");
        return FALSE;
    }
    /* Leave with new supplied filename on display */
    Caption(hwnd, " - %s", szFileName);
    RegPri();
    return TRUE;
}
/*...e*/
/*...sSaveChanges:0:*/
/*
Returns TRUE if changes saved ok.
Returns FALSE if cancel selected.
*/

static BOOL SaveChanges(HWND hwnd)
{
    CHAR sz[255];

    if ( !fGotBitmap )
        return TRUE;

    if ( !fUnsavedChanges )
        return TRUE;

    sprintf(sz, "Save current changes: %s", FileName());
    switch ( WinMessageBox(HWND_DESKTOP, hwnd, sz, szAppName, 0, MB_YESNOCANCEL | MB_WARNING | MB_MOVEABLE) )
    {
        case MBID_YES:
        {
            GBMFILEDLG gbmfild;
            MOD_ERR mrc;

            memset(&gbmfild.fild, 0, sizeof(FILEDLG));
            gbmfild.fild.cbSize = sizeof(FILEDLG);
            gbmfild.fild.fl = (FDS_CENTER|FDS_SAVEAS_DIALOG|FDS_ENABLEFILELB|FDS_HELPBUTTON);
            strcpy(gbmfild.fild.szFullFile, szFileName);
            strcpy(gbmfild.szOptions, "");
            while ( gbmfild.fild.szFullFile[0] == '\0' )
            /* Try to correct filename */
            {
                GbmFileDlg(HWND_DESKTOP, hwnd, &gbmfild);
                if ( gbmfild.fild.lReturn != DID_OK )
                {
                    return FALSE;
                }
                if (! CheckFileExistsAndQueryOverride(hwnd, gbmfild.fild.szFullFile))
                {
                    return FALSE;
                }
            }

            if ( (mrc = ModWriteToFile(&(drawingBitmap.mod), gbmfild.fild.szFullFile, gbmfild.szOptions)) != MOD_ERR_OK )
            {
                Warning(hwnd, "Error saving %s: %s", gbmfild.fild.szFullFile, ModErrorString(mrc));
                return FALSE;
            }
            strcpy(szFileName, gbmfild.fild.szFullFile);
            fUnsavedChanges = FALSE;
            return TRUE;
        }
        case MBID_NO:
            return TRUE;
        case MBID_CANCEL:
            return FALSE;
    }
    /* NOT REACHED */
    return FALSE; /* Keep fussy compiler happy */
}
/*...e*/
/*...sBitmapWndProc:0:*/
#define    WC_BITMAP "GbmV2BitmapViewerClass"

static BOOL fTracking = FALSE;

/*
This is a pretty simple window class. Its aim in life is to display the bitmap
held by the global struct 'drawingBitmap'.
*/
static HMTX hmtxBmp;

static VOID RequestBmp(VOID)
{
    DosRequestMutexSem(hmtxBmp, SEM_INDEFINITE_WAIT);
}

static VOID ReleaseBmp(VOID)
{
    DosReleaseMutexSem(hmtxBmp);
}

MRESULT _System BitmapWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( (int) msg )
    {
        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            /* forward the scroll messages to the frame so that mouse wheel is processed */
            HWND hwndFrame = WinQueryWindow(hwnd, QW_PARENT);

            if ( fTracking )
                /* Swallow the key */
                return (MRESULT) 0;

            return WinSendMsg(hwndFrame, msg, mp1, mp2);
        }

        /*...sWM_PAINT       \45\ repaint client area:16:*/
        case WM_PAINT:
        {
            RECTL rclUpdate;
            HPS hps = WinBeginPaint(hwnd, (HPS) NULL, &rclUpdate);

            RequestBmp();

            if (drawingBitmap.mod.pbData != NULL)
            {
                /* Draw the bitmap */
                RECTL aptlPoints[2] = { { 0, 0, 0, 0 } };

                /* set forground/background color if we render a 1bpp bitmap */
                if (drawingBitmap.bmp2.cBitCount == 1)
                {
                    const RGB2 * pRGBA = drawingBitmap.argb2Color;

                    const LONG colorFg = (pRGBA[0].bRed << 16) + (pRGBA[0].bGreen << 8) + pRGBA[0].bBlue;
                    const LONG colorBg = (pRGBA[1].bRed << 16) + (pRGBA[1].bGreen << 8) + pRGBA[1].bBlue;

                    LONG colorTable[2];
                    colorTable[0] = colorBg;
                    colorTable[1] = colorFg;

                    GpiCreateLogColorTable(hps, 0, LCOLF_CONSECRGB, 0, 2, (PLONG) colorTable);
                    /* if something goes wrong when creating color table, use the closest color */
                    GpiSetBackColor(hps, GpiQueryColorIndex(hps, 0, colorBg));
                    GpiSetColor    (hps, GpiQueryColorIndex(hps, 0, colorFg));
                }

                aptlPoints[0] = rclUpdate;
                aptlPoints[1] = rclUpdate;

                /* Target coordinates are (model : inclusive,inclusive).
                   Source coordinates are (device: inclusive,non-inclusive).
                   This means that if the lower left corner is equal to the upper right corner,
                   the image size is empty.
                   From WinBeginPaint() we get device coordinates which can be directly
                   used by GpiDrawBits() as source coordinates. But because the target
                   coordinates are (inclusive,inclusive) we have to correct them.
                 */
                (aptlPoints[0].xRight)--;
                (aptlPoints[0].yTop)--;

                GpiDrawBits(hps,
                            (PBYTE) drawingBitmap.mod.pbData,
                            (PBITMAPINFO2)(&drawingBitmap),
                            4L,
                            (PPOINTL) aptlPoints,
                             ROP_SRCCOPY,
                             BBO_IGNORE);

                if ( fSelectionDefined )
                {
                    POINTL ptl;

                    GpiSetMix(hps, FM_INVERT);
                    ptl.x = rclSelection.xLeft;
                    ptl.y = rclSelection.yBottom;
                    GpiMove(hps, &ptl);
                    ptl.x = rclSelection.xRight - 1L;
                    ptl.y = rclSelection.yTop   - 1L;
                    GpiBox(hps, DRO_OUTLINE, &ptl, 0L, 0L);
                }
            }

            ReleaseBmp();

            WinEndPaint(hps);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sWM_CHAR        \45\ got a character from the user:16:*/
        #define    KS(vkey,kc) ((vkey)+((kc&(KC_ALT|KC_SHIFT|KC_CTRL))<<16))

        case WM_CHAR:
        {
            USHORT fs        = SHORT1FROMMP(mp1);
            CHAR   ch        = (CHAR) SHORT1FROMMP(mp2);
            USHORT vkey      = SHORT2FROMMP(mp2);
            HWND hwndHscroll = WinWindowFromID(hwndScroller, SCID_HSCROLL);
            HWND hwndVscroll = WinWindowFromID(hwndScroller, SCID_VSCROLL);

            if ( fTracking )
                /* Swallow the key */
                return (MRESULT) 0;

            if ( fs & KC_VIRTUALKEY )
            {
                switch ( KS(vkey,fs) )
                {
                    case KS(VK_LEFT,0):
                    case KS(VK_RIGHT,0):
                        return WinSendMsg(hwndHscroll, msg, mp1, mp2);
                    case KS(VK_LEFT,KC_SHIFT):
                        fs &= ~KC_SHIFT;
                        vkey = VK_PAGEUP;
                        mp1 = MPFROMSHORT(fs);
                        mp2 = MPFROM2SHORT(ch, vkey);
                        return WinSendMsg(hwndHscroll, msg, mp1, mp2);
                    case KS(VK_RIGHT,KC_SHIFT):
                        fs &= ~KC_SHIFT;
                        vkey = VK_PAGEDOWN;
                        mp1 = MPFROMSHORT(fs);
                        mp2 = MPFROM2SHORT(ch, vkey);
                        return WinSendMsg(hwndHscroll, msg, mp1, mp2);
                    case KS(VK_UP,0):
                    case KS(VK_DOWN,0):
                    case KS(VK_PAGEUP,0):
                    case KS(VK_PAGEDOWN,0):
                        return WinSendMsg(hwndVscroll, msg, mp1, mp2);
                    case KS(VK_UP,KC_SHIFT):
                        fs &= ~KC_SHIFT;
                        vkey = VK_PAGEUP;
                        mp1 = MPFROMSHORT(fs);
                        mp2 = MPFROM2SHORT(ch, vkey);
                        return WinSendMsg(hwndVscroll, msg, mp1, mp2);
                    case KS(VK_DOWN,KC_SHIFT):
                        fs &= ~KC_SHIFT;
                        vkey = VK_PAGEDOWN;
                        mp1 = MPFROMSHORT(fs);
                        mp2 = MPFROM2SHORT(ch, vkey);
                        return WinSendMsg(hwndVscroll, msg, mp1, mp2);
                }
            }
            break;
        }
        /*...e*/
        /*...sWM_BUTTON2DOWN \45\ cancel current selection:16:*/
        case WM_BUTTON2DOWN:
            fSelectionDefined = FALSE;
            WinInvalidateRect(hwnd, NULL, TRUE);
            WinUpdateWindow(hwnd);
            break;
        /*...e*/
        /*...sWM_MOUSEMOVE   \45\ hourglass:16:*/
        case WM_MOUSEMOVE:
            if ( fTracking )
                return (MRESULT) 0;

            if ( fBusy )
            {
                WinSetPointer(HWND_DESKTOP,
                    WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE));
                return (MRESULT) 0;
            }
            break;
        /*...e*/
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

/*...sSetBitmap:0:*/
/* Assured bmpNew != NULL on entry */

static VOID SetBitmap(const BITMAP * bmpNew)
{
    const BOOL fOld = (drawingBitmap.mod.pbData != NULL);
    LONG cx, cy;
    RequestBmp();

    if (bmpNew->mod.pbData == drawingBitmap.mod.pbData)
    {
        ReleaseBmp();
        return;
    }

    DeleteBitmap(&drawingBitmap);
    if (ModCopy(&(bmpNew->mod), &(drawingBitmap.mod)) == MOD_ERR_OK)
    {
        drawingBitmap.bmp2 = bmpNew->bmp2;
        memcpy(drawingBitmap.argb2Color, bmpNew->argb2Color, sizeof(drawingBitmap.argb2Color));
    }

    cx = drawingBitmap.bmp2.cx;
    cy = drawingBitmap.bmp2.cy;

    ReleaseBmp();

    WinShowWindow(hwndBitmap, FALSE);
    WinSetWindowPos(hwndBitmap, (HWND) NULL, 0, 0, cx, cy, SWP_SIZE);

    if ( fOld )
    {
        WinSendMsg(hwndScroller, SCM_SIZE, MPFROMHWND(hwndBitmap), NULL);
    }
    else
    {
        WinSendMsg(hwndScroller, SCM_CHILD, MPFROMHWND(hwndBitmap), NULL);
    }
    WinShowWindow(hwndBitmap, TRUE);
    WinInvalidateRect(hwndBitmap, NULL, TRUE);
    WinUpdateWindow(hwndBitmap);
}
/*...e*/
/*...sSetNoBitmap:0:*/
static VOID SetNoBitmap(VOID)
{
    const BOOL fOld = (drawingBitmap.mod.pbData != NULL);
    RequestBmp();

    if (drawingBitmap.mod.pbData == NULL)
    {
        ReleaseBmp();
        return;
    }
    if ( fOld )
    {
        DeleteBitmap(&drawingBitmap);
    }

    ReleaseBmp();

    WinSetWindowPos(hwndBitmap, (HWND) NULL, 0,0,0,0, SWP_SIZE);

    if ( fOld )
    {
        WinSendMsg(hwndScroller, SCM_CHILD, MPFROMHWND((HWND) NULL), NULL);
    }

    WinInvalidateRect(hwndScroller, NULL, TRUE);
    WinUpdateWindow(hwndScroller);
}
/*...e*/
/*...sObjectWndProc:0:*/
#define    WC_GBMV2_OBJECT    "GbmV2ObjectWindowClass"

/*...sUM_ user window messages:0:*/
#define UM_NEW            WM_USER
#define UM_OPEN          (WM_USER+ 1)
#define UM_SAVE          (WM_USER+ 2)
#define UM_SAVE_AS       (WM_USER+ 3)
#define UM_EXPORT_MET    (WM_USER+ 4)
#define UM_PRINT         (WM_USER+ 5)
#define UM_SNAPSHOT      (WM_USER+ 6)
#define UM_UNDO          (WM_USER+ 7)
#define UM_SELECT        (WM_USER+ 8)
#define UM_DESELECT      (WM_USER+ 9)
#define UM_COPY          (WM_USER+10)
#define UM_PASTE         (WM_USER+11)
#define UM_REF_HORZ      (WM_USER+12)
#define UM_REF_VERT      (WM_USER+13)
#define UM_ROT_90        (WM_USER+14)
#define UM_ROT_180       (WM_USER+15)
#define UM_ROT_270       (WM_USER+16)
#define UM_TRANSPOSE     (WM_USER+17)
#define UM_CROP          (WM_USER+18)
#define UM_COLOUR        (WM_USER+19)
#define UM_MAP           (WM_USER+20)
#define UM_REDEYE        (WM_USER+21)
#define UM_RESIZE        (WM_USER+22)
#define UM_VIEW_NULL     (WM_USER+23)
#define UM_VIEW_HALFTONE (WM_USER+24)
#define UM_VIEW_ERRDIFF  (WM_USER+25)
#define UM_ABOUT         (WM_USER+26)
#define UM_DONE          (WM_USER+27)
/*...e*/

/*...sTracking:0:*/
static HWND hwndClientTracking;
static TRACKINFO ti;

/*
The documentation says that ti.rclTrack is updated to the current values before
the hook functions are called. Under OS/2 2.1 GA (and perhaps other versions),
this is not the case. So I will code this differently.
*/

#ifndef MSGF_TRACK
   #define MSGF_TRACK  8  /* This is an OS/2 2.1x definition not part of Open Watcom default include */
#endif

/*...sTrackPosHook:0:*/
static BOOL APIENTRY TrackPosHook(HAB hab, QMSG *pqmsg, ULONG msgf)
{
    hab=hab; /* Suppress warning */
    if ( msgf == MSGF_TRACK )
    {
        POINTL ptl = pqmsg->ptl;
        WinMapWindowPoints(pqmsg->hwnd, hwndBitmap, &ptl, 1L);
        if ( ptl.x < 0L )
            ptl.x = 0L;
        else if ( ptl.x > drawingBitmap.mod.gbm.w )
            ptl.x = drawingBitmap.mod.gbm.w;
        if ( ptl.y < 0L )
            ptl.y = 0L;
        else if ( ptl.y > drawingBitmap.mod.gbm.h )
            ptl.y = drawingBitmap.mod.gbm.h;
        Caption(hwndClientTracking, " - at %ldx%ld", ptl.x, ptl.y);
    }
    return FALSE; /* Pass on to next hook etc. / ie: don't swallow msg */
}
/*...e*/
/*...sTrackSizeHook:0:*/
static BOOL APIENTRY TrackSizeHook(HAB hab, QMSG *pqmsg, ULONG msgf)
{
    hab=hab; /* Suppress warning */
    if ( msgf == MSGF_TRACK )
    {
        POINTL ptl = pqmsg->ptl;
        SIZEL sizl;
        WinMapWindowPoints(pqmsg->hwnd, hwndBitmap, &ptl, 1L);
        sizl.cx = (ptl.x - ti.rclTrack.xLeft  );
        sizl.cy = (ptl.y - ti.rclTrack.yBottom);
        if ( sizl.cx < 0L )
            sizl.cx = 0L;
        else if ( sizl.cx > drawingBitmap.mod.gbm.w )
            sizl.cx = drawingBitmap.mod.gbm.w;
        if ( sizl.cy < 0L )
            sizl.cy = 0L;
        else if ( sizl.cy > drawingBitmap.mod.gbm.h )
            sizl.cy = drawingBitmap.mod.gbm.h;
        Caption(hwndClientTracking, " - at %ldx%ld, size %ldx%ld",
            (long) ti.rclTrack.xLeft  ,
            (long) ti.rclTrack.yBottom,
            (long) sizl.cx,
            (long) sizl.cy);
    }
    return FALSE; /* Pass on to next hook etc. / ie: don't swallow msg */
}
/*...e*/

static BOOL Tracking(
    HWND hwndClient,
    HWND hwnd,
    RECTL *prclTrack,
    int x, int y, int cx, int cy,
    HPOINTER hptr1, HPOINTER hptr2
    )
{
    HAB hab = WinQueryAnchorBlock(hwnd);
    HMQ hmq = HMQ_CURRENT;
    BOOL f;

    hwndClientTracking = hwndClient;

    ti.cxBorder   = 1;
    ti.cyBorder   = 1;
    ti.cxGrid     = 0;
    ti.cyGrid     = 0;
    ti.cxKeyboard = 4;
    ti.cyKeyboard = 4;

    ti.rclBoundary.xLeft   = x;
    ti.rclBoundary.xRight  = x + cx;
    ti.rclBoundary.yBottom = y;
    ti.rclBoundary.yTop    = y + cy;

    ti.ptlMinTrackSize.x = 1;
    ti.ptlMinTrackSize.y = 1;

    ti.ptlMaxTrackSize.x = cx;
    ti.ptlMaxTrackSize.y = cy;

    ti.rclTrack.xLeft   = x + cx/4;
    ti.rclTrack.xRight  = x + cx/4;
    ti.rclTrack.yBottom = y + cy/4;
    ti.rclTrack.yTop    = y + cy/4;

    /*...smove pointer:8:*/
    {
        POINTL ptl;

        ptl.x = ti.rclTrack.xLeft;
        ptl.y = ti.rclTrack.yBottom;
        WinMapWindowPoints(hwnd, HWND_DESKTOP, &ptl, 1);
        WinSetPointerPos(HWND_DESKTOP, ptl.x, ptl.y);
    }
    /*...e*/

    ti.fs = TF_MOVE | TF_STANDARD | TF_SETPOINTERPOS | TF_ALLINBOUNDARY;
    WinSetPointer(HWND_DESKTOP, hptr1);
    WinSetHook(hab, hmq, HK_MSGFILTER, (PFN) TrackPosHook, (HMODULE) NULL);
    f = WinTrackRect(hwnd, (HPS) NULL, &ti);
    WinReleaseHook(hab, hmq, HK_MSGFILTER, (PFN) TrackPosHook, (HMODULE) NULL);
    Caption(hwndClient, " - %s", FileName());
    if ( !f )
        return FALSE;

    ti.fs = TF_RIGHT | TF_TOP | TF_STANDARD | TF_SETPOINTERPOS | TF_ALLINBOUNDARY;
    WinSetPointer(HWND_DESKTOP, hptr2);
    WinSetHook(hab, hmq, HK_MSGFILTER, (PFN) TrackSizeHook, (HMODULE) NULL);
    f = WinTrackRect(hwnd, (HPS) NULL, &ti);
    WinReleaseHook(hab, hmq, HK_MSGFILTER, (PFN) TrackSizeHook, (HMODULE) NULL);
    Caption(hwndClient, " - %s", FileName());
    if ( !f )
        return FALSE;

    *prclTrack = ti.rclTrack;

    return TRUE;
}
/*...e*/
/*...sSelect:0:*/
static BOOL Select(
    HWND hwndClient,
    HWND hwnd,
    RECTL *prclTrack,
    int x, int y, int cx, int cy
    )
{
    HPOINTER hptr1 = WinLoadPointer(HWND_DESKTOP, (HMODULE) NULL, RID_SELECT1);
    HPOINTER hptr2 = WinLoadPointer(HWND_DESKTOP, (HMODULE) NULL, RID_SELECT2);
    BOOL f;

    fTracking = TRUE;
    f = Tracking(hwndClient, hwnd, prclTrack, x, y, cx, cy, hptr1, hptr2);
    fTracking = FALSE;

    WinDestroyPointer(hptr1);
    WinDestroyPointer(hptr2);

    return f;
}
/*...e*/
/*...sCopyToClipbrd:0:*/
static VOID CopyToClipbrd(HWND hwndClient, RECTL rcl)
{
    MOD modRect;
    MOD_ERR mrc;
    HMF hmf;

    /* extract the subrectangle */
    mrc = ModExtractSubrectangle(&(drawingBitmap.mod),
                                 rcl.xLeft, rcl.yBottom,
                                 rcl.xRight - rcl.xLeft,
                                 rcl.yTop - rcl.yBottom,
                                 &modRect);
    if (mrc != MOD_ERR_OK)
    {
        Warning(hwndClient, "Error copying to clipboard: %s", ModErrorString(mrc));
    }
    else
    {
        HAB hab = WinQueryAnchorBlock(hwndClient);
        HBITMAP hbmClipbrd = (HBITMAP) NULL;

        mrc = ModMakeHBITMAP(&modRect, hab, &hbmClipbrd);
        ModDelete(&modRect);
        if (mrc != MOD_ERR_OK)
        {
            Warning(hwndClient, "Error copying to clipboard: %s", ModErrorString(mrc));
        }
        else
        {
            WinOpenClipbrd(hab);
            WinEmptyClipbrd(hab);

            /* Try to also put CF_METAFILE data on clipboard too */
            if ( ModMakeHMF(hbmClipbrd, hab, &hmf) == MOD_ERR_OK )
            {
                WinSetClipbrdData(hab, (ULONG) hmf, CF_METAFILE, CFI_HANDLE);
            }

            WinSetClipbrdData(hab, (ULONG) hbmClipbrd, CF_BITMAP, CFI_HANDLE);
            WinCloseClipbrd(hab);

            GpiDeleteBitmap(hbmClipbrd); hbmClipbrd = (HBITMAP) NULL;
        }
    }
}
/*...e*/
/*...sReflect:0:*/
/*...sReflectAll:0:*/
/* This routine is used to perform primitives affecting the whole bitmap */

static VOID ReflectAll(
    HWND hwndClient,
    MOD_ERR (*reflector)(const MOD *mod, MOD *modNew),
    const CHAR *szWhat, const CHAR *szUndo
    )
{
    MOD_ERR mrc; MOD modNew;
    BITMAP bmpNew;

    Caption(hwndClient, " - %s %s", szWhat, FileName());
    mrc = (*reflector)(&(drawingBitmap.mod), &modNew);
    Caption(hwndClient, " - %s", FileName());

    if ( mrc != MOD_ERR_OK )
    {
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
    {
        ModDelete(&modNew);
        return;
    }
    ModDelete(&modNew);

    KeepForUndo(&drawingBitmap, szUndo);

    fUnsavedChanges = TRUE;
    fSelectionDefined = FALSE;

    SetBitmap(&bmpNew);
    DeleteBitmap(&bmpNew);
}
/*...e*/
/*...sReflectSelection:0:*/
/* This routine is called for primitives that just alter the selection */

static VOID ReflectSelection(
    HWND hwndClient,
    MOD_ERR (*reflector)(const MOD *mod, MOD *modNew),
    const CHAR *szWhat, const CHAR *szUndo
    )
{
    MOD_ERR mrc; MOD modSelBefore, modSelAfter, modNew;
    BITMAP bmpNew;

    Caption(hwndClient, " - %s %s", szWhat, FileName());

    /* Step1: Extract the selection */
    mrc = ModExtractSubrectangle(&(drawingBitmap.mod),
        rclSelection.xLeft,
        rclSelection.yBottom,
        rclSelection.xRight - rclSelection.xLeft,
        rclSelection.yTop - rclSelection.yBottom,
        &modSelBefore);
    if ( mrc != MOD_ERR_OK )
    {
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step2: Try to perform the operation */
    mrc = (*reflector)(&modSelBefore, &modSelAfter);
    ModDelete(&modSelBefore);
    if ( mrc != MOD_ERR_OK )
    {
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step3: Make a copy of the current bitmap to build composite result */
    mrc = ModCopy(&(drawingBitmap.mod), &modNew);
    if ( mrc != MOD_ERR_OK )
    {
        ModDelete(&modSelAfter);
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step4: Blit the new data over the old data */
    ModBlit(&modNew, rclSelection.xLeft, rclSelection.yBottom, /* Dst */
            &modSelAfter, 0, 0,                                /* Src */
            modSelAfter.gbm.w, modSelAfter.gbm.h);

    /* Step5: Discard after bitmap */
    ModDelete(&modSelAfter);

    /* From here onwards: Try to build new view bitmap */

    Caption(hwndClient, " - %s", FileName());

    if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
    {
        ModDelete(&modNew);
        return;
    }
    ModDelete(&modNew);

    KeepForUndo(&drawingBitmap, szUndo);

    fUnsavedChanges = TRUE;

    SetBitmap(&bmpNew);
    DeleteBitmap(&bmpNew);
}
/*...e*/

static VOID Reflect(
    HWND hwndClient,
    MOD_ERR (*reflector)(const MOD *mod, MOD *modNew),
    const CHAR *szWhat, const CHAR *szUndo
    )
{
    if ( fSelectionDefined )
        ReflectSelection(hwndClient, reflector, szWhat, szUndo);
    else
        ReflectAll(hwndClient, reflector, szWhat, szUndo);
}
/*...e*/

/*...sFixRedeyeSelection:0:*/
/* This routine is called for primitives that just alter the selection */

static VOID FixRedeyeSelection(
    HWND hwndClient,
    const CHAR *szWhat, const CHAR *szUndo
    )
{
    MOD_ERR mrc; MOD modSelBefore, modSelAfter, modNew;
    BITMAP bmpNew;

    Caption(hwndClient, " - %s %s", szWhat, FileName());

    /* Step1: Extract the selection */
    mrc = ModExtractSubrectangle(&(drawingBitmap.mod),
                                 rclSelection.xLeft,
                                 rclSelection.yBottom,
                                 rclSelection.xRight - rclSelection.xLeft,
                                 rclSelection.yTop - rclSelection.yBottom,
                                 &modSelBefore);
    if ( mrc != MOD_ERR_OK )
    {
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step2: Try to perform the operation */
    mrc = ModFixRedeye(&modSelBefore, &modSelAfter);
    ModDelete(&modSelBefore);
    if ( mrc != MOD_ERR_OK )
    {
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step3: Make a copy of the current bitmap to build composite result */
    mrc = ModCopy(&(drawingBitmap.mod), &modNew);
    if ( mrc != MOD_ERR_OK )
    {
        ModDelete(&modSelAfter);
        Caption(hwndClient, " - %s", FileName());
        Warning(hwndClient, "Error %s %s: %s", szWhat, FileName(), ModErrorString(mrc));
        return;
    }

    /* Step4: Blit the new data over the old data */
    ModBlit(&modNew, rclSelection.xLeft, rclSelection.yBottom, /* Dst */
            &modSelAfter, 0, 0,                                /* Src */
            modSelAfter.gbm.w, modSelAfter.gbm.h);

    /* Step5: Discard after bitmap */
    ModDelete(&modSelAfter);

    /* From here onwards: Try to build new view bitmap */

    Caption(hwndClient, " - %s", FileName());

    if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
    {
        ModDelete(&modNew);
        return;
    }
    ModDelete(&modNew);

    KeepForUndo(&drawingBitmap, szUndo);

    fUnsavedChanges = TRUE;

    SetBitmap(&bmpNew);
    DeleteBitmap(&bmpNew);
}
/*...e*/

/*...sNewView:0:*/
static VOID NewView(HWND hwndClient, BYTE bViewNew)
{
    BITMAP bmpNew;

    if ( !MakeVisualBg(hwndClient, &(drawingBitmap.mod), bViewNew, &bmpNew, FileName()) )
        return;

    bView = bViewNew;
    SetBitmap(&bmpNew);
    DeleteBitmap(&bmpNew);
}
/*...e*/

/*...sAboutDlgProc:0:*/
MRESULT _System AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( msg )
    {
        /*...sWM_INITDLG:16:*/
        case WM_INITDLG:
        {
            CHAR sz[80+1];
            sprintf(sz, "Version %4.2f, using GBM.DLL version %4.2f",
                (double) VERSION/100.0, (double) gbm_version()/100.0);
            WinSetDlgItemText(hwnd, DID_VERSION_TEXT, sz);
            TidySysMenu(hwnd);
            return (MRESULT) FALSE; /* We have not set the focus */
        }
        /*...e*/
        /*...sWM_COMMAND:16:*/
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                /*...sDID_OK:32:*/
                case DID_OK:
                    WinDismissDlg(hwnd, TRUE);
                    return (MRESULT) 0;
                /*...e*/
                /*...sDID_CANCEL:32:*/
                case DID_CANCEL:
                    WinDismissDlg(hwnd, FALSE);
                    return (MRESULT) 0;
                /*...e*/
            }
            break;
        /*...e*/
        /*...sWM_CLOSE:16:*/
        case WM_CLOSE:
            WinDismissDlg(hwnd, FALSE);
            return (MRESULT) 0;
        /*...e*/
        /*...sWM_HELP:16:*/
        case WM_HELP:
            /* Parent is HWND_DESKTOP */
            /* WinDefDlgProc() will pass this up to the parent */
            /* So redirect to the owner */
            return WinSendMsg(WinQueryWindow(hwnd, QW_OWNER), msg, mp1, mp2);
        /*...e*/
    }
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}
/*...e*/
/*...sColourDlgProc:0:*/
static int map = CVT_I_TO_L;
static double gama = 2.1, shelf = 0.0;

MRESULT _System ColourDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( msg )
    {
        /*...sWM_INITDLG:16:*/
        case WM_INITDLG:
        {
            CHAR sz[50+1] = { 0 };
            SHORT id = DID_I_TO_L;

            switch ( map )
            {
                case CVT_I_TO_L:    id = DID_I_TO_L;    break;
                case CVT_I_TO_P:    id = DID_I_TO_P;    break;
                case CVT_L_TO_I:    id = DID_L_TO_I;    break;
                case CVT_L_TO_P:    id = DID_L_TO_P;    break;
                case CVT_P_TO_I:    id = DID_P_TO_I;    break;
                case CVT_P_TO_L:    id = DID_P_TO_L;    break;
            }

            WinSendDlgItemMsg(hwnd, id, BM_CLICK, MPFROMSHORT(TRUE), NULL);
            WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, id));

            sprintf(sz, "%1.1f", gama);
            WinSetDlgItemText(hwnd, DID_GAMMA, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_GAMMA), NULL, NULL, "0123456789.", NULL);

            sprintf(sz, "%1.1f", shelf);
            WinSetDlgItemText(hwnd, DID_SHELF, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_SHELF), NULL, NULL, "0123456789.", NULL);

            TidySysMenu(hwnd);
            return (MRESULT) TRUE; /* We have set the focus */
        }
        /*...e*/
        /*...sWM_COMMAND:16:*/
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                /*...sDID_OK:32:*/
                case DID_OK:
                {
                    SHORT sInx = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, DID_I_TO_L, BM_QUERYCHECKINDEX, NULL, NULL)) - 1;
                    CHAR sz[50+1] = { 0 };
                    int mapNew = CVT_I_TO_L;
                    double gamaNew, shelfNew;

                    switch ( sInx )
                    {
                        case DID_I_TO_L - DID_I_TO_L:    mapNew = CVT_I_TO_L;    break;
                        case DID_I_TO_P - DID_I_TO_L:    mapNew = CVT_I_TO_P;    break;
                        case DID_L_TO_I - DID_I_TO_L:    mapNew = CVT_L_TO_I;    break;
                        case DID_L_TO_P - DID_I_TO_L:    mapNew = CVT_L_TO_P;    break;
                        case DID_P_TO_I - DID_I_TO_L:    mapNew = CVT_P_TO_I;    break;
                        case DID_P_TO_L - DID_I_TO_L:    mapNew = CVT_P_TO_L;    break;
                    }
                    WinQueryDlgItemText(hwnd, DID_GAMMA, sizeof(sz), sz);
                    sscanf(sz, "%lf", &gamaNew);
                    WinQueryDlgItemText(hwnd, DID_SHELF, sizeof(sz), sz);
                    sscanf(sz, "%lf", &shelfNew);

                    if ( gamaNew < 0.1 || gamaNew > 10.0 )
                        Warning(hwnd, "Gamma must be between 0.1 and 10.0");
                    else if ( shelfNew < 0.0 || shelfNew > 1.0 )
                        Warning(hwnd, "Shelf must be between 0.0 and 1.0");
                    else
                    {
                        map   = mapNew;
                        gama  = gamaNew;
                        shelf = shelfNew;
                        WinDismissDlg(hwnd, TRUE);
                        return (MRESULT) 0;
                    }
                }
                    break;
                /*...e*/
                /*...sDID_CANCEL:32:*/
                case DID_CANCEL:
                    WinDismissDlg(hwnd, FALSE);
                    return (MRESULT) 0;
                /*...e*/
            }
            break;
        /*...e*/
        /*...sWM_CONTROL:16:*/
        case WM_CONTROL:
        {
            SHORT id = SHORT1FROMMP(mp1);
            SHORT note = SHORT2FROMMP(mp1);
            HWND hwndG = WinWindowFromID(hwnd, DID_GAMMA_TEXT);
            HWND hwndS = WinWindowFromID(hwnd, DID_SHELF_TEXT);

            switch ( id )
            {
                case DID_I_TO_L:
                case DID_L_TO_I:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndG, FALSE);
                        WinEnableWindow(hwndS, FALSE);
                    }
                    break;
                case DID_I_TO_P:
                case DID_P_TO_I:
                case DID_P_TO_L:
                case DID_L_TO_P:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndG, TRUE);
                        WinEnableWindow(hwndS, TRUE);
                    }
                    break;
            }
        }
            break;
        /*...e*/
        /*...sWM_CLOSE:16:*/
        case WM_CLOSE:
            WinDismissDlg(hwnd, FALSE);
            return (MRESULT) 0;
        /*...e*/
        /*...sWM_HELP:16:*/
        case WM_HELP:
            /* Parent is HWND_DESKTOP */
            /* WinDefDlgProc() will pass this up to the parent */
            /* So redirect to the owner */
            return WinSendMsg(WinQueryWindow(hwnd, QW_OWNER), msg, mp1, mp2);
        /*...e*/
    }
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}
/*...e*/
/*...sMapDlgProc:0:*/
static int iKeepRed = 8, iKeepGreen = 8, iKeepBlue = 8, nCols = 256;
static int iPal = CVT_784, iAlg = CVT_ERRDIFF;

MRESULT _System MapDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( msg )
    {
        /*...sWM_INITDLG:16:*/
        case WM_INITDLG:
        {
            CHAR sz[50+1] = { 0 };
            SHORT id = DID_MAP_RGB;

            switch ( iPal )
            {
                case CVT_BW:       id = DID_MAP_BW;     break;
                case CVT_VGA:      id = DID_MAP_VGA;    break;
                case CVT_8:        id = DID_MAP_8;      break;
                case CVT_4G:       id = DID_MAP_4G;     break;
                case CVT_784:      id = DID_MAP_784;    break;
                case CVT_666:      id = DID_MAP_666;    break;
                case CVT_444:      id = DID_MAP_444;    break;
                case CVT_8G:       id = DID_MAP_8G;     break;
                case CVT_TRIPEL:   id = DID_MAP_TRIPEL; break;
                case CVT_RGB:      id = DID_MAP_RGB;    break;
                case CVT_FREQ:     id = DID_MAP_FREQ;   break;
                case CVT_MCUT:     id = DID_MAP_MCUT;   break;
            }

            WinSendDlgItemMsg(hwnd, id, BM_CLICK, MPFROMSHORT(TRUE), NULL);
            WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, id));

            switch ( iAlg )
            {
                case CVT_NEAREST:    id = DID_NEAREST;    break;
                case CVT_HALFTONE:    id = DID_HALFTONE;    break;
                case CVT_ERRDIFF:    id = DID_ERRDIFF;    break;
            }

            WinSendDlgItemMsg(hwnd, id, BM_CLICK, MPFROMSHORT(TRUE), NULL);

            sprintf(sz, "%d", iKeepRed);
            WinSetDlgItemText(hwnd, DID_R, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_R), NULL, NULL, "0123456789", NULL);

            sprintf(sz, "%d", iKeepGreen);
            WinSetDlgItemText(hwnd, DID_G, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_G), NULL, NULL, "0123456789", NULL);

            sprintf(sz, "%d", iKeepBlue);
            WinSetDlgItemText(hwnd, DID_B, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_B), NULL, NULL, "0123456789", NULL);

            sprintf(sz, "%d", nCols);
            WinSetDlgItemText(hwnd, DID_N, sz);
            RestrictEntryfield(WinWindowFromID(hwnd, DID_N), NULL, NULL, "0123456789", NULL);

            TidySysMenu(hwnd);
            return (MRESULT) TRUE; /* We have set the focus */
        }
        /*...e*/
        /*...sWM_CONTROL:16:*/
        case WM_CONTROL:
        {
            SHORT id = SHORT1FROMMP(mp1);
            SHORT note = SHORT2FROMMP(mp1);
            HWND hwndR = WinWindowFromID(hwnd, DID_R_TEXT);
            HWND hwndG = WinWindowFromID(hwnd, DID_G_TEXT);
            HWND hwndB = WinWindowFromID(hwnd, DID_B_TEXT);
            HWND hwndN = WinWindowFromID(hwnd, DID_N_TEXT);
            HWND hwndH = WinWindowFromID(hwnd, DID_HALFTONE);
            HWND hwndE = WinWindowFromID(hwnd, DID_ERRDIFF);

            switch ( id )
            {
                case DID_MAP_BW:
                case DID_MAP_8:
                case DID_MAP_VGA:
                case DID_MAP_4G:
                case DID_MAP_784:
                case DID_MAP_666:
                case DID_MAP_444:
                case DID_MAP_8G:
                case DID_MAP_TRIPEL:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndR, FALSE);
                        WinEnableWindow(hwndG, FALSE);
                        WinEnableWindow(hwndB, FALSE);
                        WinEnableWindow(hwndN, FALSE);
                    }
                    break;
                case DID_MAP_RGB:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndR, TRUE);
                        WinEnableWindow(hwndG, TRUE);
                        WinEnableWindow(hwndB, TRUE);
                        WinEnableWindow(hwndN, FALSE);
                    }
                    break;
                case DID_MAP_FREQ:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndR, TRUE);
                        WinEnableWindow(hwndG, TRUE);
                        WinEnableWindow(hwndB, TRUE);
                        WinEnableWindow(hwndN, TRUE);
                    }
                    break;
                case DID_MAP_MCUT:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndR, FALSE);
                        WinEnableWindow(hwndG, FALSE);
                        WinEnableWindow(hwndB, FALSE);
                        WinEnableWindow(hwndN, TRUE);
                    }
                    break;
            }

            switch ( id )
            {
                case DID_MAP_8G:
                case DID_MAP_TRIPEL:
                case DID_MAP_FREQ:
                case DID_MAP_MCUT:
                    if ( note == BN_CLICKED )
                    {
                        WinSendDlgItemMsg(hwnd, DID_NEAREST, BM_CLICK, MPFROMSHORT(TRUE), NULL);
                        WinEnableWindow(hwndH, FALSE);
                        WinEnableWindow(hwndE, FALSE);
                    }
                    break;
                case DID_MAP_BW:
                case DID_MAP_4G:
                    if ( note == BN_CLICKED )
                    {
                        WinSendDlgItemMsg(hwnd, DID_NEAREST, BM_CLICK, MPFROMSHORT(TRUE), NULL);
                        WinEnableWindow(hwndH, FALSE);
                        WinEnableWindow(hwndE, TRUE);
                    }
                    break;
                case DID_MAP_8:
                case DID_MAP_VGA:
                case DID_MAP_784:
                case DID_MAP_666:
                case DID_MAP_444:
                case DID_MAP_RGB:
                    if ( note == BN_CLICKED )
                    {
                        WinEnableWindow(hwndH, TRUE);
                        WinEnableWindow(hwndE, TRUE);
                    }
                    break;
            }
        }
            break;
        /*...e*/
        /*...sWM_COMMAND:16:*/
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                /*...sDID_OK:32:*/
                case DID_OK:
                {
                    SHORT sInx;
                    CHAR sz[50+1];
                    int iPalNew = CVT_RGB;
                    int iAlgNew = CVT_NEAREST;
                    int iKeepRedNew, iKeepGreenNew, iKeepBlueNew, nColsNew;

                    sInx = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, DID_MAP_BW, BM_QUERYCHECKINDEX, NULL, NULL)) - 1;
                    switch ( sInx )
                    {
                        case DID_MAP_BW     - DID_MAP_BW:    iPalNew = CVT_BW;    break;
                        case DID_MAP_VGA    - DID_MAP_BW:    iPalNew = CVT_VGA;    break;
                        case DID_MAP_8      - DID_MAP_BW:    iPalNew = CVT_8;    break;
                        case DID_MAP_4G     - DID_MAP_BW:    iPalNew = CVT_4G;    break;
                        case DID_MAP_784    - DID_MAP_BW:    iPalNew = CVT_784;    break;
                        case DID_MAP_666    - DID_MAP_BW:    iPalNew = CVT_666;    break;
                        case DID_MAP_444    - DID_MAP_BW:    iPalNew = CVT_444;    break;
                        case DID_MAP_8G     - DID_MAP_BW:    iPalNew = CVT_8G;    break;
                        case DID_MAP_TRIPEL - DID_MAP_BW:    iPalNew = CVT_TRIPEL;    break;
                        case DID_MAP_RGB    - DID_MAP_BW:    iPalNew = CVT_RGB;    break;
                        case DID_MAP_FREQ   - DID_MAP_BW:    iPalNew = CVT_FREQ;    break;
                        case DID_MAP_MCUT   - DID_MAP_BW:    iPalNew = CVT_MCUT;    break;
                    }

                    WinEnableWindow(WinWindowFromID(hwnd, DID_NEAREST ), TRUE);
                    WinEnableWindow(WinWindowFromID(hwnd, DID_HALFTONE), TRUE);
                    WinEnableWindow(WinWindowFromID(hwnd, DID_ERRDIFF ), TRUE);

                    sInx = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, DID_NEAREST, BM_QUERYCHECKINDEX, NULL, NULL)) - 1;
                    switch ( sInx )
                    {
                        case DID_NEAREST  - DID_NEAREST:    iAlgNew = CVT_NEAREST;    break;
                        case DID_HALFTONE - DID_NEAREST:    iAlgNew = CVT_HALFTONE;    break;
                        case DID_ERRDIFF  - DID_NEAREST:    iAlgNew = CVT_ERRDIFF;    break;
                    }

                    WinQueryDlgItemText(hwnd, DID_R, sizeof(sz), sz);
                    sscanf(sz, "%d", &iKeepRedNew);
                    WinQueryDlgItemText(hwnd, DID_G, sizeof(sz), sz);
                    sscanf(sz, "%d", &iKeepGreenNew);
                    WinQueryDlgItemText(hwnd, DID_B, sizeof(sz), sz);
                    sscanf(sz, "%d", &iKeepBlueNew);
                    WinQueryDlgItemText(hwnd, DID_N, sizeof(sz), sz);
                    sscanf(sz, "%d", &nColsNew);

                    if ( iKeepRedNew   < 0 || iKeepRedNew   > 8 )
                        Warning(hwnd, "No. bits R must be between 0 and 8");
                    else if ( iKeepGreenNew < 0 || iKeepGreenNew > 8 )
                        Warning(hwnd, "No. bits G must be between 0 and 8");
                    else if ( iKeepBlueNew  < 0 || iKeepBlueNew  > 8 )
                        Warning(hwnd, "No. bits B must be between 0 and 8");
                    else if ( nColsNew      < 1 || nColsNew      > 256 )
                        Warning(hwnd, "No. bits B must be between 1 and 256");
                    else
                    {
                        iPal       = iPalNew;
                        iAlg       = iAlgNew;
                        iKeepRed   = iKeepRedNew;
                        iKeepGreen = iKeepGreenNew;
                        iKeepBlue  = iKeepBlueNew;
                        nCols      = nColsNew;
                        WinDismissDlg(hwnd, TRUE);
                        return (MRESULT) 0;
                    }
                }
                    return (MRESULT) 0;
                /*...e*/
                /*...sDID_CANCEL:32:*/
                case DID_CANCEL:
                    WinDismissDlg(hwnd, FALSE);
                    return (MRESULT) 0;
                /*...e*/
            }
            break;
        /*...e*/
        /*...sWM_CLOSE:16:*/
        case WM_CLOSE:
            WinDismissDlg(hwnd, FALSE);
            return (MRESULT) 0;
        /*...e*/
        /*...sWM_HELP:16:*/
        case WM_HELP:
            /* Parent is HWND_DESKTOP */
            /* WinDefDlgProc() will pass this up to the parent */
            /* So redirect to the owner */
            return WinSendMsg(WinQueryWindow(hwnd, QW_OWNER), msg, mp1, mp2);
        /*...e*/
    }
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}
/*...e*/

/* ----------------------------------------------------------- */

/*...sResizeDlgProc:0:*/
static PSZ resizeLboxItems[] = { "Nearest Neighbor",
                                 "Bilinear",
                                 "Bell",
                                 "BSpline",
                                 "Quadratic",
                                 "Gaussian",
                                 "Mitchell",
                                 "Catmull/Rom",
                                 "Lanczos",
                                 "Kaiser",
                                 "Blackman" };
static GBM_SCALE_FILTER resizeLboxItemsFilterId[] =
{
  GBM_SCALE_FILTER_NEARESTNEIGHBOR,
  GBM_SCALE_FILTER_BILINEAR,
  GBM_SCALE_FILTER_BELL,
  GBM_SCALE_FILTER_BSPLINE,
  GBM_SCALE_FILTER_QUADRATIC,
  GBM_SCALE_FILTER_GAUSSIAN,
  GBM_SCALE_FILTER_MITCHELL,
  GBM_SCALE_FILTER_CATMULLROM,
  GBM_SCALE_FILTER_LANCZOS,
  GBM_SCALE_FILTER_KAISER,
  GBM_SCALE_FILTER_BLACKMAN
};
static const int resizeLboxItemsLength = sizeof(resizeLboxItems)/sizeof(resizeLboxItems[0]);

static int cxNew = -1, cyNew = -1;
static BOOL resizingQualityEnabled = FALSE;
static LONG resizeFilterIndex = LIT_NONE;

/* reset resize dialog remembered image size */
static void resetResizeDialogSettings(void)
{
    cxNew = -1; cyNew = -1;
}


MRESULT _System ResizeDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch ( msg )
  {
    case WM_INITDLG:
    {
      CHAR sz[100+1];
      HWND hwndLbox;
      int  i;

      sprintf(sz, "Bitmap is currently %dx%d at %d bpp",
              drawingBitmap.mod.gbm.w, drawingBitmap.mod.gbm.h, drawingBitmap.mod.gbm.bpp);
      WinSetDlgItemText(hwnd, DID_CURRENTLY, sz);

      WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, DID_WIDTH));

      if ( cxNew == -1 ) cxNew = drawingBitmap.mod.gbm.w;
      sprintf(sz, "%d", cxNew);
      WinSetDlgItemText(hwnd, DID_WIDTH, sz);
      RestrictEntryfield(WinWindowFromID(hwnd, DID_WIDTH ), NULL, NULL, "0123456789", NULL);

      if ( cyNew == -1 ) cyNew = drawingBitmap.mod.gbm.h;
      sprintf(sz, "%d", cyNew);
      WinSetDlgItemText(hwnd, DID_HEIGHT, sz);
      RestrictEntryfield(WinWindowFromID(hwnd, DID_HEIGHT), NULL, NULL, "0123456789", NULL);

      WinSetDlgItemText(hwnd, DID_MUL, "2");
      RestrictEntryfield(WinWindowFromID(hwnd, DID_MUL), NULL, NULL, "0123456789.", NULL);

      /* Initialize List Box Control */
      if ((drawingBitmap.mod.gbm.bpp > 8) || isGrayscalePalette(&(drawingBitmap.mod)))
      {
        WinCheckButton  (hwnd, DID_RESIZE_CHECK_INTERPOLATE, resizingQualityEnabled ? 1 : 0);
        WinEnableControl(hwnd, DID_RESIZE_CHECK_INTERPOLATE, TRUE);
      }
      else
      {
        WinCheckButton  (hwnd, DID_RESIZE_CHECK_INTERPOLATE, 0);
        WinEnableControl(hwnd, DID_RESIZE_CHECK_INTERPOLATE, FALSE);
        resizingQualityEnabled = FALSE;
      }
      hwndLbox = WinWindowFromID(hwnd, DID_RESIZE_FILTERLIST);
      WinEnableWindow(hwndLbox, resizingQualityEnabled);
      for (i = 0; i < resizeLboxItemsLength; i++)
      {
        WinInsertLboxItem(hwndLbox, LIT_END, resizeLboxItems[i]);
      }
      WinSendMsg(hwndLbox, LM_SELECTITEM,
                 MPFROMSHORT(resizeFilterIndex),
                 MPFROMSHORT(TRUE));

      TidySysMenu(hwnd);
      return (MRESULT) TRUE; /* We have set the focus */
    }

    /* ----------------------------------- */

    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
        case DID_WIDTH_MUL:
        {
          CHAR sz[40+1]; double w, n;
          WinQueryDlgItemText(hwnd, DID_WIDTH, sizeof(sz), sz);
          sscanf(sz, "%lf", &w);
          WinQueryDlgItemText(hwnd, DID_MUL, sizeof(sz), sz);
          sscanf(sz, "%lf", &n);
          w *= n;
          if ( w > 15000.0 )
          {
            Warning(hwnd, "Width of %6.2lf is too big", w);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) w);
          WinSetDlgItemText(hwnd, DID_WIDTH, sz);
          return (MRESULT) 0;
        }

        case DID_WIDTH_DIV:
        {
          CHAR sz[40+1]; double w, n;
          WinQueryDlgItemText(hwnd, DID_WIDTH, sizeof(sz), sz);
          sscanf(sz, "%lf", &w);
          WinQueryDlgItemText(hwnd, DID_MUL, sizeof(sz), sz);
          sscanf(sz, "%lf", &n);
          if ( n < 1e-10 )
          {
            Warning(hwnd, "N is too small");
            return (MRESULT) 0;
          }
          w /= n;
          if ( w > 15000.0 )
          {
            Warning(hwnd, "Width of %6.2lf is too big", w);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) w);
          WinSetDlgItemText(hwnd, DID_WIDTH, sz);
          return (MRESULT) 0;
        }

        case DID_WIDTH_MATCH:
        {
          CHAR sz[40+1]; double w, h;
          WinQueryDlgItemText(hwnd, DID_HEIGHT, sizeof(sz), sz);
          sscanf(sz, "%lf", &h);
          w = ( h/drawingBitmap.mod.gbm.h ) * drawingBitmap.mod.gbm.w;
          if ( w > 15000.0 )
          {
            Warning(hwnd, "Width of %6.2lf is too big", w);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) w);
          WinSetDlgItemText(hwnd, DID_WIDTH, sz);
          return (MRESULT) 0;
        }

        case DID_HEIGHT_MUL:
        {
          CHAR sz[40+1]; double h, n;
          WinQueryDlgItemText(hwnd, DID_HEIGHT, sizeof(sz), sz);
          sscanf(sz, "%lf", &h);
          WinQueryDlgItemText(hwnd, DID_MUL, sizeof(sz), sz);
          sscanf(sz, "%lf", &n);
          h *= n;
          if ( h > 15000.0 )
          {
            Warning(hwnd, "Height of %6.2lf is too big", h);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) h);
          WinSetDlgItemText(hwnd, DID_HEIGHT, sz);
          return (MRESULT) 0;
        }

        case DID_HEIGHT_DIV:
        {
          CHAR sz[40+1]; double h, n;
          WinQueryDlgItemText(hwnd, DID_HEIGHT, sizeof(sz), sz);
          sscanf(sz, "%lf", &h);
          WinQueryDlgItemText(hwnd, DID_MUL, sizeof(sz), sz);
          sscanf(sz, "%lf", &n);
          if ( n < 1e-10 )
          {
            Warning(hwnd, "N is too small");
            return (MRESULT) 0;
          }
          h /= n;
          if ( h > 15000.0 )
          {
            Warning(hwnd, "Height of %6.2lf is too big", h);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) h);
          WinSetDlgItemText(hwnd, DID_HEIGHT, sz);
          return (MRESULT) 0;
        }

        case DID_HEIGHT_MATCH:
        {
          CHAR sz[40+1]; double w, h;
          WinQueryDlgItemText(hwnd, DID_WIDTH, sizeof(sz), sz);
          sscanf(sz, "%lf", &w);
          h = ( w/drawingBitmap.mod.gbm.w ) * drawingBitmap.mod.gbm.h;
          if ( h > 15000.0 )
          {
            Warning(hwnd, "Height of %6.2lf is too big", h);
            return (MRESULT) 0;
          }
          sprintf(sz, "%d", (int) h);
          WinSetDlgItemText(hwnd, DID_HEIGHT, sz);
          return (MRESULT) 0;
        }

        case DID_OK:
        {
          CHAR sz[50+1];

          WinQueryDlgItemText(hwnd, DID_WIDTH, sizeof(sz), sz);
          sscanf(sz, "%d", &cxNew);
          WinQueryDlgItemText(hwnd, DID_HEIGHT, sizeof(sz), sz);
          sscanf(sz, "%d", &cyNew);

          resizeFilterIndex = WinQueryLboxSelectedItem(WinWindowFromID(hwnd,
                                                       DID_RESIZE_FILTERLIST));
          if (resizingQualityEnabled && (resizeFilterIndex == LIT_NONE))
          {
            Warning(hwnd, "No interpolation filter selected.");
            return (MRESULT) 0;
          }
          if ( cxNew < 1 || cyNew < 1 )
          {
              Warning(hwnd, "Desired size is too small.");
          }
          else if ( cxNew > 15000 || cyNew > 15000 )
          {
              Warning(hwnd, "Desired size is too big.");
          }
          else
          {
              WinDismissDlg(hwnd, TRUE);
          }
          return (MRESULT) 0;
        }

        case DID_CANCEL:
          WinDismissDlg(hwnd, FALSE);
          return (MRESULT) 0;
      }
      break;

    /* ----------------------------------- */

    case WM_CONTROL:
      /* switch on control code */
      switch(SHORT2FROMMP(mp1))
      {
        case BN_CLICKED:
        {
          const USHORT usCheckId = SHORT1FROMMP(mp1);
          if (usCheckId == DID_RESIZE_CHECK_INTERPOLATE)
          {
            resizingQualityEnabled = (WinQueryButtonCheckstate(hwnd, DID_RESIZE_CHECK_INTERPOLATE) == 0)
                                     ? FALSE : TRUE;
            WinEnableControl(hwnd, DID_RESIZE_FILTERLIST, resizingQualityEnabled);
          }
        }
      }
      break;

    /* ----------------------------------- */

    case WM_CLOSE:
      WinDismissDlg(hwnd, FALSE);
      return (MRESULT) 0;

    /* ----------------------------------- */

    case WM_HELP:
      /* Parent is HWND_DESKTOP */
      /* WinDefDlgProc() will pass this up to the parent */
      /* So redirect to the owner */
      return WinSendMsg(WinQueryWindow(hwnd, QW_OWNER), msg, mp1, mp2);

  }
  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

/* ----------------------------------------------------------- */

static VOID MergeFilenameAndPath(char * basePathname, char * fullFileName)
{
    char fullDllName[_MAX_PATH+1] = { 0 };
    char drive[_MAX_DRIVE+1]      = { 0 };
    char dir  [_MAX_DIR+1]        = { 0 };
    char fname[_MAX_FNAME+1]      = { 0 };
    char ext  [_MAX_EXT]          = { 0 };

    strcpy(fullDllName, fullFileName);

    /* extract the file extension */
    _splitpath(fullDllName , NULL , NULL, fname, ext);
    _splitpath(basePathname, drive,  dir, NULL , NULL);

    _makepath(basePathname, drive, dir, fname, ext);
}

/*...sRemoveExtension:0:*/
static VOID RemoveExtension(CHAR *sz)
{
    CHAR *szDot;

    if ( (szDot = strrchr(sz, '.')) == NULL )
        return;

    if ( strchr(szDot+1, '\\') != NULL )
        return;

    *szDot = '\0';
}
/*...e*/

/* ----------------------------------------------------------- */

MRESULT _System ObjectWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( (int) msg )
    {
        /*...sUM_NEW           \45\ clear out bitmap:16:*/
        case UM_NEW:
        {
            HWND hwndClient = HWNDFROMMP(mp1);

            DiscardUndo();

            if ( fGotBitmap )
                if ( SaveChanges(hwndClient) )
                {
                    SetNoBitmap();
                    fGotBitmap        = FALSE;
                    fUnsavedChanges   = FALSE;
                    fSelectionDefined = FALSE;
                    strcpy(szFileName, "");

                    Caption(hwndClient, "");
                }

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_OPEN          \45\ read in a new bitmap:16:*/
        case UM_OPEN:
        {
            HAB hab = WinQueryAnchorBlock(hwnd);
            HWND hwndClient = HWNDFROMMP(mp1);
            GBMFILEDLG gbmfild;
            MOD_ERR mrc; MOD modNew;
            char extFmt[_MAX_EXT+1] = { 0 };
            BITMAP bmpNew;

            if ( !SaveChanges(hwndClient) )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            memset(&(gbmfild.fild), 0, sizeof(FILEDLG));
            gbmfild.fild.cbSize = sizeof(FILEDLG);
            gbmfild.fild.fl = (FDS_CENTER|FDS_OPEN_DIALOG|FDS_HELPBUTTON);

            if (ProfileGetOpenFileDialogPath(hab, gbmfild.fild.szFullFile,
                                             sizeof(gbmfild.fild.szFullFile),
                                             extFmt,
                                             sizeof(extFmt)))
            {
                if (HasNonEmptyFileName())
                {
                    MergeFilenameAndPath(gbmfild.fild.szFullFile, szFileName);
                    if (strlen(extFmt) > 0)
                    {
                        RemoveExtension(gbmfild.fild.szFullFile);
                        strcat(gbmfild.fild.szFullFile, extFmt);
                    }
                }
            }
            else
            {
                if (GetCurrentDir(gbmfild.fild.szFullFile,
                                  sizeof(gbmfild.fild.szFullFile)))
                {
                    if (HasNonEmptyFileName())
                    {
                        MergeFilenameAndPath(gbmfild.fild.szFullFile, szFileName);
                    }
                }
                else
                {
                    strcpy(gbmfild.fild.szFullFile, "");
                }
            }
            strcpy(gbmfild.szOptions, "");

            GbmFileDlg(HWND_DESKTOP, hwndClient, &gbmfild);
            if ( gbmfild.fild.lReturn != DID_OK )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            ProfileSetOpenFileDialogPathFromFullFile(hab, gbmfild.fild.szFullFile);

            SetNoBitmap();

            LowPri();
            Caption(hwndClient, " - reading %s", gbmfild.fild.szFullFile);
            mrc = ModCreateFromFile(gbmfild.fild.szFullFile, gbmfild.szOptions, &modNew);
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error loading %s: %s", gbmfild.fild.szFullFile, ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, gbmfild.fild.szFullFile) )
            {
                ModDelete(&modNew);
                Caption(hwndClient, "");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            DiscardUndo();
            fGotBitmap = TRUE;
            fUnsavedChanges = FALSE;
            fSelectionDefined = FALSE;
            strcpy(szFileName, gbmfild.fild.szFullFile);
            Caption(hwndClient, " - %s", FileName());

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            /* reset resize dialog remembered image size */
            resetResizeDialogSettings();

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_SAVE          \45\ save with old name:16:*/
        case UM_SAVE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc;

            LowPri();
            Caption(hwndClient, " - saving %s", FileName());
            mrc = ModWriteToFile(&(drawingBitmap.mod), szFileName, "");
            Caption(hwndClient, "- %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error saving %s: %s", FileName(), ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            fUnsavedChanges = FALSE;

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_SAVE_AS       \45\ save with new name:16:*/
        case UM_SAVE_AS:
        {
            HAB hab = WinQueryAnchorBlock(hwnd);
            HWND hwndClient = HWNDFROMMP(mp1);
            GBMFILEDLG gbmfild;
            char extFmt[_MAX_EXT+1] = { 0 };
            MOD_ERR mrc;

            memset(&gbmfild.fild, 0, sizeof(FILEDLG));
            gbmfild.fild.cbSize = sizeof(FILEDLG);
            gbmfild.fild.fl = (FDS_CENTER|FDS_SAVEAS_DIALOG|FDS_ENABLEFILELB|FDS_HELPBUTTON);

            if (ProfileGetSaveFileDialogPath(hab, gbmfild.fild.szFullFile,
                                             sizeof(gbmfild.fild.szFullFile),
                                             extFmt,
                                             sizeof(extFmt)))
            {
                if (HasNonEmptyFileName())
                {
                    MergeFilenameAndPath(gbmfild.fild.szFullFile, szFileName);
                    if (strlen(extFmt) > 0)
                    {
                        RemoveExtension(gbmfild.fild.szFullFile);
                        strcat(gbmfild.fild.szFullFile, extFmt);
                    }
                }
            }
            else
            {
                if (GetCurrentDir(gbmfild.fild.szFullFile,
                                  sizeof(gbmfild.fild.szFullFile)))
                {
                    if (HasNonEmptyFileName())
                    {
                        MergeFilenameAndPath(gbmfild.fild.szFullFile, szFileName);
                    }
                }
                else
                {
                    strcpy(gbmfild.fild.szFullFile, "");
                }
            }
            strcpy(gbmfild.szOptions, "");

            GbmFileDlg(HWND_DESKTOP, hwndClient, &gbmfild);
            if ( gbmfild.fild.lReturn != DID_OK )
            {
              WinSendMsg(hwnd, UM_DONE, NULL, NULL);
              return (MRESULT) 0;
            }
            if (! CheckFileExistsAndQueryOverride(hwnd, gbmfild.fild.szFullFile))
            {
              WinSendMsg(hwnd, UM_DONE, NULL, NULL);
              return (MRESULT) 0;
            }

            ProfileSetSaveFileDialogPathFromFullFile(hab, gbmfild.fild.szFullFile);

            LowPri();
            Caption(hwndClient, " - saving as %s", gbmfild.fild.szFullFile);
            mrc = ModWriteToFile(&(drawingBitmap.mod), gbmfild.fild.szFullFile, gbmfild.szOptions);
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error saving as %s: %s", gbmfild.fild.szFullFile, ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            strcpy(szFileName, gbmfild.fild.szFullFile);
            Caption(hwndClient, " - %s", FileName());
            fUnsavedChanges = FALSE;

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_EXPORT_MET    \45\ export to Metafile:16:*/
        case UM_EXPORT_MET:
        {
            HAB hab = WinQueryAnchorBlock(hwnd);
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc;
            BOOL fOk;
            HMF hmf;
            HBITMAP hbm;
            FILEDLG fild;

            memset(&fild, 0, sizeof(FILEDLG));
            fild.cbSize = sizeof(FILEDLG);
            fild.fl = (FDS_CENTER|FDS_SAVEAS_DIALOG);
            fild.pszTitle = "Export in MetaFile";

            if (ProfileGetSaveFileDialogPath(hab, fild.szFullFile,
                                             sizeof(fild.szFullFile),
                                             NULL, 0))
            {
                if (HasNonEmptyFileName())
                {
                    MergeFilenameAndPath(fild.szFullFile, szFileName);
                    RemoveExtension(fild.szFullFile);
                    strcat(fild.szFullFile, ".MET");
                }
                else
                {
                    strcat(fild.szFullFile, "EXPORT.MET");
                }
            }
            else
            {
                if (GetCurrentDir(fild.szFullFile,
                                  sizeof(fild.szFullFile)))
                {
                    if (HasNonEmptyFileName())
                    {
                        MergeFilenameAndPath(fild.szFullFile, szFileName);
                        RemoveExtension(fild.szFullFile);
                        strcat(fild.szFullFile, ".MET");
                    }
                    else
                    {
                        strcat(fild.szFullFile, "EXPORT.MET");
                    }
                }
                else
                {
                    strcpy(fild.szFullFile, "EXPORT.MET");
                }
            }

            WinFileDlg(HWND_DESKTOP, hwndClient, &fild);
            if ( fild.lReturn != DID_OK )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            ProfileSetSaveFileDialogPathFromFullFile(hab, fild.szFullFile);

            LowPri();
            Caption(hwndClient, " - making metafile");

            mrc = ModMakeHBITMAP(&(drawingBitmap.mod), hab, &hbm);
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error making metafile: %s", ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            mrc = ModMakeHMF(hbm, hab, &hmf);
            GpiDeleteBitmap(hbm);

            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error making metafile: %s", ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            LowPri();
            Caption(hwndClient, " - exporting to %s", fild.szFullFile);
            remove(fild.szFullFile);
            fOk = GpiSaveMetaFile(hmf, fild.szFullFile);
            Caption(hwndClient, " - %s", FileName());
            RegPri();

            GpiDeleteMetaFile(hmf);

            if ( !fOk )
            {
                Warning(hwndClient, "Error exporting to %s", fild.szFullFile);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_PRINT         \45\ print image:16:*/
        case UM_PRINT:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            HAB hab = WinQueryAnchorBlock(hwndClient);
            HBITMAP hbmCopy;
            MOD_ERR mrc;
            BMP_ERR brc;
            CHAR sz[255+1];
            const RGB2 * pRGBA = drawingBitmap.argb2Color;
            const LONG colorFg = (pRGBA[0].bRed << 16) + (pRGBA[0].bGreen << 8) + pRGBA[0].bBlue;
            const LONG colorBg = (pRGBA[1].bRed << 16) + (pRGBA[1].bGreen << 8) + pRGBA[1].bBlue;

            Caption(hwndClient, " - printing %s", FileName());
            LowPri();

            mrc = ModMakeHBITMAP(&(drawingBitmap.mod), hab, &hbmCopy);
            if ( mrc != MOD_ERR_OK )
            {
                RegPri();
                Caption(hwndClient, " - %s", FileName());
                Warning(hwndClient, "Error preparing to print %s: %s", FileName(), ModErrorString(mrc));
            }
            else if ( (brc = BmpPrint(hab, hbmCopy, colorFg, colorBg, FileName(), szAppName, NULL)) != BMP_ERR_OK )
            {
                RegPri();
                Caption(hwndClient, " - %s", FileName());
                GpiDeleteBitmap(hbmCopy);
                Warning(hwndClient, "Error printing: %s", BmpErrorMessage(brc, sz));
            }
            else
            {
                RegPri();
                Caption(hwndClient, " - %s", FileName());
                GpiDeleteBitmap(hbmCopy);
            }

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }

        /*...e*/
        /*...sUM_SNAPSHOT      \45\ take snapshot of screen:16:*/
        case UM_SNAPSHOT:
        {
            HAB hab = WinQueryAnchorBlock(hwnd);
            HWND hwndClient = HWNDFROMMP(mp1);
            int cxScreen = (int) WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
            int cyScreen = (int) WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
            MOD_ERR mrc; MOD modNew;
            BITMAP bmpNew;
            HDC hdcMem; SIZEL sizl; HPS hpsMem; HBITMAP hbmMem;

            #pragma pack(2)
            struct
            {
                BITMAPINFOHEADER2 bmp2;
                RGB2 argb2Color[0x100];
            } bm;
            #pragma pack()

            if ( (hdcMem = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
            {
                Warning(hwndClient, "Error snapshotting screen: can't create PM HDC resource");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            sizl.cx = cxScreen;
            sizl.cy = cyScreen;
            if ( (hpsMem = GpiCreatePS(hab, hdcMem, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
            {
                DevCloseDC(hdcMem);
                Warning(hwndClient, "Error snapshotting screen: can't create PM HDC resource");
                return (MRESULT) 0;
            }

            memset(&(bm.bmp2), 0, sizeof(BITMAPINFOHEADER2));
            bm.bmp2.cbFix     = 16;
            bm.bmp2.cx        = cxScreen;
            bm.bmp2.cy        = cyScreen;
            bm.bmp2.cBitCount = lBitCountScreen;
            bm.bmp2.cPlanes   = 1;
            if ( (hbmMem = GpiCreateBitmap(hpsMem, &(bm.bmp2), 0L, NULL, (BITMAPINFO2 *) &bm)) == (HBITMAP) NULL )
            {
                GpiDestroyPS(hpsMem);
                DevCloseDC(hdcMem);
                Warning(hwndClient, "Error snapshotting screen: can't create PM HDC resource");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            GpiSetBitmap(hpsMem, hbmMem);

            LowPri();
            Caption(hwndClient, " - snapshotting (untitled)");
            /*...sdo the actual copy from the screen to hpsMem:24:*/
            {
                HWND hwndFrame = WinQueryWindow(hwndClient, QW_PARENT);
                POINTL aptl[3];
                HPS hps;

                WinShowWindow(hwndFrame, FALSE);
                DosSleep(2000L); /* Give other windows a chance to repaint */
                hps = WinGetScreenPS(HWND_DESKTOP);
                aptl[0].x = 0;
                aptl[0].y = 0;
                aptl[1].x = cxScreen;
                aptl[1].y = cyScreen;
                aptl[2].x = 0;
                aptl[2].y = 0;
                GpiBitBlt(hpsMem, hps, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
                WinReleasePS(hps);
                WinShowWindow(hwndFrame, TRUE);
            }
            /*...e*/
            mrc = ModCreateFromHPS(hpsMem ,cxScreen, cyScreen,
                ( lBitCountScreen > 8 ) ? 24 : lBitCountScreen,
                &modNew);
            Caption(hwndClient, " - %s", FileName());
            RegPri();

            GpiSetBitmap(hpsMem, (HBITMAP) NULL);
            GpiDeleteBitmap(hbmMem);
            GpiDestroyPS(hpsMem);
            DevCloseDC(hdcMem);

            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error snapshotting screen: %s", ModErrorString(mrc));
                WinSendMsg(hwndClient, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, VIEW_NULL, &bmpNew, "(untitled)") )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            bView = VIEW_NULL;

            DiscardUndo();
            if ( fGotBitmap )
            {
                SetNoBitmap();
                fGotBitmap = FALSE;
                fUnsavedChanges = FALSE;
                fSelectionDefined = FALSE;
            }

            fGotBitmap = TRUE;
            fUnsavedChanges = TRUE;
            fSelectionDefined = FALSE;
            strcpy(szFileName, ""); /* Become untitled */
            Caption(hwndClient, " - %s", FileName());

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_UNDO          \45\ undo previous operation:16:*/
        case UM_UNDO:
        {
            BITMAP *pbmpFromUndo;
            const CHAR *szWhat;

            UseUndoBuffer(&pbmpFromUndo, &szWhat);

            fGotBitmap        = TRUE;
            fUnsavedChanges   = TRUE;
            fSelectionDefined = FALSE;

            SetBitmap(pbmpFromUndo);
            DiscardUndo();

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_SELECT        \45\ select region of bitmap:16:*/
        case UM_SELECT:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            const BITMAPINFOHEADER2 * pbmp = &(drawingBitmap.bmp2);
            SWP swpBitmap, swpScroller;
            int cxScrollbar, cyScrollbar, x, y, cx, cy;

            fSelectionDefined = FALSE;
            WinInvalidateRect(hwndBitmap, NULL, TRUE);
            WinUpdateWindow(hwndBitmap);
            WinQueryWindowPos(hwndBitmap  , &swpBitmap  );
            WinQueryWindowPos(hwndScroller, &swpScroller);
            cxScrollbar = (int) WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
            cyScrollbar = (int) WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);
            x = - (int) swpBitmap.x              ; if ( x < 0 ) x = 0;
            y = - (int) swpBitmap.y + cyScrollbar; if ( y < 0 ) y = 0;
            cx = (int) swpScroller.cx - cxScrollbar; if ( cx > (int) pbmp->cx ) cx = (int) pbmp->cx;
            cy = (int) swpScroller.cy - cyScrollbar; if ( cy > (int) pbmp->cy ) cy = (int) pbmp->cy;
            fSelectionDefined = Select(hwndClient, hwndBitmap, &rclSelection, x, y, cx, cy);
            if ( fSelectionDefined )
            {
                if ( rclSelection.xLeft   != rclSelection.xRight &&
                     rclSelection.yBottom != rclSelection.yTop   )
                {
                    WinInvalidateRect(hwndBitmap, NULL, TRUE);
                    WinUpdateWindow(hwndBitmap);
                }
            }

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_DESELECT      \45\ de\45\select any selcted area:16:*/
        case UM_DESELECT:
            if ( fSelectionDefined )
            {
                fSelectionDefined = FALSE;
                WinInvalidateRect(hwndBitmap, NULL, TRUE);
                WinUpdateWindow(hwndBitmap);
            }

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        /*...e*/
        /*...sUM_COPY          \45\ copy bitmap to clipboard:16:*/
        case UM_COPY:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            if ( fSelectionDefined )
                CopyToClipbrd(hwndClient, rclSelection);
            else
            {
                RECTL rclAll;
                rclAll.xLeft = 0; rclAll.xRight = drawingBitmap.mod.gbm.w;
                rclAll.yBottom = 0; rclAll.yTop = drawingBitmap.mod.gbm.h;
                CopyToClipbrd(hwndClient, rclAll);
            }
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_PASTE         \45\ paste from clipboard:16:*/
        case UM_PASTE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            HAB hab = WinQueryAnchorBlock(hwndClient);
            HDC hdc; SIZEL sizl; HPS hps; HBITMAP hbmClipbrd;
            MOD_ERR mrc; MOD modNew;
            BITMAP bmpNew;

            #pragma pack(2)
            struct
            {
                BITMAPINFOHEADER2 bmp2;
                RGB2 argb2Color[0x100];
            } bm;
            #pragma pack()

            WinOpenClipbrd(hab);
            if ( (hbmClipbrd = (HBITMAP) WinQueryClipbrdData(hab, CF_BITMAP)) == (HBITMAP) NULL )
                /* Not able to get bitmap from the clipboard */
            {
                WinCloseClipbrd(hab);
                Warning(hwndClient, "Error pasting: can't access clipboard data");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            memset(&(bm.bmp2), 0, sizeof(bm.bmp2));
            bm.bmp2.cbFix = 16;
            GpiQueryBitmapInfoHeader(hbmClipbrd, &bm.bmp2);

            if ( lBitCountScreen < bm.bmp2.cBitCount )
                /* Data only actually stored in clipboard quality */
                bm.bmp2.cBitCount = lBitCountScreen;

            if ( bm.bmp2.cBitCount == 16 )
                bm.bmp2.cBitCount = 24;

            if ( bm.bmp2.cPlanes != 1 )
            {
                WinCloseClipbrd(hab);
                Warning(hwndClient, "Error pasting: don't know how to handle cPlanes != 1");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( (hdc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
            {
                WinCloseClipbrd(hab);
                Warning(hwnd, "Error pasting: can't create PM HDC resource");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            sizl.cx = bm.bmp2.cx;
            sizl.cy = bm.bmp2.cy;
            if ( (hps = GpiCreatePS(hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
            {
                DevCloseDC(hdc);
                WinCloseClipbrd(hab);
                Warning(hwnd, "Error pasting: can't create PM HPS resource");
                return (MRESULT) 0;
            }

            LowPri();
            Caption(hwndClient, " - pasting %s", FileName());
            GpiSetBitmap(hps, hbmClipbrd);
            mrc = ModCreateFromHPS(hps, bm.bmp2.cx, bm.bmp2.cy,
                ( bm.bmp2.cBitCount > 8 ) ? 24 : bm.bmp2.cBitCount,
                &modNew);
            GpiSetBitmap(hps, (HBITMAP) NULL);
            Caption(hwndClient, " - %s", FileName());
            RegPri();

            GpiDestroyPS(hps);
            DevCloseDC(hdc);
            WinCloseClipbrd(hab);

            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error pasting: can't query bitmap bits from clipboard");
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            /* Now modNew contains a bitmap from the clipboard */

            if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, "(untitled)") )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            if ( fGotBitmap )
            {
                fGotBitmap        = FALSE;
                fUnsavedChanges   = FALSE;
                fSelectionDefined = FALSE;
                KeepForUndo(&drawingBitmap, "paste");
                SetNoBitmap();
            }
            else
                DiscardUndo();

            fGotBitmap        = TRUE;
            fUnsavedChanges   = TRUE;
            fSelectionDefined = FALSE;
            Caption(hwndClient, " - %s", FileName());

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_REF_HORZ      \45\ reflect horizontally:16:*/
        case UM_REF_HORZ:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModReflectHorz, "reflecting horizontally", "horizontal reflection");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_REF_VERT      \45\ reflect vertically:16:*/
        case UM_REF_VERT:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModReflectVert, "reflecting vertically", "vertical reflection");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_ROT_90        \45\ rotate 90 degrees:16:*/
        case UM_ROT_90:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModRotate90, "rotating by 90 degrees", "90 degree rotation");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_ROT_180       \45\ rotate 180 degrees:16:*/
        case UM_ROT_180:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModRotate180, "rotating by 180 degrees", "180 degree rotation");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_ROT_270       \45\ rotate 270 degrees:16:*/
        case UM_ROT_270:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModRotate270, "rotating by 270 degrees", "270 degree rotation");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_TRANSPOSE     \45\ transpose x for y:16:*/
        case UM_TRANSPOSE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            Reflect(hwndClient, ModTranspose, "transposing", "transposition");
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_CROP          \45\ crop to selection:16:*/
        case UM_CROP:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc; MOD modNew;
            BITMAP bmpNew;

            LowPri();
            Caption(hwndClient, " - cropping %s", FileName());
            mrc = ModExtractSubrectangle(&(drawingBitmap.mod),
                rclSelection.xLeft,
                rclSelection.yBottom,
                rclSelection.xRight - rclSelection.xLeft,
                rclSelection.yTop - rclSelection.yBottom,
                &modNew);
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error cropping: %s", ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            KeepForUndo(&drawingBitmap, "cropping");

            fUnsavedChanges = TRUE;
            fSelectionDefined = FALSE;

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_COLOUR        \45\ colour space:16:*/
        case UM_COLOUR:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc; MOD modNew;
            BITMAP bmpNew;

            if ( !WinDlgBox(HWND_DESKTOP, hwndClient, ColourDlgProc, (HMODULE) NULL, RID_DLG_COLOUR, NULL) )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            LowPri();
            Caption(hwndClient, " - colour space mapping %s", FileName());
            mrc = ModColourAdjust(&(drawingBitmap.mod), map, gama, shelf, &modNew);
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error colour space mapping %s: %s", FileName(), ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            KeepForUndo(&drawingBitmap, "mapping");

            fUnsavedChanges = TRUE;

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_MAP           \45\ map:16:*/
        case UM_MAP:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc; MOD modNew;
            BYTE bViewFastSuggested, bViewFast;
            BITMAP bmpNew;

            if ( !WinDlgBox(HWND_DESKTOP, hwndClient, MapDlgProc, (HMODULE) NULL, RID_DLG_MAP, NULL) )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            /*...sspeedup rendering later:24:*/
            {
                CHAR *szFast = "You have just selected a mapping to a palette which has all "
                               "of its colours in the screen palette. Therefore halftoning "
                               "or error diffusion will cause no improvement - change View "
                               "setting to Raw PM mapping?";
                bViewFastSuggested = bViewFast = bView;

                switch ( iPal )
                {
                    case CVT_BW:
                        bViewFastSuggested = VIEW_NULL;
                        break;
                    case CVT_8:
                    case CVT_VGA:
                        if ( lBitCountScreen == 4 )
                            bViewFastSuggested = VIEW_NULL;
                        break;
                    case CVT_784:
                        if ( lBitCountScreen == 8 )
                            bViewFastSuggested = VIEW_NULL;
                        break;
                    case CVT_FREQ:
                    case CVT_RGB:
                        if ( lBitCountScreen == 16 &&
                             iKeepRed   <= 5 &&
                             iKeepGreen <= 6 &&
                             iKeepBlue  <= 5 )
                            bViewFastSuggested = VIEW_NULL;
                        break;
                }

                if ( bViewFastSuggested != bView )
                    switch ( WinMessageBox(HWND_DESKTOP, hwndClient, szFast, szAppName, 0, MB_YESNOCANCEL | MB_WARNING | MB_MOVEABLE) )
                    {
                        case MBID_YES:
                            bViewFast = bViewFastSuggested;
                            break;
                        case MBID_NO:
                            break;
                        case MBID_CANCEL:
                            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                            return (MRESULT) 0;
                    }
            }
            /*...e*/

            LowPri();
            Caption(hwndClient, " - mapping %s", FileName());
            mrc = ModBppMap(&(drawingBitmap.mod), iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, &modNew);
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error mapping %s: %s", FileName(), ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, bViewFast, &bmpNew, FileName()) )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            bView = bViewFast;

            KeepForUndo(&drawingBitmap, "mapping");

            fUnsavedChanges = TRUE;

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_REDEYE          \45\ redeye fix of selection:16:*/
        case UM_REDEYE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            LowPri();
            FixRedeyeSelection(hwndClient, "fixing red eye", "Fix red eye");
            RegPri();
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_RESIZE        \45\ resize:16:*/
        case UM_RESIZE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            MOD_ERR mrc; MOD modNew;
            BITMAP bmpNew;

            if ( !WinDlgBox(HWND_DESKTOP, hwndClient, ResizeDlgProc, (HMODULE) NULL, RID_DLG_RESIZE, NULL) )
            {
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            LowPri();
            Caption(hwndClient, " - resizing %s", FileName());

            if (resizingQualityEnabled && (resizeFilterIndex != LIT_NONE))
            {
              mrc = ModResize(&(drawingBitmap.mod),
                               cxNew, cyNew,
                               isGrayscalePalette(&(drawingBitmap.mod)),
                               resizingQualityEnabled,
                               resizeLboxItemsFilterId[resizeFilterIndex],
                               &modNew);
            }
            else
            {
              mrc = ModResize(&(drawingBitmap.mod),
                              cxNew, cyNew,
                              isGrayscalePalette(&(drawingBitmap.mod)),
                              resizingQualityEnabled,
                              -1,
                              &modNew);
            }
            Caption(hwndClient, " - %s", FileName());
            RegPri();
            if ( mrc != MOD_ERR_OK )
            {
                Warning(hwndClient, "Error resizing %s: %s", FileName(), ModErrorString(mrc));
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }

            if ( !MakeVisualBg(hwndClient, &modNew, bView, &bmpNew, FileName()) )
            {
                ModDelete(&modNew);
                WinSendMsg(hwnd, UM_DONE, NULL, NULL);
                return (MRESULT) 0;
            }
            ModDelete(&modNew);

            KeepForUndo(&drawingBitmap, "resizing");

            fSelectionDefined = FALSE;
            fUnsavedChanges   = TRUE;

            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);

            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_VIEW_NULL     \45\ view with no transform:16:*/
        case UM_VIEW_NULL:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            NewView(hwndClient, VIEW_NULL);
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_VIEW_HALFTONE \45\ view with error diffusion:16:*/
        case UM_VIEW_HALFTONE:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            NewView(hwndClient, VIEW_HALFTONE);
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_VIEW_ERRDIFF  \45\ view with halftoning:16:*/
        case UM_VIEW_ERRDIFF:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            NewView(hwndClient, VIEW_ERRDIFF);
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_ABOUT         \45\ about box:16:*/
        case UM_ABOUT:
        {
            HWND hwndClient = HWNDFROMMP(mp1);
            WinDlgBox(HWND_DESKTOP, hwndClient, AboutDlgProc, (HMODULE) NULL, RID_DLG_ABOUT, NULL);
            WinSendMsg(hwnd, UM_DONE, NULL, NULL);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sUM_DONE          \45\ done\44\ and set pointer back:16:*/
        case UM_DONE:
        {
            POINTL ptl;
            fBusy = FALSE;
            WinQueryPointerPos(HWND_DESKTOP, &ptl);
            WinSetPointerPos(HWND_DESKTOP, ptl.x, ptl.y);
            return (MRESULT) 0;
        }
        /*...e*/
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}
/*...e*/
/*...sObjectThread:0:*/
#define    CB_STACK_OBJECT    0x20000

static VOID ObjectThread(VOID *pvParams)
{
    HAB hab = WinInitialize(0);
    HMQ hmq = WinCreateMsgQueue(hab, 0);
    QMSG qmsg;

    pvParams=pvParams; /* Suppress 'unref' compiler warning */

    WinRegisterClass(hab, WC_GBMV2_OBJECT, ObjectWndProc, 0L, 0);

    hwndObject = WinCreateWindow(HWND_OBJECT, WC_GBMV2_OBJECT, NULL, 0L, 0,0,0,0,
            HWND_DESKTOP, HWND_BOTTOM, 0, NULL, NULL);

    while ( WinGetMsg(hab, &qmsg, (HWND) NULL, 0, 0) )
        WinDispatchMsg(hab, &qmsg);

    WinDestroyWindow(hwndObject);

    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);

    _endthread();
}
/*...e*/
/*...sGbmV2WndProc:0:*/
#define    WC_GBMV2 "GbmV2Class"

/*...smap MID_ to UM_:0:*/
typedef struct { int mid, um; } MIDUM;

static MIDUM mid_um[] =
{
  { MID_NEW,           UM_NEW           },
  { MID_OPEN,          UM_OPEN          },
  { MID_SAVE,          UM_SAVE          },
  { MID_SAVE_AS,       UM_SAVE_AS       },
  { MID_EXPORT_MET,    UM_EXPORT_MET    },
  { MID_PRINT,         UM_PRINT         },
  { MID_SNAPSHOT,      UM_SNAPSHOT      },
  { MID_UNDO,          UM_UNDO          },
  { MID_SELECT,        UM_SELECT        },
  { MID_DESELECT,      UM_DESELECT      },
  { MID_COPY,          UM_COPY          },
  { MID_PASTE,         UM_PASTE         },
  { MID_REF_HORZ,      UM_REF_HORZ      },
  { MID_REF_VERT,      UM_REF_VERT      },
  { MID_ROT_90,        UM_ROT_90        },
  { MID_ROT_180,       UM_ROT_180       },
  { MID_ROT_270,       UM_ROT_270       },
  { MID_TRANSPOSE,     UM_TRANSPOSE     },
  { MID_CROP,          UM_CROP          },
  { MID_COLOUR,        UM_COLOUR        },
  { MID_MAP,           UM_MAP           },
  { MID_REDEYE,        UM_REDEYE        },
  { MID_RESIZE,        UM_RESIZE        },
  { MID_VIEW_NULL,     UM_VIEW_NULL     },
  { MID_VIEW_HALFTONE, UM_VIEW_HALFTONE },
  { MID_VIEW_ERRDIFF,  UM_VIEW_ERRDIFF  },
  { MID_ABOUT,         UM_ABOUT         }
};

#define    N_MID_UM (sizeof(mid_um)/sizeof(mid_um[0]))
/*...e*/

MRESULT _System GbmV2WndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( (int) msg )
    {
        case WM_CREATE:
        {
            LONG x, y, w, h;
            HWND hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
            if (ProfileGetWindowPos(hwndFrame, szProfKeyMainWindowSize,
                                    &x, &y, &w, &h))
            {
                WinSetWindowPos(hwndFrame, HWND_TOP,
                                x, y, w, h, SWP_MOVE | SWP_SIZE | SWP_SHOW);
            }
            return (MRESULT) FALSE;
        }

#ifdef DEBUG
        /*...sWM_PAINT     \45\ repaint client area:16:*/
        /*
        Dead simple, as this window is usually completely covered by the scroller
        window.
        */
        case WM_PAINT:
        {
            HPS hps = WinBeginPaint(hwnd, (HPS) NULL, (RECTL *) NULL);
            GpiSetBackColor(hps, CLR_RED);
            GpiErase(hps);
            WinEndPaint(hps);
            return (MRESULT) 0;
        }
        /*...e*/
#endif
        /*...sWM_SIZE      \45\ user has resized window:16:*/
        case WM_SIZE:
        {
            SWP swp;

            WinQueryWindowPos(hwnd, &swp);
            WinSetWindowPos(hwndScroller, (HWND) NULL, 0, 0, swp.cx, swp.cy, SWP_SIZE);
        }
            break;
        /*...e*/
        /*...sWM_INITMENU  \45\ enable right menuitems:16:*/
        case WM_INITMENU:
        {
            HWND hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
            HWND hwndMenu = WinWindowFromID(hwndFrame, FID_MENU);

            switch ( SHORT1FROMMP(mp1) )
            {
                /*...sMID_FILE:32:*/
                case MID_FILE:
                    EnableMenuItem(hwndMenu, MID_NEW       , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_OPEN      , !fBusy              );
                    EnableMenuItem(hwndMenu, MID_SAVE      , !fBusy && fGotBitmap && (szFileName[0] != '\0') );
                    EnableMenuItem(hwndMenu, MID_SAVE_AS   , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_EXPORT_MET, !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_PRINT     , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_SNAPSHOT  , !fBusy              );
                    return (MRESULT) 0;
                /*...e*/
                /*...sMID_EDIT:32:*/
                case MID_EDIT:
                {
                    CHAR szUndo[100+1];
                    ULONG ulInfo;
                    HAB hab = WinQueryAnchorBlock(hwnd);
                    BOOL fClipbrd = WinQueryClipbrdFmtInfo(hab, CF_BITMAP, &ulInfo);

                    EnableMenuItem(hwndMenu, MID_UNDO      , !fBusy && fCanUndo);
                    if ( fCanUndo )
                        sprintf(szUndo, "~Undo: %s\tCtrl+Z", szWhatUndo);
                    else
                        sprintf(szUndo, "~Undo\tCtrl+Z");
                    WinSendMsg(hwndMenu, MM_SETITEMTEXT, MPFROMSHORT(MID_UNDO), MPFROMP(szUndo));
                    EnableMenuItem(hwndMenu, MID_SELECT    , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_DESELECT  , !fBusy && fSelectionDefined);
                    EnableMenuItem(hwndMenu, MID_COPY      , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_PASTE     , !fBusy && fClipbrd);
                    return (MRESULT) 0;
                }
                /*...e*/
                /*...sMID_BITMAP:32:*/
                case MID_BITMAP:
                {
                    const BOOL fSquare = !fSelectionDefined ||
                                         ( rclSelection.xRight - rclSelection.xLeft ==
                                           rclSelection.yTop - rclSelection.yBottom );
                    const BOOL fTrueColor24bpp = (drawingBitmap.mod.gbm.bpp == 24);
                    EnableMenuItem(hwndMenu, MID_REF_HORZ , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_REF_VERT , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_ROT_90   , !fBusy && fGotBitmap && fSquare);
                    EnableMenuItem(hwndMenu, MID_ROT_180  , !fBusy && fGotBitmap);
                    EnableMenuItem(hwndMenu, MID_ROT_270  , !fBusy && fGotBitmap && fSquare);
                    EnableMenuItem(hwndMenu, MID_TRANSPOSE, !fBusy && fGotBitmap && fSquare);
                    EnableMenuItem(hwndMenu, MID_CROP     , !fBusy && fGotBitmap && fSelectionDefined);
                    EnableMenuItem(hwndMenu, MID_COLOUR   , !fBusy && fGotBitmap && (!fSelectionDefined));
                    EnableMenuItem(hwndMenu, MID_MAP      , !fBusy && fGotBitmap && (!fSelectionDefined));
                    EnableMenuItem(hwndMenu, MID_REDEYE   , !fBusy && fGotBitmap && fSelectionDefined && fTrueColor24bpp);
                    EnableMenuItem(hwndMenu, MID_RESIZE   , !fBusy && fGotBitmap && (!fSelectionDefined));
                    return (MRESULT) 0;
                }
                /*...e*/
                /*...sMID_VIEW:32:*/
                case MID_VIEW:
                {
                    BOOL fHalftone = ( lBitCountScreen == 4 || lBitCountScreen == 8 || lBitCountScreen == 16 );
                    BOOL fErrDiff  = ( lBitCountScreen == 4 || lBitCountScreen == 8 || lBitCountScreen == 16 );

                /* Don't disable menu item when it is checked. This is against the PM menu rules and confuses the user
                    EnableMenuItem(hwndMenu, MID_VIEW_NULL    , !fBusy && fGotBitmap && bView != VIEW_NULL                 );
                    EnableMenuItem(hwndMenu, MID_VIEW_HALFTONE, !fBusy && fGotBitmap && bView != VIEW_HALFTONE && fHalftone);
                    EnableMenuItem(hwndMenu, MID_VIEW_ERRDIFF , !fBusy && fGotBitmap && bView != VIEW_ERRDIFF  && fErrDiff );
                */
                    EnableMenuItem(hwndMenu, MID_VIEW_NULL    , !fBusy && fGotBitmap             );
                    EnableMenuItem(hwndMenu, MID_VIEW_HALFTONE, !fBusy && fGotBitmap && fHalftone);
                    EnableMenuItem(hwndMenu, MID_VIEW_ERRDIFF , !fBusy && fGotBitmap && fErrDiff );
                    CheckMenuItem (hwndMenu, MID_VIEW_NULL    , bView == VIEW_NULL    );
                    CheckMenuItem (hwndMenu, MID_VIEW_HALFTONE, bView == VIEW_HALFTONE);
                    CheckMenuItem (hwndMenu, MID_VIEW_ERRDIFF , bView == VIEW_ERRDIFF );
                    return (MRESULT) 0;
                }
                /*...e*/
            }
        }
            break;
        /*...e*/
        /*...sWM_COMMAND   \45\ menu command:16:*/
        case WM_COMMAND:
        {
            USHORT cmd = SHORT1FROMMP(mp1);
            int i;

            /* Try to handle majority case */
            for ( i = 0; i < N_MID_UM; i++ )
                if ( mid_um[i].mid == cmd )
                {
                    fBusy = TRUE;
                    WinPostMsg(hwndObject, mid_um[i].um, MPFROMHWND(hwnd), NULL);
                    return (MRESULT) 0;
                }

            /* Now try infrequent cases */
            switch ( cmd )
            {
                /*...sMID_HELP_FOR_HELP \45\ bring up help for help:32:*/
                case MID_HELP_FOR_HELP:
                    HlpHelpForHelp(hwndHelp);
                    break;
                /*...e*/
                /*...sMID_EXIT          \45\ initiate shutdown of this app:32:*/
                case MID_EXIT:
                    WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
                    break;
                /*...e*/
            }
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sWM_CLOSE     \45\ close window:16:*/
        case WM_CLOSE:
        {
            HWND hwndFrame = WinQueryWindow(hwnd, QW_PARENT);
            ProfileSetWindowPos(hwndFrame, szProfKeyMainWindowSize);
            WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
            return (MRESULT) 0;
        }
        /*...e*/
        /*...sWM_CONTROL   \45\ override scrolling amount:16:*/
        case WM_CONTROL:
            if ( SHORT1FROMMP(mp1) == WID_SCROLL )
                switch ( SHORT2FROMMP(mp1) )
                {
                    case SCN_HPAGE:
                    case SCN_VPAGE:
                        return (MRESULT) ((LONG) SHORT2FROMMP(mp2));
                }
            break;
        /*...e*/
        /*...sWM_ACTIVATE  \45\ ensure help instance is ours:16:*/
        case WM_ACTIVATE:
        {
            HWND hwndFrame = WinQueryWindow(hwnd, QW_PARENT);

            if ( mp1 && hwndHelp != (HWND) NULL )
                HlpActivate(hwndHelp, hwndFrame);
        }
            break;
        /*...e*/
        /*...sHM_\42\         \45\ redirect help support:16:*/
        case HM_ERROR:
        case HM_INFORM:
        case HM_QUERY_KEYS_HELP:
        case HM_EXT_HELP_UNDEFINED:
        case HM_HELPSUBITEM_NOT_FOUND:
            return HlpWndProc(hwnd, msg, mp1, mp2);
        /*...e*/
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}
/*...e*/

/* =================================================== */

/*...smain:0:*/
int main(int argc, CHAR *argv[])
{
    HAB  hab = WinInitialize(0);
    HMQ  hmq = WinCreateMsgQueue(hab, 0);
    QMSG qmsg;
    HWND hwndFrame, hwndClient;
    SWP  swp;
    ULONG flFrameFlags = FCF_TITLEBAR      | FCF_SYSMENU  |
                         FCF_SIZEBORDER    | FCF_MINMAX   |
                         FCF_MENU          | FCF_ICON     |
                         FCF_SHELLPOSITION | FCF_TASKLIST |
                         FCF_ACCELTABLE    | FCF_NOBYTEALIGN;

    ProfileSetProfileNameProgramName(argv[0]);

    memset(&drawingBitmap, 0, sizeof(drawingBitmap));

    gbm_init();

    /*...sdetermine lBitCountScreen:8:*/
    {
        HPS hps = WinGetPS(HWND_DESKTOP);
        HDC hdc = GpiQueryDevice(hps);
        DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1L, &lBitCountScreen);
        WinReleasePS(hps);
    }
    /*...e*/

    DosCreateMutexSem(NULL, &hmtxBmp, 0 /*=Private*/, FALSE /*=unowned*/ );

    RegisterScrollClass(hab);
    WinRegisterClass(hab, WC_GBMV2, GbmV2WndProc, CS_CLIPCHILDREN|CS_SIZEREDRAW, 0);
    WinRegisterClass(hab, WC_BITMAP, BitmapWndProc, 0L, 0);

    hwndFrame = WinCreateStdWindow(
        HWND_DESKTOP,      /* Parent window handle              */
        WS_VISIBLE,        /* Style of frame window             */
        &flFrameFlags,     /* Pointer to control data           */
        WC_GBMV2,          /* Client window class name          */
        NULL,              /* Title bar text                    */
        0L,                /* Style of client window            */
        (HMODULE) NULL,    /* Module handle for resources       */
        RID_GBMV2,         /* ID of resources                   */
        &hwndClient);      /* Pointer to client window handle   */

    WinSetWindowText(hwndFrame, szAppName);

    WinQueryWindowPos(hwndClient, &swp);

    hwndScroller = WinCreateWindow(hwndClient, WC_SCROLL, "", WS_VISIBLE|SCS_HSCROLL|SCS_VSCROLL|SCS_HCENTRE|SCS_VCENTRE|SCS_HPAGE|SCS_VPAGE,
         0,0,swp.cx,swp.cy, hwndClient, HWND_BOTTOM, WID_SCROLL, NULL, NULL);
    hwndBitmap = WinCreateWindow(hwndScroller, WC_BITMAP, "", WS_VISIBLE|WS_CLIPSIBLINGS,
         0,0,0,0, hwndScroller, HWND_BOTTOM, WID_BITMAP, NULL, NULL);
    WinSetFocus(HWND_DESKTOP, hwndBitmap);

    _beginthread(ObjectThread, NULL, CB_STACK_OBJECT, NULL);

    if ( argc >= 2 )
    {
        GBMTOOL_FILEARG gbmfilearg;
        CHAR opt_src[GBMTOOL_OPTIONS_MAX+1];

        MOD_ERR mrc; MOD modNew;
        BITMAP bmpNew;

        /* Split filename and file options. */
        gbmfilearg.argin = argv[1];
        /* accept also empty filenames just quoted by "" */
        if (strcmp(gbmfilearg.argin, "\"\"") != 0)
        {
          if (gbmtool_parse_argument(&gbmfilearg, FALSE) == GBM_ERR_OK)
          {
             strcpy(szFileName , gbmfilearg.files->filename);
            strcpy(opt_src, gbmfilearg.options);
          }
          else
          {
            strcpy(szFileName, gbmfilearg.argin);
            strcpy(opt_src   , "");
          }
          gbmtool_free_argument(&gbmfilearg);

          if ( (mrc = ModCreateFromFile(szFileName, opt_src, &modNew)) != MOD_ERR_OK )
          {
            Warning(HWND_DESKTOP, "Error loading %s: %s", szFileName, ModErrorString(mrc));
          }
          else if ( !MakeVisual(&modNew, bView, &bmpNew) )
          {
            ModDelete(&modNew);
            Warning(HWND_DESKTOP, "Error loading %s: can't create view bitmap", szFileName);
          }
          else
          {
            ModDelete(&modNew);
            fGotBitmap = TRUE;
            Caption(hwndClient, " - %s", szFileName);
            SetBitmap(&bmpNew);
            DeleteBitmap(&bmpNew);
          }
        }
    }

    if ( (hwndHelp = HlpInit(hwndClient,
               (HMODULE) NULL, RID_HELP_TABLE,
               "GBMV2.HLP GBMDLG.HLP", szAppName)) != (HWND) NULL )
        HlpActivate(hwndHelp, hwndFrame);

    do
        for ( ;; )
        {
            while ( WinGetMsg(hab, &qmsg, (HWND) NULL, 0, 0) )
                WinDispatchMsg(hab, &qmsg);
            if ( !fBusy )
                break;
            Warning(hwndFrame, "Cannot Close yet, %s is busy, please wait and retry", szAppName);
        }
    while ( !SaveChanges(hwndFrame) );

    if ( hwndHelp != (HWND) NULL )
        HlpDeinit(hwndHelp);

    WinPostMsg(hwndObject, WM_QUIT, NULL, NULL);

    WinDestroyWindow(hwndBitmap);
    WinDestroyWindow(hwndScroller);
    WinDestroyWindow(hwndFrame);

    DosCloseMutexSem(hmtxBmp);

    gbm_deinit();

    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);

    return 0;
}
/*...e*/


