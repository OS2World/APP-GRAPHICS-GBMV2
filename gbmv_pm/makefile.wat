IDIR = bin


all:
!ifdef debug
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat debug=yes)
		(cd gbmv    && wmake -c -ms -h -f makefile.wat debug=yes)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat debug=yes)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat debug=yes)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat debug=yes)
!else
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat)
		(cd gbmv    && wmake -c -ms -h -f makefile.wat)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat)
!endif

clean:
		(cd gbmv    && wmake -c -ms -h -f makefile.wat clean)
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat clean)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat clean)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat clean)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat clean)

clobber:
		(cd gbmv    && wmake -c -ms -h -f makefile.wat clobber)
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat clobber)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat clobber)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat clobber)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat clobber)
		-del /N $(IDIR)\*   2>nul

install:
		(cd gbmv    && wmake -c -ms -h -f makefile.wat install)
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat install)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat install)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat install)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat install)
		copy gbmwpobj.cmd            $(IDIR)
		copy doc\readme_gbmos2pm.txt $(IDIR)

exepack:
		(cd gbmv    && wmake -c -ms -h -f makefile.wat exepack)
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat exepack)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat exepack)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat exepack)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat exepack)

package:
		(cd gbmv    && wmake -c -ms -h -f makefile.wat package)
		(cd gbmdlg  && wmake -c -ms -h -f makefile.wat package)
		(cd gbmv2   && wmake -c -ms -h -f makefile.wat package)
		(cd gbmvfsa && wmake -c -ms -h -f makefile.wat package)
		(cd gbmlogo && wmake -c -ms -h -f makefile.wat package)


