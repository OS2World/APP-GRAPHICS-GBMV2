/*******************************************************************/
/*                                                                 */
/* This program calls the various REXX external functions          */
/* provided in the gbmrx.c function package.                       */
/*                                                                 */
/*******************************************************************/
/* '@echo off' */

call RxFuncAdd 'GBM_LoadFuncs', 'GBMRX', 'GBM_LoadFuncs'
call GBM_LoadFuncs

/* Set the filename used in this example */
readFilename  = "images\example.gif"
readOptions   = "index=5,ext_bpp"   /* allow deep colour depth > 24bpp */

writeFilename = "images\example_out.jpg"
writeOptions  = "quality=85"

/* Get version number of GBMRX lib ***/
gbmVersionRexx = GBM_VersionRexx()
say 'GBMRX.DLL version: 'gbmVersionRexx

/* Get version number of GBM.DLL ***/
gbmVersion = GBM_Version()
say 'GBM.DLL   version: 'gbmVersion

/* Get the supported types with all descriptions */
rc = GBM_Types('types.')
if rc <> "0" then
do
   say
   say '### Cannot query supported bitmap types.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say
   say '======================'
   say 'Number of supported formats: 'types.0
   say '----------------------'
   do i = 1 to types.0
      say '  Extensions: 'types.i.extensions
      say '  Short desc: 'types.i.shortname
      say '  Long  desc: 'types.i.longname
      if i < types.0 then
      do
        say '----------------------'
      end
   end
end

/* ----------------------------------------------- */
say
say '======================'
say '======================'
say 'Checking file: "'readFilename'"'

/* Get file type */
rc = GBM_FileType(readFilename, 'type.')
if rc <> "0" then
do
   say
   say '### Cannot detect bitmap type.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   say '  Extensions: 'type.extensions
   say '  Short desc: 'type.shortname
   say '  Long  desc: 'type.longname
end

/* Get number of pages */
numPages = GBM_FilePages(readFilename)
say '----------------------'
say 'Number of pages: 'numPages

/* Get bitmap header */
rc = GBM_FileHeader(readFilename, readOptions, 'header.')
if rc <> "0" then
do
   say
   say '### Cannot read bitmap header.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   say '  Bitmap width : 'header.width
   say '  Bitmap height: 'header.height
   say '  Bitmap bpp   : 'header.bpp' bpp'
end

/* Get bitmap palette */
rc = GBM_FilePalette(readFilename, readOptions, 'colors.')
if rc <> "0" then
do
   say
   say '### Cannot read bitmap palette.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   /* Print palette entries. Note the palette can be empty for true color bitmaps. */
   say
   say '======================'
   say 'Number of palette entries: 'colors.0
   say '----------------------'
   do i = 1 to colors.0
      say '  Index, (R,G,B): 'i', ('colors.i.red','colors.i.green','colors.i.blue')'
   end
end

/* Get bitmap data */
say
say '======================'
say 'Reading bitmap data'

rc = GBM_FileData(readFilename, readOptions, 'data')
if rc <> "0" then
do
   say
   say '### Cannot read bitmap data.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   expectedLength = (TRUNC((header.width * header.bpp + 31)/32)*4)*header.height
   say '  Bitmap data length (expected): 'expectedLength
   say '  Bitmap data length (read)    : 'LENGTH(data)
end

/* ----------------------------------------------- */

/* Get the supported scaling algorithms */
scalerIndex = -1
rc = GBM_ScaleAlgorithms('scaleAlgorithms.')
if rc <> "0" then
do
   say
   say '### Cannot query supported scaling algorithms.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say
   say '======================'
   say 'Number of supported algorithms: 'scaleAlgorithms.0
   say '----------------------'
   do i = 1 to scaleAlgorithms.0
      say '  Algorithm: 'scaleAlgorithms.i.algorithm
      if scaleAlgorithms.i.algorithm = "lanczos" then
      do
         scalerIndex = i
      end
   end
end

if scalerIndex = -1 then
do
   say
   say '### No scaling algorithm found'
   call GBM_DropFuncs
   exit(1)
end

