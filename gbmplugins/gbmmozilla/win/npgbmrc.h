/***************************************************
* Mozilla GBM plug-in: npgbmrc.h
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

#ifndef __NP_GBMRC_H__
  #define __NP_GBMRC_H__

/* ----------------------------- */
/* Language specific definitions */
/* ----------------------------- */

/* Language specific base offset */
/* English:          700
 * German:          1000
 */

/* GBM Bitmap viewer dialogs */
#define RID_GBMVIEWER_FORMATINFO_EN                  701
#define RID_GBMVIEWER_FORMATINFO_DE                 1001

/* GBM Bitmap preferences dialogs */
#define RID_GBMVIEWER_PREFS_EN                       702
#define RID_GBMVIEWER_PREFS_DE                      1002

/* Popup menu definitions */
#define RID_GBMVIEWER_SMALL_POPUP_EN                 703
#define RID_GBMVIEWER_SMALL_POPUP_DE                1003
#define RID_GBMVIEWER_FULL_POPUP_EN                  704
#define RID_GBMVIEWER_FULL_POPUP_DE                 1004

/* String definitions */
#define SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_EN      705
#define SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_DE     1005

#define SID_GBMVIEWER_STRING_SAVEAS_EN               706
#define SID_GBMVIEWER_STRING_SAVEAS_DE              1006

#define SID_GBMVIEWER_STRING_ALL_FILES_EN            707
#define SID_GBMVIEWER_STRING_ALL_FILES_DE           1007

#define SID_GBMVIEWER_STRING_FMT_EXT_ERROR_EN        708
#define SID_GBMVIEWER_STRING_FMT_EXT_ERROR_DE       1008

#define SID_GBMVIEWER_STRING_SCALE_SIMPLE_EN         709
#define SID_GBMVIEWER_STRING_SCALE_SIMPLE_DE        1009

#define SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_EN   710
#define SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_DE  1010

/* --------------------------------- */
/* Non-Language specific definitions */
/* --------------------------------- */

/* GBM Bitmap viewer file menu */
#define MID_FILE_SAVE_AS              1311

/* GBM Bitmap viewer Bitmap menu */
#define MID_BITMAP_REF_HORZ           1321
#define MID_BITMAP_REF_VERT           1322
#define MID_BITMAP_TRANSPOSE          1323
#define MID_BITMAP_ROT_P90            1324
#define MID_BITMAP_ROT_M90            1325

/* GBM Bitmap viewer View menu */
#define MID_VIEW_PAGE_FIRST           1332
#define MID_VIEW_PAGE_PREV            1333
#define MID_VIEW_PAGE_NEXT            1334
#define MID_VIEW_PAGE_LAST            1335
#define MID_VIEW_ZOOM_DOWN            1336
#define MID_VIEW_ZOOM_UP              1337
#define MID_VIEW_ZOOM_NONE            1338
#define MID_VIEW_ZOOM_FITWIN          1339
#define MID_VIEW_ZOOM_FITWIDTH        1340
#define MID_VIEW_PREFERENCES          1341

/* GBM Bitmap viewer help menu */
#define MID_HELP_IMAGEINFO            1351
#define MID_HELP_ABOUT                1352

/* GBM Bitmap viewer format info dialog text fields */
#define TID_FORMATINFO_FORMAT         1360
#define TID_FORMATINFO_WIDTH          1361
#define TID_FORMATINFO_HEIGHT         1362
#define TID_FORMATINFO_BPP            1363
#define TID_FORMATINFO_PAGES          1364

/* GBM Bitmap viewer preferences dialog text fields */
#define TID_PREFERENCES_ALGO          1370
#define TID_PREFERENCES_PAGES         1371
#define TID_PREFERENCES_PAGESUD       1372

#endif /* __NP_GBMRC_H__ */

