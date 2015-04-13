/*

scroll.c - Automatic scroller overview control

*/

#define	INCL_DOS
#define	INCL_WIN
#define	INCL_GPI
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#define	_SCROLL_
#include "scroll.h"

#ifndef max
 #define max(a,b)  ((a) > (b) ? (a) : (b))
#endif

/*...vscroll\46\h:0:*/

typedef struct /* sc */
	{
	ULONG flStyle;			/* Window style supplied at create   */
	SIZEL sizlScroll;		/* Our size                          */
	RECTL rclWindow;		/* The window part of scroller       */
	SIZEL sizlChild;		/* Size of child window              */
	HWND hwndChild;			/* Thing to scroll over (or NULL)    */
	HWND hwndHScroll;		/* Handle to horizontal scrollbar    */
	HWND hwndVScroll;		/* Handle to vertical scrollbar      */
	HWND hwndCorner;		/* The little bottom right corner    */
	SHORT xPosn, cxRange, cxWindow;	/* X slider (if hwndChild not NULL)  */
	SHORT yPosn, cyRange, cyWindow;	/* Y slider (if hwndChild not NULL)  */
	SCROLLCOLORS sccols;		/* Colors to use for non-convered    */
	} SCROLL;

/*...sScrollWndProc:0:*/
/*...sCalcHScroll    \45\ calculate window size and scrollbar range:0:*/
static VOID CalcHScroll(SCROLL *psc)
	{
	psc -> cxWindow = (SHORT) (psc -> rclWindow.xRight - psc -> rclWindow.xLeft);
	psc -> cxRange  = max(0, (SHORT) (psc -> sizlChild.cx - psc -> cxWindow));
	}
/*...e*/
/*...sCalcVScroll:0:*/
static VOID CalcVScroll(SCROLL *psc)
	{
	psc -> cyWindow = (SHORT) (psc -> rclWindow.yTop - psc -> rclWindow.yBottom);
	psc -> cyRange  = max(0, (SHORT) (psc -> sizlChild.cy - psc -> cyWindow));
	}
/*...e*/
/*...sDisableHScroll \45\ make scrollbar unusable \40\used when hwndChild \61\\61\ NULL\41\:0:*/
static VOID DisableHScroll(SCROLL *psc)
	{
	WinSendMsg(psc -> hwndHScroll, SBM_SETSCROLLBAR,
		   MPFROMSHORT(0), MPFROM2SHORT(0, 0));
	WinSendMsg(psc -> hwndHScroll, SBM_SETTHUMBSIZE,
		   MPFROM2SHORT(1, 1), NULL);
	}
/*...e*/
/*...sDisableVScroll:0:*/
static VOID DisableVScroll(SCROLL *psc)
	{
	WinSendMsg(psc -> hwndVScroll, SBM_SETSCROLLBAR,
		   MPFROMSHORT(0), MPFROM2SHORT(0, 0));
	WinSendMsg(psc -> hwndVScroll, SBM_SETTHUMBSIZE,
		   MPFROM2SHORT(1, 1), NULL);
	}
/*...e*/
/*...sUpdateHScroll  \45\ update a scrollbar \40\used when hwndChild \33\\61\ NULL\41\:0:*/
static VOID UpdateHScroll(SCROLL *psc)
	{
	WinSendMsg(psc -> hwndHScroll, SBM_SETSCROLLBAR,
		   MPFROMSHORT(psc -> xPosn), MPFROM2SHORT(0, psc -> cxRange));
	WinSendMsg(psc -> hwndHScroll, SBM_SETTHUMBSIZE,
		   MPFROM2SHORT(psc -> cxWindow, psc -> sizlChild.cx), NULL);
	}
/*...e*/
/*...sUpdateVScroll:0:*/
static VOID UpdateVScroll(SCROLL *psc)
	{
	WinSendMsg(psc -> hwndVScroll, SBM_SETSCROLLBAR,
		   MPFROMSHORT(psc -> yPosn), MPFROM2SHORT(0, psc -> cyRange));
	WinSendMsg(psc -> hwndVScroll, SBM_SETTHUMBSIZE,
		   MPFROM2SHORT(psc -> cyWindow, psc -> sizlChild.cy), NULL);
	}
/*...e*/
/*...sKeepHScroll    \45\ adjust posn to keep roughly in same place:0:*/
static VOID KeepHScroll(SCROLL *psc)
	{
	SHORT cxRangeOld = SHORT2FROMMR(WinSendMsg(psc -> hwndHScroll,
					SBM_QUERYRANGE, NULL, NULL));

	psc -> xPosn = ( cxRangeOld != 0 ) ?
		(SHORT) ((psc -> xPosn * (LONG) psc -> cxRange) / cxRangeOld) : 0;
	}