/* check if scaling is supported */
say
say '======================'
say 'Checking if scaling is supported'
say '----------------------'
rc = GBM_ScaleIsSupported(scaleAlgorithms.scalerIndex.algorithm, header.bpp, 'colors.')
if rc = "0" then
do
   say 'Scaling is not supported for this color depth with algorithm: 'scaleAlgorithms.scalerIndex.algorithm
   say 'Expanding bitmap to 24bpp'

   rc = GBM_PaletteDataTo24bpp('header.', 'colors.', 'data')
   if rc <> "0" then
   do
       say '### Color depth conversion failed'
       call GBM_DropFuncs
       exit(1)
   end
   expectedLength = (TRUNC((header.width * header.bpp + 31)/32)*4)*header.height
   say '  Bitmap data length (expected): 'expectedLength
   say '  Bitmap data length (read)    : 'LENGTH(data)
end
else
do
   say 'SUCCESS'
end

/* Scale bitmap */
say
say '======================'
say 'Scaling bitmap data with algorithm "'scaleAlgorithms.scalerIndex.algorithm'"'

newSize.width  = header.width  * 3
newSize.height = header.height * 3
rc = GBM_Scale(scaleAlgorithms.scalerIndex.algorithm, 'header.', 'colors.', 'data', 'newSize.')
if rc <> "0" then
do
   say
   say '### Cannot scale bitmap'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   say 'New width : 'header.width
   say 'New height: 'header.height
end

/* ----------------------------------------------- */
say
say '======================'
say 'Reflect the bitmap horizontal'
rc = GBM_Reflect('horizontal', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot reflect bitmap horizontal'
   say rc
   call GBM_DropFuncs
   exit(1)
end

say 'Transpose the bitmap'
rc = GBM_Reflect('transpose', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot transpose bitmap'
   say rc
   call GBM_DropFuncs
   exit(1)
end

say 'Reflect the bitmap vertical'
rc = GBM_Reflect('vertical', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot reflect bitmap vertical'
   say rc
   call GBM_DropFuncs
   exit(1)
end

/* ----------------------------------------------- */
say
say '======================'
say 'Rotate the bitmap by 90 degree'
rc = GBM_Rotate('90', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot rotate bitmap by 90 degree'
   say rc
   call GBM_DropFuncs
   exit(1)
end

say 'Rotate the bitmap by 270 degree'
rc = GBM_Rotate('270', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot rotate bitmap by 270 degree'
   say rc
   call GBM_DropFuncs
   exit(1)
end

say 'Rotate the bitmap by 180 degree'
rc = GBM_Rotate('180', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot rotate bitmap by 180 degree'
   say rc
   call GBM_DropFuncs
   exit(1)
end

say 'Rotate the bitmap by -90 degree'
rc = GBM_Rotate('-90', 'header.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot rotate bitmap by -90 degree'
   say rc
   call GBM_DropFuncs
   exit(1)
end

/* ----------------------------------------------- */
say
say '======================'
say 'Writing bitmap to file: "'writeFilename'"'

/* Get output file type */
rc = GBM_FileType(writeFilename, 'typeout.')
if rc <> "0" then
do
   say
   say '### Cannot detect output bitmap type.'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   say '  Extensions: 'typeout.extensions
   say '  Short desc: 'typeout.shortname
   say '  Long  desc: 'typeout.longname
end

/* check if output colour depth is support by the write format */

rc = GBM_IsBppSupported(WORD(typeout.extensions, 1), header.bpp, 'w')
if rc = "0" then
do
   say
   say '### Colour depth is not supported by output format'
   call GBM_DropFuncs
   exit(1)
end

/* Write new bitmap in other format */

rc = GBM_FileWrite(writeFilename, writeOptions, 'header.', 'colors.', 'data')
if rc <> "0" then
do
   say
   say '### Cannot write bitmap "'writeFilename'".'
   say rc
   call GBM_DropFuncs
   exit(1)
end
else
do
   say '----------------------'
   say '  SUCCESS'
end

say
say 'All is DONE.'

call GBM_DropFuncs
exit(0)


