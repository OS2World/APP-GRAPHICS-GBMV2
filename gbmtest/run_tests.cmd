@echo off

echo ----------------------------------------------------  > out\gbmtest.log
echo --- Running regression tests for GBM integration --- >> out\gbmtest.log
echo ---------------------------------------------------- >> out\gbmtest.log

call cleanup.cmd

REM *** BMP source format ***
for %%i in (ref\BMP\*.bmp) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** GIF source format ***
for %%i in (ref\GIF\*.gif) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** GIF source format (PNG Suite) ***
for %%i in (ref\PNG_GIF\*.gif) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** PNG source format (PNG Suite) ***
for %%i in (ref\PNG_GIF\*.png) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** APNG source format (APNG Patch of libpng) ***
for %%i in (ref\APNG\*.png) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** PBM, PGM, PPM, PNM source formats ***
for %%i in (ref\PxM\*.p?m) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** JPG source format ***
for %%i in (ref\JPG\*.jpg) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** JPEG2000 source format ***
for %%i in (ref\J2K\*) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** TIF source format ***
for %%i in (ref\TIF\*.tif) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** TGA source format ***
for %%i in (ref\TGA\*.tga) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** XPM source format ***
for %%i in (ref\XPM\*.xpm) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd

REM *** JBG source format ***
for %%i in (ref\JBG\*.jbg) do gbmtest %%i >> out\gbmtest.log 2>&1
call cleanup.cmd


