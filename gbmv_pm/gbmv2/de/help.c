/*

help.c - PM Help

*/

/*...sincludes:0:*/
#define	INCL_DOS
#define	INCL_WIN
#define	INCL_GPI
#define	INCL_DEV
#include <os2.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include "gbmv2hlp.h"

/*...vgbmv2hlp\46\h:0:*/
/*...e*/

/*...sHlpWarning:0:*/
static VOID HlpWarning(HWND hwnd, const CHAR *szFmt, ...)
	{
	va_list vars;
	CHAR sz[256+1];

	va_start(vars, szFmt);
	vsprintf(sz, szFmt, vars);
	va_end(vars);
	WinMessageBox(HWND_DESKTOP, hwnd, sz, "Hilfesystem", 0, MB_OK | MB_WARNING | MB_MOVEABLE);
	}
/*...e*/
/*...sHlpInit:0:*/
static HELPINIT hinit;

HWND HlpInit(
	HWND hwnd,
	HMODULE hmod, USHORT idHelpTable,
	const CHAR *szHelpFile,
	const CHAR *szTitle
	)
	{
	HAB hab = WinQueryAnchorBlock(hwnd);
	HWND hwndHelp;
	CHAR sz[256+1];

	hinit.cb                       = sizeof(HELPINIT);
	hinit.ulReturnCode             = 0L;

	/* HELPTABLE resource handle */
	hinit.hmodHelpTableModule      = hmod;
	hinit.phtHelpTable             = (HELPTABLE *) (0xffff0000L | idHelpTable);

	/* Help window title bar */
	strcpy(sz, szTitle);
	strcat(sz, " Hilfe");
	hinit.pszHelpWindowTitle       = sz;

	/* Not showing panel IDs */
	hinit.fShowPanelId             = CMIC_HIDE_PANEL_ID;

	/* No tutorial program */
	hinit.pszTutorialName          = NULL;

	/* Action bar is not tailored */
	hinit.hmodAccelActionBarModule = (HMODULE) NULL;
	hinit.idAccelTable             = (USHORT) NULL;
	hinit.idActionBar              = (USHORT) NULL;

	/* No default librarys */
	hinit.pszHelpLibraryName       = "";

	if ( (hwndHelp = WinCreateHelpInstance(hab, &hinit)) == (HWND) NULL )
		{
		HlpWarning(hwnd, "Hilfeinstanz konnte nicht erzeugt werden");
		return (HWND) NULL;
		}

	if ( WinSendMsg(hwndHelp, HM_SET_HELP_LIBRARY_NAME, MPFROMP(szHelpFile), NULL) )
		{
		WinDestroyHelpInstance(hwndHelp);
		HlpWarning(hwnd, "Hilfebibliothek wurde nicht gefunden");
		return (HWND) NULL;
		}

	return hwndHelp;
	}
/*...e*/
/*...sHlpDeinit:0:*/
VOID HlpDeinit(HWND hwndHelp)
	{
	WinDestroyHelpInstance(hwndHelp);
	}
/*...e*/
/*...sHlpActivate:0:*/
VOID HlpActivate(HWND hwndHelp, HWND hwndFrame)
	{
	if ( !WinAssociateHelpInstance(hwndHelp, hwndFrame))
		HlpWarning(hwndFrame, "VerknÅpfung zur Hilfeinstanz fehlgeschlagen");
	}
/*...e*/
/*...sHlpWndProc:0:*/
MRESULT _System HlpWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
	{
	CHAR sz[256+1];

	switch ( (int) msg )
		{
/*...sHM_ERROR:16:*/
case HM_ERROR:
	switch( SHORT1FROMMP(mp1) )
		{
		case HMERR_HELPITEM_NOT_FOUND:
			sprintf(sz, "Thema 0x%08lx nicht gefunden", LONGFROMMP(mp2));
			HlpWarning(hwnd, sz);
			break;
		case HMERR_HELPSUBITEM_NOT_FOUND:
			sprintf(sz, "Kapitel 0x%08lx nicht gefunden", LONGFROMMP(mp2));
			HlpWarning(hwnd, sz);
			break;
		case HMERR_INDEX_NOT_FOUND:
			HlpWarning(hwnd, "Kein Index in Bibliothek verfÅgbar" );
			break;
		case HMERR_CONTENT_NOT_FOUND:
			HlpWarning(hwnd, "Bibliothek ist leer");
			break;
		case HMERR_OPEN_LIB_FILE:
			HlpWarning(hwnd, "Bibliothek kann nicht geîffnet werden");
			break;
		case HMERR_READ_LIB_FILE:
			HlpWarning(hwnd, "Bibliothek nicht lesbar");
			break;
		case HMERR_CLOSE_LIB_FILE:
			HlpWarning(hwnd, "Bibliothek konnte nicht geschlossen werden");
			break;
		case HMERR_INVALID_LIB_FILE:
			HlpWarning(hwnd, "Falsche Bibliothek angegeben");
			break;
		case HMERR_NO_MEMORY:
			HlpWarning(hwnd, "Nicht genÅgend freier Speicher");
			break;
		case HMERR_ALLOCATE_SEGMENT:
			HlpWarning(hwnd, "Nicht genÅgend freier Speicher fÅr IPF");
			break;
		case HMERR_FREE_MEMORY:
			HlpWarning(hwnd, "Speicherfreigabe fehlgeschlagen");
			break;
		case HMERR_PANEL_NOT_FOUND:
			HlpWarning(hwnd, "Hilfeanzeige nicht gefunden");
			break;
		case HMERR_DATABASE_NOT_OPEN:
			HlpWarning(hwnd, "Eine ungeîffnete Datenbank kann nicht gelesen werden");
			break;
		default:
			sprintf(sz, "Fehler mp1=0x%08lx mp2=0x%08lx",LONGFROMMP(mp1), LONGFROMMP(mp2));
			HlpWarning(hwnd, sz);
			break;
		}
	break;
/*...e*/
/*...sHM_INFORM:16:*/
case HM_INFORM:
	sprintf(sz, "Information 0x%08lx 0x%08lx",
		LONGFROMMP(mp1), LONGFROMMP(mp2));
	HlpWarning(hwnd, sz);
	break;
/*...e*/
/*...sHM_QUERY_KEYS_HELP:16:*/
case HM_QUERY_KEYS_HELP:
	return (MRESULT) HID_HELPKEYS;	
/*...e*/
/*...sHM_EXT_HELP_UNDEFINED:16:*/
case HM_EXT_HELP_UNDEFINED:
	sprintf(sz, "Erweiterte Hilfe nicht definiert mp1=0x%08lx mp2=0x%08lx",
		LONGFROMMP(mp1), LONGFROMMP(mp2));
	HlpWarning(hwnd, sz);
	break;
/*...e*/
/*...sHM_HELPSUBITEM_NOT_FOUND:16:*/
case HM_HELPSUBITEM_NOT_FOUND:
	sprintf(sz, "Kapitel 0x%08lx 0x%08lx nicht gefunden",
		LONGFROMMP(mp1), LONGFROMMP(mp2));
	HlpWarning(hwnd, sz);
	break;
/*...e*/
		}
	return WinDefWindowProc(hwnd, msg, mp1, mp2);
	}
/*...e*/
/*...sHlpHelpForHelp:0:*/
VOID HlpHelpForHelp(HWND hwndHelp)
	{
	WinPostMsg(hwndHelp, HM_DISPLAY_HELP, NULL, NULL);
	}
/*...e*/
