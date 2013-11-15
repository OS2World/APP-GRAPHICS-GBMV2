/*

gbmdlgrx.c - GBM File Dialog (GBMDLG) REXX adapter library

Author : Heiko Nitzsche
License: Public Domain

History
-------
30-Apr-2006: 1.00: Inital version
15-May-2006: 1.01: Add support for querying version number
16-Aug-2006: 1.02: Add bldlevel info
23-Nov-2006: 1.03: Fix file dialog subclassing, require now GBM File Dialog 1.31
                   Make library usable from multiple threads of a process (is thread-safe now)
*/

#define  INCL_BASE
#define  INCL_DOS
#define  INCL_WIN
#define  INCL_REXXSAA
#include <os2.h>

/* GCC on OS/2 using EMX headers has its own defines for REXX */
/* which are included via os2.h */
#ifndef RXSHV_SYSET
 #include <rexxsaa.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>

#include "gbmdlg.h"
#include "gbmdlgrx.h"

#include "rexxhlp.h"


/*********************************************************************/
/* GBMDLGRX.DLL version                                              */
/*********************************************************************/
#define GBMDLG_VERSION  "1.04"

/*********************************************************************/
/* Numeric Error Return Strings                                      */
/*********************************************************************/
#define  NOERROR_RET    "0"          /* No error whatsoever          */

/*********************************************************************/
/* Alpha Numeric Return Strings                                      */
/*********************************************************************/
#define  BUTTON_OK      "OK"         /* OK button selected           */
#define  BUTTON_CANCEL  "CANCEL"     /* CANCEL button selected       */
#define  ERROR_RETSTR   "ERROR"      /* An error occured             */

/*********************************************************************/
/* Numeric Return calls                                              */
/*********************************************************************/
#define  INVALID_ROUTINE 40            /* Raise Rexx error           */
#define  VALID_ROUTINE    0            /* Successful completion      */


/*********************************************************************/
/*  Declare all exported functions as REXX functions.                */
/*********************************************************************/

/* Unfortunately there is a prototype conflict between the     */
/* OS/2 Toolkit and the EMX headers. Thus we redefine our own. */

typedef ULONG APIENTRY MyRexxFunctionHandler(PUCHAR,
                                             ULONG,
                                             PRXSTRING,
                                             PSZ,
                                             PRXSTRING);

MyRexxFunctionHandler GBMDLG_LoadFuncs;
MyRexxFunctionHandler GBMDLG_DropFuncs;

MyRexxFunctionHandler GBMDLG_OpenFileDlg;
MyRexxFunctionHandler GBMDLG_SaveAsFileDlg;

MyRexxFunctionHandler GBMDLG_VersionRexx;

/*********************************************************************/
/* RxFncTable                                                        */
/*   Array of names of the REXXUTIL functions.                       */
/*   This list is used for registration and deregistration.          */
/*********************************************************************/

static PSZ RxFncTable[] =
{
  "GBMDLG_LoadFuncs",
  "GBMDLG_DropFuncs",
  "GBMDLG_OpenFileDlg",
  "GBMDLG_SaveAsFileDlg",
  "GBMDLG_VersionRexx"
};


/*********************************************************************/
/* Some useful macros                                                */
/*********************************************************************/
#define BUILDRXSTRING(t, s) { \
  strcpy((t)->strptr,(s));\
  (t)->strlength = strlen((s)); \
}

/*********************************************************************/
/* variables                                                         */
/*********************************************************************/

typedef struct
{
   GBMFILEDLG gbmfild;   /* GBM File Dialog struct */
   PSZ        helpTitle; /* The window title of the online help */
} THREAD_ARGS;


/*********************************************************************/
/* Window handling                                                   */
/*********************************************************************/

