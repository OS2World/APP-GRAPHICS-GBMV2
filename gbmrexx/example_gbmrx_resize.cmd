/*******************************************************************/

call RxFuncAdd 'GBM_LoadFuncs', 'GBMRX', 'GBM_LoadFuncs'
call GBM_LoadFuncs

say '> GBM.DLL   Version is 'GBM_Version()' <'
say '> GBMRX.DLL Version is 'GBM_VersionRexx()' <'
say imgReSize( 'images\example.gif', 'images\example1.png', 120 )
say imgReSize( 'images\example.gif', 'images\example2.png', 120, 120 )
say imgReSize( 'images\example.gif', 'images\example3.png',, 400 )
say imgReSize( 'images\example.gif', 'images\example4.png',, 400, 'aspect' )
say imgReSize( 'images\example.gif', 'images\example5.png' )
say imgReSize( 'images\example5.png',,, 600, 'Aspect', 'Lanczos' )

call GBM_DropFuncs
exit(0)

/*******************************************************************/

/* Load, Resize and Save a bitmap */
imgReSize: procedure expose fileIn fileOut w h algorithm header colors data
PARSE ARG fileIn, fileOut, w, h, aspect, algorithm

/* translate aspect to upper case */
aspect = TRANSLATE(aspect)

/* translate algorithm to lower case */
sourceChars = XRANGE('A','Z')" "XRANGE('a','z')
algorithm   = TRANSLATE(algorithm)
algorithm   = TRANSLATE(algorithm, XRANGE('a','z'), sourceChars)

if (fileIn  = '') then exit(1)
if (fileOut = '') then fileOut = fileIn

/* Get the bitmap */
if (0 <> GBM_FileHeader(fileIn, '', 'header.'))  then return 2
if (0 <> GBM_FilePalette(fileIn, '', 'colors.')) then return 2
if (0 <> GBM_FileData(fileIn, '', 'data'))       then return 2

/* interprete optional parameters */
if (WORDPOS('ASPECT', aspect) > 0) then
do
   if (w <> '') & (h  = '') then h = ( header.height * w ) / header.width
   if (w  = '') & (h <> '') then w = ( header.width  * h ) / header.height
end

if (w <> '') & (h  = '') then h = header.height
if (w  = '') & (h <> '') then w = header.width
if (algorithm = '') then algorithm = 'simple'

/* scale the bitmap */
if (w <> '') & (h <> '') then
do
  if (0 = GBM_ScaleIsSupported(algorithm, header.bpp, 'colors.')) then
  do
     /* check if output format supports 24bpp */
     if (0 <> GBM_FileType(fileOut, 'fileOutType.')) then
        return 4

     if (0 = GBM_IsBppSupported(WORD(fileOutType.extensions, 1), 24, 'w')) then
     do
        /* Requested scaler is not available and output format does */
        /* not support 24bpp, so switch to simple scaler. */
        algorithm = 'simple'
     end
     else
     do
        /* convert bitmap to 24bpp so that the scaler request can be fulfilled */
        if (0 <> GBM_PaletteDataTo24bpp('header.', 'colors.', 'data')) then
           return 3
     end
  end
  newSize.width  = w
  newSize.height = h
  if (0 <> GBM_Scale(algorithm, 'header.', 'colors.', 'data', 'newSize.')) then
     return 3
end

/* write the bitmap */
if (0 <> GBM_FileWrite(fileOut, '', 'header.', 'colors.', 'data')) then
   return 4

return 0