/*...e*/
/*...sKeepVScroll:0:*/
static VOID KeepVScroll(SCROLL *psc)
	{
	SHORT cyRangeOld = SHORT2FROMMR(WinSendMsg(psc -> hwndVScroll,
					SBM_QUERYRANGE, NULL, NULL));

	psc -> yPosn = ( cyRangeOld != 0 ) ?
		(SHORT) ((psc -> yPosn * (LONG) psc -> cyRange) / cyRangeOld) : 0;
	}
/*...e*/
/*...sMoveChild      \45\ place child according to posn:0:*/
/*
Only called when hwndChild != NULL.
*/

static VOID MoveChild(SCROLL *psc)
	{
	SHORT xPosn    = (SHORT) (psc -> rclWindow.xLeft   - psc -> xPosn);
	SHORT yPosn    = (SHORT) (psc -> rclWindow.yBottom - (psc -> cyRange - psc -> yPosn));
	SHORT cxMargin = (SHORT) (psc -> cxWindow - psc -> sizlChild.cx);
	SHORT cyMargin = (SHORT) (psc -> cyWindow - psc -> sizlChild.cy);

	if ( (psc -> flStyle & SCS_HCENTRE) != 0 && cxMargin > 0 )
		xPosn += (cxMargin >> 1);
	if ( (psc -> flStyle & SCS_VCENTRE) != 0 && cyMargin > 0 )
		yPosn += (cyMargin >> 1);

	WinSetWindowPos(psc -> hwndChild, (HWND) NULL, xPosn, yPosn, 0, 0, SWP_MOVE);
	WinUpdateWindow(psc -> hwndChild);
	}
/*...e*/

/*...sNotify         \45\ inform owner of something:0:*/
static MRESULT Notify(HWND hwnd, USHORT usNotification, MPARAM mp2)
	{
	HWND hwndOwner = WinQueryWindow(hwnd, QW_OWNER);
	USHORT usId    = WinQueryWindowUShort(hwnd, QWS_ID);

	return ( WinSendMsg(hwndOwner,
			    WM_CONTROL,
			    MPFROM2SHORT(usId, usNotification),
			    mp2
			    ) );
	}
/*...e*/
/*...sPageWidth      \45\ get owner to give us a left\47\right page step:0:*/
static SHORT PageWidth(HWND hwnd, SCROLL *psc)
	{
	SHORT cxPage = ( psc -> flStyle & SCS_HPAGE ) ?
		SHORT1FROMMR(Notify(hwnd, SCN_HPAGE, MPFROM2SHORT(psc -> cxRange, psc -> cxWindow))) :
		psc -> cxRange / 10;

	return ( max(1, cxPage) );
	}
/*...e*/
/*...sPageHeight     \45\ get owner to give us an up\47\down page step:0:*/
static SHORT PageHeight(HWND hwnd, SCROLL *psc)
	{
	SHORT cyPage = ( psc -> flStyle & SCS_VPAGE ) ?
		SHORT1FROMMR(Notify(hwnd, SCN_VPAGE, MPFROM2SHORT(psc -> cyRange, psc -> cyWindow))) :
		psc -> cyRange / 10;

	return ( max(1, cyPage) );
	}
/*...e*/