MRESULT EXPENTRY GBMDLGRX_WindowFunc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  const FILEDLG * fild = (const FILEDLG *) WinQueryWindowULong(hwnd, QWL_USER);
  if (fild != NULL)
  {
      const HWND hwndHelp = fild->ulUser;
      switch(msg)
      {
         /* We need to intercept the help manager message as the GBM file dialog
          * usually post help messages to the owner window (which we don't have here).
          */
         case WM_ACTIVATE:
         {
            const HWND hwndActivated = HWNDFROMMP(mp2);
            if (hwndActivated == hwnd)
            {
                const BOOL activated = SHORT1FROMMP(mp1); /* usactive */
                if (activated)
                {
                    WinAssociateHelpInstance(hwndHelp, hwnd);
                }
                else
                {
                    WinAssociateHelpInstance(NULLHANDLE, hwnd);
                }
            }
            break;
         }

         case WM_HELP:
           WinSendMsg(hwndHelp, HM_DISPLAY_HELP, MPFROMSHORT(HID_GBM_FILEDLG),
                                                 MPFROMSHORT(HM_RESOURCEID));
           return ((MRESULT) TRUE);

         default:
           break;
      }
  }
  return GbmDefFileDlgProc(hwnd, msg, mp1, mp2);
}

/*********************************************************************/
const unsigned THREAD_STACK_SIZE = 0xf000;


static void ShowGBMFILEDLG(void * args)
{
  HAB   hab        = NULLHANDLE;  /* Anchor-block handle  */
  HMQ   hmq        = NULLHANDLE;  /* Message queue handle */
  HWND  hwndDlg    = NULLHANDLE;  /* GBM File Dialog window handle */
  PPIB  ppib       = NULL;        /* Process information block pointer */
  PTIB  ptib       = NULL;        /* Thread information block */
  ULONG pib_ultype;               /* process type backup for morphing PM<->VIO */

  HELPINIT  helpinit;
  HWND      hwndHelp;

  THREAD_ARGS * targs = (THREAD_ARGS *) args;

  /* morph to PM to get message queue working */
  DosGetInfoBlocks(&ptib, &ppib);
  pib_ultype = ppib->pib_ultype;
  ppib->pib_ultype = 3; /* PM */

  /* Initialize PM */
  hab = WinInitialize(0);

  if (hab == NULLHANDLE)
  {
     return;
  }
  hmq = WinCreateMsgQueue(hab, 0);
  if (hmq == NULLHANDLE)
  {
     WinTerminate(hab);
     ppib->pib_ultype = pib_ultype;
     return;
  }

  /* Setup the help initialization structure */
  helpinit.cb                       = sizeof(HELPINIT);
  helpinit.ulReturnCode             = 0L;
  helpinit.pszTutorialName          = (PSZ)NULL;

  /* Help table in application resource */
  helpinit.phtHelpTable             = (HELPTABLE *) (0xffff0000L | HELP_TABLE_GBMDLGRX);
  helpinit.hmodHelpTableModule      = NULLHANDLE;

  /* Default action bar and accelerators */
  helpinit.hmodAccelActionBarModule = NULLHANDLE;
  helpinit.idAccelTable             = 0;
  helpinit.idActionBar              = 0;
  helpinit.pszHelpWindowTitle       = targs->helpTitle;
  helpinit.fShowPanelId             = CMIC_HIDE_PANEL_ID;
  helpinit.pszHelpLibraryName       = "GBMDLG.HLP";

  /* Create and associate the help instance */
  hwndHelp = WinCreateHelpInstance(hab, &helpinit);

  /* subclass file dialog window procedure to trigger help manager */
  targs->gbmfild.fild.pfnDlgProc = GBMDLGRX_WindowFunc;
  targs->gbmfild.fild.ulUser     = hwndHelp;

  /* PM is up, create GBM File Dialog */
  hwndDlg = GbmFileDlg(HWND_DESKTOP, HWND_DESKTOP, &targs->gbmfild);

  if (hwndHelp)
  {
     /* PM is up, process GBM File Dialog */
     WinProcessDlg(hwndDlg);

     /* Remove help instance */
     WinDestroyHelpInstance(hwndHelp);
  }
  else
  {
     WinDestroyWindow(hwndDlg);
  }

  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);

  /* morph back */
  ppib->pib_ultype = pib_ultype;

  _endthread();
}

/*********************************************************************/
/* Function:  GBMDLG_LoadFuncs                                       */
/*                                                                   */
/* Syntax:    call GBMDLG_LoadFuncs                                  */
/*                                                                   */
/* Params:    none                                                   */
/*                                                                   */
/* Return:    null string                                            */
/*********************************************************************/

ULONG GBMDLG_LoadFuncs(PUCHAR name, ULONG numargs, RXSTRING args[],
                       PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0; /* return a null string result*/

  if (numargs != 0)
  {
    return INVALID_ROUTINE;
  }

  RegisterRexxFunctions("GBMDLGRX",
                        RxFncTable,
                        sizeof(RxFncTable)/sizeof(PSZ));

  return VALID_ROUTINE;  /* no error on call */
}

