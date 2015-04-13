.autodepend

OBJDIR = ..\..\..\OBJS
EXEDIR = ..\..\..\BIN

.path.obj = $(OBJDIR)
.path.res = $(OBJDIR)
.path.exe = $(EXEDIR)
.path.dll = $(EXEDIR)


.CPP.OBJ:
     WPP386 @make.cfg -fo$@ $<

.idl.xh:
    sc -c -s"xc;xih;xh" $<
    ..\..\sominc\cnvsomex.cmd $@
     

all: make.cfg ludoc.dll ludoc.lib
   DEL MAKE.CFG

ludoc.dll: ludoc.obj lupixbuf.obj luinode.obj cpconv.obj
  copy &&|
option quiet
system os2v2 dll initinstance terminstance
option map=$(OBJDIR)\ludoc
option statics
file $(OBJDIR)\ludoc.obj
file $(OBJDIR)\lupixbuf.obj
file $(OBJDIR)\luinode.obj
file $(OBJDIR)\cpconv.obj
name $(EXEDIR)\ludoc
lib somtk 
lib libconv
export LuDocumentNewClass
export LuDocumentClassData
export LuDocumentCClassData
export _LuDocumentClassData=LuDocumentClassData
export _LuDocumentCClassData=LuDocumentCClassData
export LuPixbufNewClass
export LuPixbufClassData
export LuPixbufCClassData
export _LuPixbufClassData=LuPixbufClassData
export _LuPixbufCClassData=LuPixbufCClassData
export LuIndexNodeNewClass
export LuIndexNodeClassData
export LuIndexNodeCClassData
export _LuIndexNodeClassData=LuIndexNodeClassData
export _LuIndexNodeCClassData=LuIndexNodeCClassData
export createObject
export getSupportedExtensions
export getDescription
export getSignatureCheck
export uniConvertSpChars
export uniLigaturesLength
export uniReplaceLigatures
| tmpmake.lnk >NUL
    wlink @tmpmake.lnk
    del tmpmake.lnk

ludoc.lib: ludoc.def
	implib ludoc.lib ludoc.def

cpconv.obj: cpconv.cpp
lupixbuf.obj: lupixbuf.cpp lupixbuf.xh
luinode.obj: luinode.cpp luinode.xh
ludoc.obj: lupixbuf.obj luinode.obj ludoc.cpp ludoc.xh lupixbuf.xh luinode.xh

lupixbuf.cpp: lupixbuf.idl
luinode.cpp: luinode.idl
ludoc.cpp: ludoc.idl luinode.xh

lupixbuf.xh: lupixbuf.idl
luinode.xh: luinode.idl
ludoc.xh: ludoc.idl


MAKE.CFG: makefile
  copy &&|
-zq 
-zp=8
-bm
-ot
-ox
-oh
-xs
-xr
-I..\..\sominc
-DOS2
| MAKE.CFG >NUL
