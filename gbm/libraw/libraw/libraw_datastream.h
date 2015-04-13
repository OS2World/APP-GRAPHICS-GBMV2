/* -*- C -*-
 * File: libraw_datastream.h
 * Copyright 2008-2010 LibRaw LLC (info@libraw.org)
 * Created: Sun Jan 18 13:07:35 2009
 *
 * LibRaw Data stream interface

LibRaw is free software; you can redistribute it and/or modify
it under the terms of the one of three licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
   (See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
   (See file LICENSE.CDDL provided in LibRaw distribution archive for details).

3. LibRaw Software License 27032010
   (See file LICENSE.LibRaw.pdf provided in LibRaw distribution archive for details).

 */

#ifndef __LIBRAW_DATASTREAM_H
#define __LIBRAW_DATASTREAM_H

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#ifndef __cplusplus


#else /* __cplusplus */

#include "libraw_const.h"
#include "libraw_types.h"
#include <fstream>
#include <memory>
#include "gbmmem.h"

#if defined (WIN32)
#include <winsock2.h>

/* MSVS 2008 and above... */
#if _MSC_VER >= 1500
#define WIN32SECURECALLS
#endif
#endif

#define IOERROR() do { throw LIBRAW_EXCEPTION_IO_EOF; } while(0)

class LibRaw_buffer_datastream;
class LibRaw_byte_buffer;
class LibRaw_bit_buffer;

class LibRaw_abstract_datastream
{
  public:
    LibRaw_abstract_datastream(){ substream=0;};
    virtual             ~LibRaw_abstract_datastream(void){if(substream) delete substream;}
    virtual int         valid() = 0;
    virtual int         read(void *,size_t, size_t ) = 0;
    virtual int         seek(INT64 , int ) = 0;
    virtual INT64       tell() = 0;
    virtual int         get_char() = 0;
    virtual char*       gets(char *, int) = 0;
    virtual int         scanf_one(const char *, void *) = 0;
    virtual int         eof() = 0;
    virtual void *      make_jas_stream() = 0;
    /* Make buffer from current offset */
    virtual LibRaw_byte_buffer *make_byte_buffer(unsigned int sz);

    /* subfile parsing not implemented in base class */
    virtual const char* fname(){ return NULL;};
    virtual int         subfile_open(const char*) { return -1;}
    virtual void        subfile_close() { }


    virtual int		tempbuffer_open(void*, size_t);
    virtual void	tempbuffer_close();

  protected:
    LibRaw_abstract_datastream *substream;
};

/* HN: For GBM purpose we only need buffer access.
 *     So we strip the code for files here and save
 *     quite some code size in the binary.
 */
#if 0 /* HN */

#ifdef WIN32
template class DllDef std::auto_ptr<std::streambuf>;
#endif

class LibRaw_file_datastream: public LibRaw_abstract_datastream
{
  protected:
    std::auto_ptr<std::streambuf> f; /* will close() automatically through dtor */
    std::auto_ptr<std::streambuf> saved_f; /* when *f is a subfile, *saved_f is the master file */
    const char *filename;

  public:
    virtual             ~LibRaw_file_datastream(){}
                        LibRaw_file_datastream(const char *fname);
    virtual void        *make_jas_stream();
    virtual int         valid();
    virtual int         read(void * ptr,size_t size, size_t nmemb);
    virtual int         eof();
    virtual int         seek(INT64 o, int whence);
    virtual INT64       tell();
    virtual int         get_char()
        { 
            if(substream) return substream->get_char();
            return f->sbumpc();  
        }
    virtual char*       gets(char *str, int sz); 
    virtual int         scanf_one(const char *fmt, void*val); 
    virtual const char* fname();
    virtual int         subfile_open(const char *fn);
    virtual void        subfile_close();
};

#endif // #if 0 /* HN */


class LibRaw_buffer_datastream : public LibRaw_abstract_datastream
{
  public:
                        LibRaw_buffer_datastream(void *buffer, size_t bsize);
    virtual             ~LibRaw_buffer_datastream();
    virtual int         valid();
    virtual void        *make_jas_stream();
    virtual LibRaw_byte_buffer *make_byte_buffer(unsigned int sz);
    virtual int         read(void * ptr,size_t sz, size_t nmemb);
    virtual int         eof();
    virtual int         seek(INT64 o, int whence);
    virtual INT64       tell();
    virtual char*       gets(char *s, int sz);
    virtual int         scanf_one(const char *fmt, void* val);
    virtual int         get_char()
    { 
        if(substream) return substream->get_char();
        if(streampos>=streamsize)
            return -1;
        return buf[streampos++];
    }

  private:
    unsigned char *buf;
    size_t   streampos,streamsize;
};

/* HN: For GBM purpose we only need buffer access.
 *     So we strip the code for files here and save
 *     quite some code size in the binary.
 */
#if 0 /* HN */

class LibRaw_bigfile_datastream : public LibRaw_abstract_datastream
{
  public:
                        LibRaw_bigfile_datastream(const char *fname);
    virtual             ~LibRaw_bigfile_datastream();
    virtual int         valid();
    virtual void *make_jas_stream();

    virtual int         read(void * ptr,size_t size, size_t nmemb); 
    virtual int         eof();
    virtual int         seek(INT64 o, int whence);
    virtual INT64       tell();
    virtual char*       gets(char *str, int sz);
    virtual int         scanf_one(const char *fmt, void*val);
    virtual const char *fname();
    virtual int         subfile_open(const char *fn);
    virtual void        subfile_close();
    virtual int         get_char()
    { 
#ifndef WIN32
        return substream?substream->get_char():getc_unlocked(f);
#else
        return substream?substream->get_char():fgetc(f);
#endif
    }

  private:
    FILE *f,*sav;
    const char *filename;
};


#ifdef WIN32
class LibRaw_windows_datastream : public LibRaw_buffer_datastream 
{
public:
    /* ctor: high level constructor opens a file by name */
    LibRaw_windows_datastream(const TCHAR* sFile);
    /* ctor: construct with a file handle - caller is responsible for closing the file handle */
    LibRaw_windows_datastream(HANDLE hFile);
    /* dtor: unmap and close the mapping handle */
    virtual ~LibRaw_windows_datastream();

protected:
    void Open(HANDLE hFile);
    inline void reconstruct_base()
	{
            /* this subterfuge is to overcome the private-ness of LibRaw_buffer_datastream */
            (LibRaw_buffer_datastream&)*this = LibRaw_buffer_datastream(pView_, (size_t)cbView_);
	}

    HANDLE		hMap_;			/* handle of the file mapping */
    void*		pView_;			/* pointer to the mapped memory */
    __int64	cbView_;		/* size of the mapping in bytes */
};

#endif

#endif // #if 0 /* HN */


#endif /* cplusplus */
#endif

