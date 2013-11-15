#!/bin/sh

#set -x

echo "----------------------------------------------------"  > out/gbmtest.log
echo "--- Running regression tests for GBM integration ---" >> out/gbmtest.log
echo "----------------------------------------------------" >> out/gbmtest.log

cleanup.sh

# *** BMP source format ***
for i in `ls ref/BMP/*.bmp` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** GIF source format ***
for i in `ls ref/GIF/*.gif` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** GIF source format (PNG Suite) ***
for i in `ls ref/PNG_GIF/*.gif` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** PNG source format (PNG Suite) ***
for i in `ls ref/PNG_GIF/*.png` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** APNG source format (APNG Patch of libpng) ***
for i in `ls ref/APNG/*.png` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** PBM, PGM, PPM, PNM source formats ***
for i in `ls ref/PxM/*.p?m` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** JPG source format ***
for i in `ls ref/JPG/*.jpg` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** JPEG2000 source format ***
for i in `ls ref/J2K/*` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** TIF source format ***
for i in `ls ref/TIF/*.tif` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** TGA source format ***
for i in `ls ref/TGA/*.tga` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** XPM source format ***
for i in `ls ref/XPM/*.xpm` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh

# *** JBG source format ***
for i in `ls ref/JBG/*.jbg` ; do gbmtest $i >> out/gbmtest.log ; done 2>&1
cleanup.sh


