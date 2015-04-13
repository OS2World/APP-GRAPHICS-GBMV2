/***************************************************
* Mozilla GBM plug-in: GbmSaveFileDialog_rc.h
*
* Copyright (C) 2006-2012 Heiko Nitzsche
*
*   This software is provided 'as-is', without any express or implied
*   warranty.  In no event will the author be held liable for any damages
*   arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose,
*   including commercial applications, and to alter it and redistribute it
*   freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not
*      claim that you wrote the original software. If you use this software
*      in a product, an acknowledgment in the product documentation would be
*      appreciated but is not required.
*   2. Altered source versions must be plainly marked as such, and must not be
*      misrepresented as being the original software.
*   3. This notice may not be removed or altered from any source distribution.
***************************************************/

#ifndef __GBMSAVEFILEDIALOG_RC_H__
  #define __GBMSAVEFILEDIALOG_RC_H__

/* ----------------------------- */
/* Language specific definitions */
/* ----------------------------- */

/* Language specific base offset */
#define RID_GBMDLG_BASE_EN          6000
#define RID_GBMDLG_BASE_DE          7000
#define RID_GBMDLG_BASE             8000

/* File Dialog IDs */
#define RID_GBM_FILEDLG_EN   RID_GBMDLG_BASE_EN
#define RID_GBM_FILEDLG_DE   RID_GBMDLG_BASE_DE


/* --------------------------------- */
/* Non-Language specific definitions */
/* --------------------------------- */

/*
gbmdlgrc.h - Resource ID's etc.

The HID_ help ID's must be decimal to keep IPFCPREP and IPFC happy.
A calling program using GBM dialog should not use Help ID's in the range
>= HID_GBM_MIN and < HID_GBM_MAX, as these are used below and are reserved for
use by GBM dialog. Only HID_GBM_FILEDLG and HID_GBM_SUPPORT will remain
unchanged. The others may change/disappear/be-added-to and should not be
referred to by :link. tags in application help files.

The DID_ control ID's are the additional controls on the GBM dialog.
If you provide your own subclass dialog procedure, you may wish to use these.
If you add your own additional controls, ensure they are >= DID_GBM_MAX.

If help is requested, then the help ID will always be for a mythical control
DID_GBM_FILEDLG. Thus the application need only supply the following help
tables.

HELPTABLE RID_HELP_TABLE
{
   HELPITEM RID_GBM_FILEDLG, RID_GBM_FILEDLG, HID_GBM_FILEDLG
   etc.
}

HELPSUBTABLE RID_GBM_FILEDLG
{
   HELPSUBITEM DID_GBM_FILEDLG, HID_GBM_FILEDLG
}

*/

#define    RID_GBM_FILEDLG          512

#define    DID_GBM_MIN             4096
#define    DID_GBM_FILEDLG         4096        /* Can safely refer to this */
#define    DID_GBM_OPTIONS_TXT     4097
#define    DID_GBM_OPTIONS_ED      4098

#define    DID_PREVIEW_GROUP_TXT   4100
#define    DID_PREVIEW_GROUP_ED    4101
#define    DID_PREVIEW_FORMAT_TXT  4102
#define    DID_PREVIEW_FORMAT_ED   4103
#define    DID_PREVIEW_WIDTH_TXT   4104
#define    DID_PREVIEW_WIDTH_ED    4105
#define    DID_PREVIEW_HEIGHT_TXT  4106
#define    DID_PREVIEW_HEIGHT_ED   4107
#define    DID_PREVIEW_DEPTH_TXT   4108
#define    DID_PREVIEW_DEPTH_ED    4109
#define    DID_PREVIEW_PAGES_TXT   4110
#define    DID_PREVIEW_PAGES_ED    4111
#define    DID_PREVIEW_RECT        4112

#define    DID_GBM_MAX             5120

#define    HID_GBM_MIN             2000
#define    HID_GBM_FILEDLG         2000        /* Can safely refer to this */
#define    HID_GBM_SUPPORT         2001        /* Can safely refer to this */
#define    HID_GBM_BITMAP          2002
#define    HID_GBM_GIF             2003
#define    HID_GBM_PCX             2004
#define    HID_GBM_TIFF            2005
#define    HID_GBM_TARGA           2006
#define    HID_GBM_ILBM            2007
#define    HID_GBM_YUV12C          2008
#define    HID_GBM_GREYMAP         2009
#define    HID_GBM_PIXMAP          2010
#define    HID_GBM_KIPS            2011
#define    HID_GBM_IAX             2012
#define    HID_GBM_XBITMAP         2013
#define    HID_GBM_SPRITE          2014
#define    HID_GBM_PSEG            2015
#define    HID_GBM_GEMRAS          2016
#define    HID_GBM_PORTRAIT        2017
#define    HID_GBM_JPEG            2018
#define    HID_GBM_PNG             2019
#define    HID_GBM_PBITMAP         2020
#define    HID_GBM_ANYMAP          2021
#define    HID_GBM_XPIXMAP         2022
#define    HID_GBM_OTHERS          2023
#define    HID_GBM_JPEG2000        2024
#define    HID_GBM_JBIG            2025
#define    HID_GBM_RAW             2026
#define    HID_GBM_MAX             3000

#endif /* __GBMSAVEFILEDIALOG_RC_H__ */