/*********************************************************************/
/* Function:  GBMDLG_DropFuncs                                       */
/*                                                                   */
/* Syntax:    call GBMDLG_DropFuncs                                  */
/*                                                                   */
/* Params:    none                                                   */
/*                                                                   */
/* Return:    null string                                            */
/*********************************************************************/

ULONG GBMDLG_DropFuncs(PUCHAR name, ULONG numargs, RXSTRING args[],
                       PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0;           /* return a null string result*/

  if (numargs != 0)                /* no arguments for this      */
  {
    return INVALID_ROUTINE;
  }

  UnregisterRexxFunctions(RxFncTable,
                          sizeof(RxFncTable)/sizeof(PSZ));

  return VALID_ROUTINE;  /* no error on call */
}


/*********************************************************************/
/* Function:  GBMDLG_VersionRexx                                     */
/*                                                                   */
/* Syntax:    GBMDLG_VersionRexx()                                   */
/*                                                                   */
/* Return:    Version number: "major.minor"                          */
/*********************************************************************/

ULONG GBMDLG_VersionRexx(PUCHAR name, ULONG numargs, RXSTRING args[],
                         PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0;           /* return a null string result*/

  if (numargs != 0)                /* no arguments for this      */
  {
    return INVALID_ROUTINE;
  }

  /* build return string ("major.minor")*/
  BUILDRXSTRING(retstr, GBMDLG_VERSION);

  return VALID_ROUTINE;  /* no error on call */
}



/**************************************************************************/
/* Function:  GBMDLG_OpenFileDlg                                          */
/*                                                                        */
/* Syntax:    call GBMDLG_OpenFileDlg title stem1 stem2                   */
/*                                                                        */
/* Params:    title  in     - help window title                           */
/*            stem1  in/out - filename                                    */
/*            stem2  in/out - options (comma separated)                   */
/*                                                                        */
/* Return:    BUTTON_OK      The OK button was selected. Filename/options */
/*                           have been returned in stems.                 */
/*            BUTTON_CANCEL  The CANCEL button was selected.              */
/*                           Filename/options are unchanged.              */
/*            ERROR_RETSTR   An error occured.                            */
/*                           Filename/options are unchanged.              */
/**************************************************************************/

ULONG GBMDLG_OpenFileDlg(PUCHAR name,        /* Routine name */
                         ULONG numargs,      /* Number of arguments */
                         RXSTRING args[],    /* Null-terminated RXSTRINGs array*/
                         PSZ   queuename,    /* Current external data queue name */
                         RXSTRING *retstr)   /* returning RXSTRING  */
{
  TID           tid;   /* Thread ID */
  THREAD_ARGS   targs; /* Thread arguments, contains GBMFILEDLG */
  GBMFILEDLG  * gbmfild = &targs.gbmfild;

  /* prevent compiler warnings */
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  /* validate arguments */
  if ((numargs != 3) || (! RXVALIDSTRING(args[0]))
                     || (! RXVALIDSTRING(args[1]))
                     || (! RXVALIDSTRING(args[2])))
  {
     return INVALID_ROUTINE;
  }

  memset(&gbmfild->fild, 0, sizeof(FILEDLG));
  gbmfild->fild.cbSize = sizeof(FILEDLG);
  gbmfild->fild.fl     = (FDS_CENTER | FDS_OPEN_DIALOG | FDS_HELPBUTTON | FDS_MODELESS);

  /* set help window title */
  targs.helpTitle = args[0].strptr;

  /* get the stem1 value (contains the filename) */
  if (! GetRexxVariable(args[1].strptr, gbmfild->fild.szFullFile, sizeof(gbmfild->fild.szFullFile)))
  {
     return INVALID_ROUTINE;
  }

  /* get the stem2 value (contains the comma separated options) */
  if (! GetRexxVariable(args[2].strptr, gbmfild->szOptions, sizeof(gbmfild->szOptions)))
  {
     return INVALID_ROUTINE;
  }

  /* show dialog in a separate PM thread with message queue */
  tid = _beginthread(ShowGBMFILEDLG, NULL, THREAD_STACK_SIZE, &targs);
  if (tid == -1)
  {
     return VALID_ROUTINE;  /* no error on call */
  }

  /* Wait for initialization doen as long as it takes. */
  if (NO_ERROR == DosWaitThread(&tid, DCWW_WAIT))
  {
     if (gbmfild->fild.lReturn == DID_OK)
     {
        /* set filename to be returned */
        if (! SetRexxVariable(args[1].strptr, gbmfild->fild.szFullFile))
        {
          return VALID_ROUTINE;  /* no error on call */
        }

        /* set options to be returned */
        if (! SetRexxVariable(args[2].strptr, gbmfild->szOptions))
        {
          return VALID_ROUTINE;
        }

        BUILDRXSTRING(retstr, BUTTON_OK);
     }
     else
     {
        BUILDRXSTRING(retstr, BUTTON_CANCEL);
     }
  }

  return VALID_ROUTINE;  /* no error on call */
}