MRESULT _System ScrollWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
	{
	SCROLL *psc = WinQueryWindowPtr(hwnd, 0);

	switch ( (int) msg )
		{
/*...sWM_CREATE       \45\ create window and scroll bars:16:*/
case WM_CREATE:
	{
	SHORT cxScrollBar = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
	SHORT cyScrollBar = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);
	CREATESTRUCT *pcrst = PVOIDFROMMP(mp2);

	if ( (psc = (SCROLL *) malloc(sizeof(SCROLL))) == NULL )
		return ( (MRESULT) 1 );

	psc -> sizlScroll.cx = pcrst -> cx;
	psc -> sizlScroll.cy = pcrst -> cy;
	psc -> flStyle         = pcrst -> flStyle;
	psc -> hwndChild       = (HWND) NULL;

	switch ( (int) (psc -> flStyle & (SCS_HSCROLL|SCS_VSCROLL)) )
		{
/*...sboth bars:32:*/
case (SCS_HSCROLL|SCS_VSCROLL):
	psc -> hwndHScroll = WinCreateWindow(hwnd, WC_SCROLLBAR, NULL, WS_VISIBLE|SBS_HORZ|SBS_THUMBSIZE,
		0, 0, pcrst -> cx - cxScrollBar, cyScrollBar,
		hwnd, HWND_BOTTOM, SCID_HSCROLL, NULL, NULL);
	psc -> hwndVScroll = WinCreateWindow(hwnd, WC_SCROLLBAR, NULL, WS_VISIBLE|SBS_VERT|SBS_THUMBSIZE,
		pcrst -> cx - cxScrollBar, cyScrollBar, cxScrollBar, pcrst -> cy - cyScrollBar,
		hwnd, HWND_BOTTOM, SCID_VSCROLL, NULL, NULL);
	psc -> hwndCorner = WinCreateWindow(hwnd, WC_STATIC, NULL, WS_VISIBLE|SS_BKGNDRECT,
		pcrst -> cx - cxScrollBar, 0, cxScrollBar, cyScrollBar,
		hwnd, HWND_BOTTOM, SCID_SQUARE, NULL, NULL);
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = pcrst -> cx - cxScrollBar;
	psc -> rclWindow.yBottom = cyScrollBar;
	psc -> rclWindow.yTop    = pcrst -> cy;
	break;
/*...e*/
/*...sjust horizontal bar:32:*/
case SCS_HSCROLL:
	psc -> hwndHScroll = WinCreateWindow(hwnd, WC_SCROLLBAR, "", WS_VISIBLE|SBS_HORZ|SBS_THUMBSIZE,
		0, 0, pcrst -> cx, cyScrollBar,
		hwnd, HWND_BOTTOM, SCID_HSCROLL, NULL, NULL);
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = pcrst -> cx;
	psc -> rclWindow.yBottom = cyScrollBar;
	psc -> rclWindow.yTop    = pcrst -> cy;
	break;
/*...e*/
/*...sjust vertical bar:32:*/
case SCS_VSCROLL:
	psc -> hwndVScroll = WinCreateWindow(hwnd, WC_SCROLLBAR, "", WS_VISIBLE|SBS_VERT|SBS_THUMBSIZE,
		pcrst -> cx - cxScrollBar, 0, cxScrollBar, pcrst -> cy,
		hwnd, HWND_BOTTOM, SCID_VSCROLL, NULL, NULL);
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = pcrst -> cx - cxScrollBar;
	psc -> rclWindow.yBottom = 0;
	psc -> rclWindow.yTop    = pcrst -> cy;
	break;
/*...e*/
/*...sneither:32:*/
case 0:
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = pcrst -> cx;
	psc -> rclWindow.yBottom = 0;
	psc -> rclWindow.yTop    = pcrst -> cy;
	break;
/*...e*/
		}

	if ( psc -> flStyle & SCS_HSCROLL )
		DisableHScroll(psc);

	if ( psc -> flStyle & SCS_VSCROLL )
		DisableVScroll(psc);

	psc -> sccols.lColorBlank = CLR_DARKGRAY; /* CLR_BLACK, CLR_DARKGRAY, CLR_PALEGRAY */

	WinSetWindowPtr(hwnd, 0, psc);

	return ( (MRESULT) 0 );
	}
/*...e*/
/*...sWM_DESTROY      \45\ destroy window:16:*/
case WM_DESTROY:
	free((void *) psc);
	return ( (MRESULT) 0 );
/*...e*/
/*...sWM_PAINT        \45\ paint background:16:*/
case WM_PAINT:
	{
	POINTL pt1, pt2;
	RECTL rclUpdate;
	HPS hps = WinBeginPaint(hwnd, (HPS) NULL, &rclUpdate);
	pt1.x = rclUpdate.xLeft;
	pt1.y = rclUpdate.yBottom;
	pt2.x = rclUpdate.xRight;
	pt2.y = rclUpdate.yTop;
	GpiSetColor(hps, psc -> sccols.lColorBlank);
	GpiMove(hps, &pt1);
	GpiBox(hps, DRO_FILL, &pt2, 0L, 0L);

	WinEndPaint(hps);

	return ( (MRESULT) 0 );
	}
/*...e*/
/*...sWM_SIZE         \45\ window size changed:16:*/
case WM_SIZE:
	{
	SWP swp;
	SHORT cxScrollBar = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
	SHORT cyScrollBar = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);

	WinQueryWindowPos(hwnd, &swp);
	psc -> sizlScroll.cx = swp.cx;
	psc -> sizlScroll.cy = swp.cy;

	/* Resize all the component windows */

	switch ( (int) (psc -> flStyle & (SCS_HSCROLL|SCS_VSCROLL)) )
		{
/*...sboth bars:32:*/
case (SCS_HSCROLL|SCS_VSCROLL):
	WinSetWindowPos(psc -> hwndHScroll, (HWND) NULL,
			0, 0, swp.cx - cxScrollBar, cyScrollBar,
			SWP_MOVE | SWP_SIZE);
	WinSetWindowPos(psc -> hwndVScroll, (HWND) NULL,
			swp.cx - cxScrollBar, cyScrollBar, cxScrollBar, swp.cy - cyScrollBar,
			SWP_MOVE | SWP_SIZE);
	WinSetWindowPos(psc -> hwndCorner, (HWND) NULL,
			swp.cx - cxScrollBar, 0, cxScrollBar, cyScrollBar,
			SWP_MOVE | SWP_SIZE);

	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = swp.cx - cxScrollBar;
	psc -> rclWindow.yBottom = cyScrollBar;
	psc -> rclWindow.yTop    = swp.cy;
	break;
/*...e*/
/*...sjust horizontal bar:32:*/
case SCS_HSCROLL:
	WinSetWindowPos(psc -> hwndHScroll, (HWND) NULL,
			0, 0, swp.cx, cyScrollBar,
			SWP_MOVE | SWP_SIZE);
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = swp.cx;
	psc -> rclWindow.yBottom = cyScrollBar;
	psc -> rclWindow.yTop    = swp.cy;
	break;
/*...e*/
/*...sjust vertical bar:32:*/
case SCS_VSCROLL:
	WinSetWindowPos(psc -> hwndVScroll, (HWND) NULL,
			swp.cx - cxScrollBar, 0, cxScrollBar, swp.cy,
			SWP_MOVE | SWP_SIZE);
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = swp.cx - cxScrollBar;
	psc -> rclWindow.yBottom = 0;
	psc -> rclWindow.yTop    = swp.cy;
	break;
/*...e*/
/*...sneither:32:*/
case 0:
	psc -> rclWindow.xLeft   = 0;
	psc -> rclWindow.xRight  = swp.cx;
	psc -> rclWindow.yBottom = 0;
	psc -> rclWindow.yTop    = swp.cy;
	break;
/*...e*/
		}

	/* Try to keep showing the same part of the child */

	if ( psc -> hwndChild != (HWND) NULL )
		{
		if ( psc -> flStyle & SCS_HSCROLL )
			{
			CalcHScroll(psc);
			KeepHScroll(psc);
			UpdateHScroll(psc);
			}
	
		if ( psc -> flStyle & SCS_VSCROLL )
			{
			CalcVScroll(psc);
			KeepVScroll(psc);
			UpdateVScroll(psc);
			}

		MoveChild(psc);
		}

	/* Else scrollbar(s) will already be disabled */

	}
	break;
