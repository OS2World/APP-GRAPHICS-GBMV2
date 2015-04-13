/***************************************************
* Mozilla GBM plug-in: GbmBitmapViewer_rc.h
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

#ifndef __NP_GBMBITMAPVIEWER_RC_H__
  #define __NP_GBMBITMAPVIEWER_RC_H__

/* ----------------------------- */
/* Language specific definitions */
/* ----------------------------- */

/* Language specific base offset */
#define RID_BASE_EN           700
#define RID_BASE_DE          1000
#define RID_BASE             1300

/* GBM Bitmap viewer window */
#define RID_GBMVIEWER_EN                 RID_BASE_EN
#define RID_GBMVIEWER_DE                 RID_BASE_DE

/* GBM Bitmap viewer dialogs */
#define RID_GBMVIEWER_FORMATINFO_EN      (RID_BASE_EN + 1)
#define RID_GBMVIEWER_FORMATINFO_DE      (RID_BASE_DE + 1)

/* GBM Bitmap preferences dialogs */
#define RID_GBMVIEWER_PREFS_EN           (RID_BASE_EN + 2)
#define RID_GBMVIEWER_PREFS_DE           (RID_BASE_DE + 2)

/* Popup menu definitions */
#define RID_GBMVIEWER_SMALL_POPUP_EN            (RID_BASE_EN + 10)
#define RID_GBMVIEWER_SMALL_POPUP_DE            (RID_BASE_DE + 10)
#define RID_GBMVIEWER_FULL_POPUP_EN             (RID_BASE_EN + 11)
#define RID_GBMVIEWER_FULL_POPUP_DE             (RID_BASE_DE + 11)

/* GBM Bitmap viewer GBM file dialog help table defines */
#define RID_GBMVIEWER_GBMDLG_HELPTABLE_EN       (RID_BASE_EN + 12)
#define RID_GBMVIEWER_GBMDLG_HELPTABLE_DE       (RID_BASE_DE + 12)

/* String definitions */
#define SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_EN (RID_BASE_EN + 13)
#define SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_DE (RID_BASE_DE + 13)

#define SID_GBMVIEWER_STRING_SAVEAS_EN          (RID_BASE_EN + 14)
#define SID_GBMVIEWER_STRING_SAVEAS_DE          (RID_BASE_DE + 14)

#define SID_GBMVIEWER_STRING_OVERRIDE_FILE_EN   (RID_BASE_EN + 15)
#define SID_GBMVIEWER_STRING_OVERRIDE_FILE_DE   (RID_BASE_DE + 15)

#define SID_GBMVIEWER_STRING_ALL_FILES_EN       (RID_BASE_EN + 16)
#define SID_GBMVIEWER_STRING_ALL_FILES_DE       (RID_BASE_DE + 16)

#define SID_GBMVIEWER_STRING_FMT_EXT_ERROR_EN   (RID_BASE_EN + 17)
#define SID_GBMVIEWER_STRING_FMT_EXT_ERROR_DE   (RID_BASE_DE + 17)

#define SID_GBMVIEWER_STRING_SCALE_SIMPLE_EN    (RID_BASE_EN + 18)
#define SID_GBMVIEWER_STRING_SCALE_SIMPLE_DE    (RID_BASE_DE + 18)

#define SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_EN (RID_BASE_EN + 19)
#define SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_DE (RID_BASE_DE + 19)

/* --------------------------------- */
/* Non-Language specific definitions */
/* --------------------------------- */

/* GBM Bitmap viewer file menu */
#define MID_FILE                        (RID_BASE + 10)
  #define MID_FILE_SAVE_AS              (RID_BASE + 11)

/* GBM Bitmap viewer Bitmap menu */
#define MID_BITMAP                      (RID_BASE + 20)
  #define MID_BITMAP_REF_HORZ           (RID_BASE + 21)
  #define MID_BITMAP_REF_VERT           (RID_BASE + 22)
  #define MID_BITMAP_TRANSPOSE          (RID_BASE + 23)
  #define MID_BITMAP_ROT_P90            (RID_BASE + 24)
  #define MID_BITMAP_ROT_M90            (RID_BASE + 25)

/* GBM Bitmap viewer View menu */
#define MID_VIEW                        (RID_BASE + 30)
  #define MID_VIEW_PAGE                 (RID_BASE + 31)
    #define MID_VIEW_PAGE_FIRST         (RID_BASE + 32)
    #define MID_VIEW_PAGE_PREV          (RID_BASE + 33)
    #define MID_VIEW_PAGE_NEXT          (RID_BASE + 34)
    #define MID_VIEW_PAGE_LAST          (RID_BASE + 35)
    #define MID_VIEW_ZOOM_DOWN          (RID_BASE + 36)
    #define MID_VIEW_ZOOM_UP            (RID_BASE + 37)
    #define MID_VIEW_ZOOM_NONE          (RID_BASE + 38)
    #define MID_VIEW_ZOOM_FITWIN        (RID_BASE + 39)
    #define MID_VIEW_ZOOM_FITWIDTH      (RID_BASE + 40)
  #define MID_VIEW_PREFERENCES          (RID_BASE + 41)

/* GBM Bitmap viewer help menu */
#define MID_HELP                        (RID_BASE + 50)
  #define MID_HELP_IMAGEINFO            (RID_BASE + 51)
  #define MID_HELP_ABOUT                (RID_BASE + 52)

/* GBM Bitmap viewer format info dialog text fields */
#define TID_FORMATINFO_FORMAT           (RID_BASE + 60)
#define TID_FORMATINFO_WIDTH            (RID_BASE + 61)
#define TID_FORMATINFO_HEIGHT           (RID_BASE + 62)
#define TID_FORMATINFO_BPP              (RID_BASE + 63)
#define TID_FORMATINFO_PAGES            (RID_BASE + 64)

/* GBM Bitmap viewer preferences dialog text fields */
#define TID_PREFERENCES_ALGO            (RID_BASE + 70)
#define TID_PREFERENCES_PAGESUD         (RID_BASE + 71)

#endif /* __NP_GBMBITMAPVIEWER_RC_H__ */


