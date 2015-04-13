/* ***** BEGIN LICENSE BLOCK *****
 * Version: CDDL 1.0/LGPL 2.1
 *
 * The contents of this file are subject to the COMMON DEVELOPMENT AND
 * DISTRIBUTION LICENSE (CDDL) Version 1.0 (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.sun.com/cddl/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is
 * Eugene Romanenko, netlabs.org.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the LGPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only under the
 * terms of the LGPL, and not to allow others to use your version of this file
 * under the terms of the CDDL, indicate your decision by deleting the
 * provisions above and replace them with the notice and other provisions
 * required by the LGPL. If you do not delete the provisions above, a recipient
 * may use your version of this file under the terms of any one of the CDDL
 * or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef lutypes_h
#define lutypes_h

//#include <os2.h>

struct LuRectangle
{
    double x1;
    double y1;
    double x2;
    double y2;
};


#define LU_LINK_TYPE_TITLE         0
#define LU_LINK_TYPE_PAGE          1
#define LU_LINK_TYPE_EXTERNAL_URI  2
#define LU_LINK_TYPE_EXTERNAL_FILE 3
#define LU_LINK_TYPE_LAUNCH        4


struct LuLink
{
    char *title;
    char *uri;
    long type;
    long page;
};


struct LuLinkMapping
{
    LuLink link;
    LuRectangle area;
};


#define LU_FONTEMBED_NOT_EMBEDDED       0
#define LU_FONTEMBED_EMBEDDED           1
#define LU_FONTEMBED_EMBEDDED_SUBSET    2

struct LuFontInfo
{
    char  *name;
    char  *type;
    short embedded;
};


#define LU_DOCUMENT_LAYOUT_SINGLE_PAGE       0
#define LU_DOCUMENT_LAYOUT_ONE_COLUMN        1
#define LU_DOCUMENT_LAYOUT_TWO_COLUMN_LEFT   2
#define LU_DOCUMENT_LAYOUT_TWO_COLUMN_RIGHT  3
#define LU_DOCUMENT_LAYOUT_TWO_PAGE_LEFT     4
#define LU_DOCUMENT_LAYOUT_TWO_PAGE_RIGHT    5


#define LU_DOCUMENT_MODE_NONE             0
#define LU_DOCUMENT_MODE_USE_OC           1
#define LU_DOCUMENT_MODE_USE_THUMBS       2
#define LU_DOCUMENT_MODE_FULL_SCREEN      3
#define LU_DOCUMENT_MODE_USE_ATTACHMENTS  4
#define LU_DOCUMENT_MODE_PRESENTATION     LU_DOCUMENT_MODE_FULL_SCREEN


#define LU_DOCUMENT_UI_HINT_HIDE_TOOLBAR       0x0001
#define LU_DOCUMENT_UI_HINT_HIDE_MENUBAR       0x0002
#define LU_DOCUMENT_UI_HINT_HIDE_WINDOWUI      0x0004
#define LU_DOCUMENT_UI_HINT_FIT_WINDOW         0x0008
#define LU_DOCUMENT_UI_HINT_CENTER_WINDOW      0x0010
#define LU_DOCUMENT_UI_HINT_DISPLAY_DOC_TITLE  0x0020
#define LU_DOCUMENT_UI_HINT_DIRECTION_RTL      0x0040



#define LU_DOCUMENT_PERMISSIONS_OK_TO_PRINT      0x0001
#define LU_DOCUMENT_PERMISSIONS_OK_TO_MODIFY     0x0002
#define LU_DOCUMENT_PERMISSIONS_OK_TO_COPY       0x0004
#define LU_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES  0x0008
#define LU_DOCUMENT_PERMISSIONS_FULL  (LU_DOCUMENT_PERMISSIONS_OK_TO_PRINT|LU_DOCUMENT_PERMISSIONS_OK_TO_MODIFY|LU_DOCUMENT_PERMISSIONS_OK_TO_COPY|LU_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES)


#define LU_DOCUMENT_INFO_TITLE          0x0001
#define LU_DOCUMENT_INFO_FORMAT         0x0002
#define LU_DOCUMENT_INFO_AUTHOR         0x0004
#define LU_DOCUMENT_INFO_SUBJECT        0x0008
#define LU_DOCUMENT_INFO_KEYWORDS       0x0010
#define LU_DOCUMENT_INFO_LAYOUT         0x0020
#define LU_DOCUMENT_INFO_CREATOR        0x0040
#define LU_DOCUMENT_INFO_PRODUCER       0x0080
#define LU_DOCUMENT_INFO_CREATION_DATE  0x0100
#define LU_DOCUMENT_INFO_MOD_DATE       0x0200
#define LU_DOCUMENT_INFO_LINEARIZED     0x0400
#define LU_DOCUMENT_INFO_START_MODE     0x0800
#define LU_DOCUMENT_INFO_UI_HINTS       0x1000
#define LU_DOCUMENT_INFO_PERMISSIONS    0x2000
#define LU_DOCUMENT_INFO_N_PAGES        0x4000
#define LU_DOCUMENT_INFO_SECURITY       0x8000


struct LuDocumentInfo
{
    char *title;
    char *format;
    char *author;
    char *subject;
    char *keywords;
    char *creator;
    char *producer;
    long security;
    long linearized;
    unsigned long creation_date;
    unsigned long modified_date;
    unsigned long layout;
    unsigned long mode;
    unsigned long ui_hints;
    unsigned long permissions;
    long n_pages;

    // Mask of all valid fields
    unsigned long fields_mask;
};

#ifndef __SOMIDL__
typedef long (_System *_asynchCallbackFn)( void *data );
//typedef long (EXPENTRY *_asynchCallbackFn)( void *data );
//typedef _asynchCallbackFn *asynchCallbackFn;
#endif

// Load errors
#define LU_LDERR_NO_ERROR       0
#define LU_LDERR_CUSTOM         1
#define LU_LDERR_OUT_OF_MEMORY  2
#define LU_LDERR_OPEN_ERROR     3
#define LU_LDERR_READ_ERROR     4
#define LU_LDERR_DAMAGED        5
#define LU_LDERR_WRONG_FORMAT   6
#define LU_LDERR_ENCRYPTED      7

// Render errors/warnings
#define LU_RERR_NO_ERROR              0
#define LU_RERR_CUSTOM                1
#define LU_RERR_OUT_OF_MEMORY         2
#define LU_RERR_CORRUPTED_PAGE_DATA   3
#define LU_RERR_FONT_NOT_FOUND        4

//
// ***  Structs to detect type of file by it's content ***
//

// one signature to check
struct LuSignature
{
    unsigned long  offset; // offset to read data
    unsigned long  origin; // offset: 0 - from beginning of file, 1 - from end
    unsigned long  length; // length of data bytes to check
    void          *data;   // pointer to data bytes
};

// list of signatures to check
// all signatures must be checked for positive result
struct LuSignatureList
{
    unsigned long  count;       // number of LuSignature structures
    LuSignature   *signatures;  // array of LuSignature structures
};

// list of checklists
// at least one checklist must be checked for positive result
struct LuSignatureCheck
{
    unsigned long    count;   // number of LuSignatureList structures
    LuSignatureList *slists;  // array of LuSignatureList structures
};

#endif // lutypes_h
