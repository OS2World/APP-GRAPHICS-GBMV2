/*******************************************************************/
/*                                                                 */
/* This program calls the various REXX external functions          */
/* provided in the gbmdlgrx.c function package.                    */
/*                                                                 */
/*******************************************************************/
/* '@echo off' */

call RxFuncAdd 'GBMDLG_LoadFuncs', 'GBMDLGRX', 'GBMDLG_LoadFuncs'
call GBMDLG_LoadFuncs


/* Get version number of GBMDLGRX lib ***/
gbmdlgVersionRexx = GBMDLG_VersionRexx()
say 'GBMDLGRX.DLL version: 'gbmdlgVersionRexx


/*** Show Open File Dialog ***/
openFilename = 'f:\Bilder\mailtux.gif'
openOptions  = 'index=0'
errorMessage = GBMDLG_OpenFileDlg('GBM File Dialog', 'openFilename', 'openOptions')
say 'Open filename:   'openFilename
say 'Open options :   'openOptions
say 'Error message:   'errorMessage

saveOptions  = 'lzw'
saveFilename = 'z:\x.tif'
errorMessage = GBMDLG_SaveAsFileDlg('GBM File Dialog', 'saveFilename', 'saveOptions')
say 'Save filename:   'saveFilename
say 'Save options :   'saveOptions
say 'Error message:   'errorMessage

call GBMDLG_DropFuncs