/*...e*/
/*...sWM_HSCROLL      \45\ horizontal scrollbar moved:16:*/
case WM_HSCROLL:
	if ( psc -> hwndChild != (HWND) NULL && (psc -> flStyle & SCS_HSCROLL) != 0 )
		{
		SHORT xPosnOld = psc -> xPosn;
	
		switch ( SHORT2FROMMP(mp2) )
			{
			case SB_LINELEFT:	psc -> xPosn--;
						break;
			case SB_PAGELEFT:	psc -> xPosn -= PageWidth(hwnd, psc);
						break;
			case SB_SLIDERPOSITION:	if ( psc -> flStyle & SCS_HSLOW )
							psc -> xPosn = SHORT1FROMMP(mp2);
						break;
			case SB_SLIDERTRACK:	if ( (psc -> flStyle & SCS_HSLOW) == 0 )
							psc -> xPosn = SHORT1FROMMP(mp2);
						break;
			case SB_PAGERIGHT:	psc -> xPosn += PageWidth(hwnd, psc);
						break;
			case SB_LINERIGHT:	psc -> xPosn++;	
						break;
			}
	
		if ( psc -> xPosn < 0 )
			psc -> xPosn = 0;
		else if ( psc -> xPosn > psc -> cxRange )
			psc -> xPosn = psc -> cxRange;
	
		if ( psc -> xPosn != xPosnOld )
			{
			UpdateHScroll(psc);
			MoveChild(psc);
			}
		}
	break;
