/*

scroll.h - Interface to Scroller control

The scroller control has a class name of WC_SCROLL.
It consists of a window with a horizontal and/or vertical scrollbar(s).
You create another window with the scroller as its owner and parent.
The scroller arranges to position the child window in such a way as to
show the part selected by the scroll bar(s).

Notifying the scroller :-
	After creating/destroying the child window you must use SCM_CHILD.
	After changing the size of the child window you must use SCM_SIZE.

The scrollbars will be created depending on the SCS_HSCROLL and SCS_VSCROLL
window style flags. The assumption is that the child window can repaint itself
as fast as it is moved interactively by the scrollbars. If this is not true,
the SCS_HSLOW and SCS_VSLOW styles may be used to make the slider only move
the child when the scrollbars reach their final position.

When the page up/down/left/right keys are used the scrollbars will by default
attempt to move one tenth of their total range. This can be overridden by
adding the SCS_HPAGE and SCS_VPAGE styles. In this case you are notified using
SCN_HPAGE and SCN_VPAGE, when you should return the step to use. The range of
the scrollbar and the amount visible at any one time are supplied in mp2.

If the child is so small it will not cover the area of the scroller window,
the scrollbars are disabled. By default the child is positioned at the
bottom left of the scroller. This can be overridden using the SCS_HCENTRE and
SCS_VCENTRE window styles.

The scrollbars will be created with window ID's of SCID_HSCROLL and
SCID_VSCROLL, so you can use WinWindowFromID(hwndScroller, SCID_HSCROLL) etc.

How to get/and set colours :-

	WinSendMsg(hwnd, SCM_SETCOLORS, MPFROMP(&sccols), NULL);
	WinSendMsg(hwnd, SCM_QUERYCOLORS, MPFROMP(&sccols), NULL);

*/

#ifndef SCROLL_H
#define SCROLL_H

#define	WC_SCROLL	"AutoScroller"	/* Name of window class              */

#define	SCS_HSCROLL	0x0001L		/* Has a left/right slider           */
#define	SCS_VSCROLL	0x0002L		/* Has an up/down slider             */
#define	SCS_HSLOW	0x0004L		/* Move only occurs after horz slide */
#define	SCS_VSLOW	0x0008L		/* Move only occurs after vert slide */
#define	SCS_HPAGE	0x0010L		/* Ask owner for page width          */
#define	SCS_VPAGE	0x0020L		/* Ask owner for page height         */
#define	SCS_HCENTRE	0x0040L		/* When not wide, centre widthways   */
#define	SCS_VCENTRE	0x0080L		/* When not tall, centre heightways  */

#define	SCM_CHILD	 WM_USER	/* mp1=hwndChild or NULL             */
#define	SCM_SIZE	(WM_USER+1)	/* Nothing passed in                 */
#define	SCM_SETCOLORS	(WM_USER+2)	/* Set colors used by control        */
#define	SCM_QUERYCOLORS	(WM_USER+3)	/* Query colors used by control      */

#define	SCN_HPAGE	1		/* mp2=MPFROM2SHORT(cxRange,cxPage)  */
#define	SCN_VPAGE	2		/* mp2=MPFROM2SHORT(cyRange,cyPage)  */

#define	SCID_HSCROLL	1		/* Horizontal bar will have this ID  */
#define	SCID_VSCROLL	2		/* Vertical bar will have this ID    */
#define	SCID_SQUARE	3		/* The static square in the corner   */

typedef struct _SCROLLCOLORS /* sccols */
	{
	LONG lColorBlank;
	} SCROLLCOLORS;

BOOL RegisterScrollClass(HAB hab);

#endif