/**************************************************************************/
/* Function:  GBMDLG_SaveAsFileDlg                                        */
/*                                                                        */
/* Syntax:    call GBMDLG_SaveAsFileDlg title stem1 stem2                 */
/*                                                                        */
/* Params:    title  in     - help window title                           */
/*            stem1  in/out - filename                                    */
/*            stem2  in/out - options (comma separated)                   */
/*                                                                        */
/* Return:    BUTTON_OK      The OK button was selected. Filename/options */
/*                           have been returned in stems.                 */
/*            BUTTON_CANCEL  The CANCEL button was selected.              */
/*                           Filename/options are unchanged.              */
/*            ERROR_RETSTR   An error occured.                            */
/*                           Filename/options are unchanged.              */
/**************************************************************************/

ULONG GBMDLG_SaveAsFileDlg(PUCHAR name,        /* Routine name */
                           ULONG numargs,      /* Number of arguments */
                           RXSTRING args[],    /* Null-terminated RXSTRINGs array*/
                           PSZ   queuename,    /* Current external data queue name */
                           RXSTRING *retstr)   /* returning RXSTRING  */
{
  TID           tid;   /* Thread ID */
  THREAD_ARGS   targs; /* Thread arguments, contains GBMFILEDLG */
  GBMFILEDLG  * gbmfild = &targs.gbmfild;

  /* prevent compiler warnings */
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  /* validate arguments */
  if ((numargs != 3) || (! RXVALIDSTRING(args[0]))
                     || (! RXVALIDSTRING(args[1]))
                     || (! RXVALIDSTRING(args[2])))
  {
     return INVALID_ROUTINE;
  }

  memset(&gbmfild->fild, 0, sizeof(FILEDLG));
  gbmfild->fild.cbSize = sizeof(FILEDLG);
  gbmfild->fild.fl     = (FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB | FDS_HELPBUTTON | FDS_MODELESS);

  /* set help window title */
  targs.helpTitle = args[0].strptr;

  /* get the stem1 value (contains the filename) */
  if (! GetRexxVariable(args[1].strptr, gbmfild->fild.szFullFile, sizeof(gbmfild->fild.szFullFile)))
  {
     return INVALID_ROUTINE;
  }

  /* get the stem2 value (contains the comma separated options) */
  if (! GetRexxVariable(args[2].strptr, gbmfild->szOptions, sizeof(gbmfild->szOptions)))
  {
     return INVALID_ROUTINE;
  }

  /* show dialog in a separate PM thread with message queue */
  tid = _beginthread(ShowGBMFILEDLG, NULL, THREAD_STACK_SIZE, &targs);
  if (tid == -1)
  {
     return VALID_ROUTINE;  /* no error on call */
  }

  /* Wait for initialization doen as long as it takes. */
  if (NO_ERROR == DosWaitThread(&tid, DCWW_WAIT))
  {
     if (gbmfild->fild.lReturn == DID_OK)
     {
        /* set filename to be returned */
        if (! SetRexxVariable(args[1].strptr, gbmfild->fild.szFullFile))
        {
          return VALID_ROUTINE;  /* no error on call */
        }

        /* set options to be returned */
        if (! SetRexxVariable(args[2].strptr, gbmfild->szOptions))
        {
          return VALID_ROUTINE;  /* no error on call */
        }

        BUILDRXSTRING(retstr, BUTTON_OK);
     }
     else
     {
        BUILDRXSTRING(retstr, BUTTON_CANCEL);
     }
  }

  return VALID_ROUTINE;   /* no error on call */
}


