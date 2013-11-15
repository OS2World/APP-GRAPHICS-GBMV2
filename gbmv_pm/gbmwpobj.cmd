/* REXX script to make the Workplace Shell objects for GbmV and GbmV2 */

Call rxfuncadd 'sysloadfuncs', 'rexxutil', 'sysloadfuncs'
Call sysloadfuncs

assoc = '*.BMP,*.VGA,*.BGA,*.RLE,*.DIB,*.RL4,*.RL8,*.GIF,*.PCX,*.PCC,*.TIF,*.TGA,*.VST,*.AFI,*.IFF,*.LBM,*.VID,*.PGM,*.PPM,*.KPS,*.IAX,*.XBM,*.SPR,*.SPRITE,*.PSE,*.PSEG,*.PSEG*,*.IMG,*.CVP,*.JPG,*.JPEG,*.JPE,*.PNG,*.XPM,*.JP2,*J2K,*.JPC,*.J2C,*.JPT,*.JBG'

/* Lookup current dir and propose to the user */
fullpath = directory()

Say "Installation directory: "fullpath
Say "Press Y and RETURN if you accept the proposed installation directory."
Say "Press any other key and RETURN if you want to specify a different location."
Parse Upper Pull fullpath_accept

If fullpath_accept <> 'Y' then
do
  Say "Please type full path where GbmV and GbmV2 are located (e.g. D:\GBM)"
  Parse Upper Pull fullpath
end

Say "Creating Desktop objects..."

/* Get rid of an eventually trailing \ */
trail = RIGHT(fullpath, 1)
if trail = '\' then
do
  fullpath = LEFT(fullpath, LENGTH(fullpath)-1)
end

/* Lookup GBMV.EXE */
rc = SysFileTree(fullpath'\gbmv.exe' , 'file_gbmv' , 'FO')
If file_gbmv.0 <> 0 then
do
  Call SysCreateObject 'WPProgram','GbmV','<WP_DESKTOP>','EXENAME='||fullpath||'\GBMV.EXE;OBJECTID=<GBMFOLDER_GBMVEXE>;PROGTYPE=PM;PARAMETERS=-e "%*";STARTUPDIR='||fullpath||';ASSOCFILTER='||assoc,'R'
  Say "  Workplace Shell Object for GbmV  created on the desktop."
end
else
do
  Say "  GBMV.EXE was not found in path "fullpath
end

/* Lookup GBMV2.EXE */
rc = SysFileTree(fullpath'\gbmv2.exe', 'file_gbmv2', 'FO')
If file_gbmv2.0 <> 0 then
do
  Call SysCreateObject 'WPProgram','GbmV2','<WP_DESKTOP>','EXENAME='||fullpath||'\GBMV2.EXE;OBJECTID=<GBMFOLDER_GBMV2EXE>;PROGTYPE=PM;PARAMETERS="%*";STARTUPDIR='||fullpath||';ASSOCFILTER='||assoc,'R'
  Say "  Workplace Shell Object for GbmV2 created on the desktop."
end
else do
  Say "  GBMV2.EXE was not found in path "fullpath
end

Say "Done."

/* Show hint about Iconedit and BMP association */
If (file_gbmv.0 <> 0) | (file_gbmv2.0 <> 0) then
do
  Say
  Say "These have associations for all GBM file formats, including *.BMP files."
  Say "This means you can just click on a bitmap file to get it displayed."
  Say "However, the Icon Editor also lays multiple claims to *.BMP files."
  Say "So you may get Icon Editor in preference to GBM if you click on a bitmap."
  Say "I would recommend removing its claims, so that GBM is the default."
  Say "If so, find the following items :-"
  Say "  \OS2\ICONEDIT.EXE file (perhaps by using Drives)."
  Say "  Icon Editor program reference object in OS/2 System -> Productivity."
  Say "  \OS2TK45\OS2BIN\ICONEDIT.EXE file (perhaps by using Drives)."
  Say "  Icon Editor program reference object in Toolkit 4.5->Development Tools."
  Say "  any other copies of ICONEDIT.EXE and program reference objects."
  Say "and then remove the following from the Settings -> Association page :-"
  Say "  Bitmap entry from Current types listbox."
  Say "  *.BMP entry from the Current names listbox."
end

Return 0

