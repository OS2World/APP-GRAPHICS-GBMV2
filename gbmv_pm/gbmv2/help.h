/*

help.h - PM Help

*/

#ifndef HELP_H
#define	HELP_H

extern HWND HlpInit(
	HWND hwnd,
	HMODULE hmod, USHORT idHelpTable,
	const CHAR *szHelpFile,
	const CHAR *szTitle
	);

extern VOID HlpDeinit(HWND hwndHelp);
extern VOID HlpActivate(HWND hwndHelp, HWND hwndFrame);
extern VOID HlpHelpForHelp(HWND hwndHelp);

extern MRESULT _System HlpWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

#endif