/*...e*/
/*...sWM_VSCROLL      \45\ vertical scrollbar moved:16:*/
case WM_VSCROLL:
	if ( psc -> hwndChild != (HWND) NULL && (psc -> flStyle & SCS_VSCROLL) != 0 )
		{
		SHORT yPosnOld = psc -> yPosn;
	
		switch ( SHORT2FROMMP(mp2) )
			{
			case SB_LINELEFT:	psc -> yPosn--;
						break;
			case SB_PAGELEFT:	psc -> yPosn -= PageHeight(hwnd, psc);
						break;
			case SB_SLIDERPOSITION:	if ( psc -> flStyle & SCS_VSLOW )
							psc -> yPosn = SHORT1FROMMP(mp2);
						break;
			case SB_SLIDERTRACK:	if ( (psc -> flStyle & SCS_VSLOW) == 0 )
							psc -> yPosn = SHORT1FROMMP(mp2);
						break;
			case SB_PAGERIGHT:	psc -> yPosn += PageHeight(hwnd, psc);
						break;
			case SB_LINERIGHT:	psc -> yPosn++;	
						break;
			}
	
		if ( psc -> yPosn < 0 )
			psc -> yPosn = 0;
		else if ( psc -> yPosn > psc -> cyRange )
			psc -> yPosn = psc -> cyRange;
	
		if ( psc -> yPosn != yPosnOld )
			{
			UpdateVScroll(psc);
			MoveChild(psc);
			}
		}
	break;
/*...e*/

		/* Scroll specific messages SCM_* */

/*...sSCM_CHILD       \45\ child window has changed:16:*/
case SCM_CHILD:
	{
	if ( (psc -> hwndChild = HWNDFROMMP(mp1)) != (HWND) NULL )
		{
		SWP swp;

		WinQueryWindowPos(psc -> hwndChild, &swp);
		psc -> sizlChild.cx = swp.cx;
		psc -> sizlChild.cy = swp.cy;
		psc -> xPosn        = 0;
		psc -> yPosn        = 0;

		if ( psc -> flStyle & SCS_HSCROLL )
			{
			CalcHScroll(psc);
			UpdateHScroll(psc);
			}

		if ( psc -> flStyle & SCS_VSCROLL )
			{
			CalcVScroll(psc);
			UpdateVScroll(psc);
			}

		MoveChild(psc);
		}
	else
		{
		if ( psc -> flStyle & SCS_HSCROLL )
			DisableHScroll(psc);

		if ( psc -> flStyle & SCS_VSCROLL )
			DisableVScroll(psc);
		}

	return ( (MRESULT) 0 );
	}
/*...e*/
/*...sSCM_SIZE        \45\ size of child window has changed:16:*/
case SCM_SIZE:
	{
	if ( psc -> hwndChild != (HWND) NULL )
		{
		SWP swp;
	
		WinQueryWindowPos(psc -> hwndChild, &swp);
		psc -> sizlChild.cx = swp.cx;
		psc -> sizlChild.cy = swp.cy;
	
		if ( psc -> flStyle & SCS_HSCROLL )
			{
			CalcHScroll(psc);
			KeepHScroll(psc);
			UpdateHScroll(psc);
			}
	
		if ( psc -> flStyle & SCS_VSCROLL )
			{
			CalcVScroll(psc);
			KeepVScroll(psc);
			UpdateVScroll(psc);
			}
	
		MoveChild(psc);
		}

	return ( (MRESULT) 0 );
	}
/*...e*/
/*...sSCM_SETCOLORS   \45\ set colors:16:*/
case SCM_SETCOLORS:
	{
	SCROLLCOLORS *psccols = PVOIDFROMMP(mp1);

	psc -> sccols = *psccols;

	WinInvalidateRect(hwnd, NULL, TRUE);

	return ( (MRESULT) 0 );
	}
/*...e*/
/*...sSCM_QUERYCOLORS \45\ query colors:16:*/
case SCM_QUERYCOLORS:
	{
	SCROLLCOLORS *psccols = PVOIDFROMMP(mp1);

	*psccols = psc -> sccols;
	return ( (MRESULT) 0 );
	}
/*...e*/
		}
	return ( WinDefWindowProc(hwnd, msg, mp1, mp2) );
	}
/*...e*/
/*...sRegisterScrollClass:0:*/
BOOL RegisterScrollClass(HAB hab)
	{
	return WinRegisterClass(
		hab,			/* Handle of programs anchor block   */
		WC_SCROLL,		/* Name of window class              */
		ScrollWndProc,		/* Window procedure for scroller     */
		CS_SIZEREDRAW |		/* Redraw if resized                 */
		CS_CLIPCHILDREN,	/* Only draw where children aren't   */
		sizeof(SCROLL *)	/* Amount of local data required     */
		);
	}
/*...e*/


