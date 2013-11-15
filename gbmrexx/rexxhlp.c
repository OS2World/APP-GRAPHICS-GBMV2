/*

rexxhlp.c - REXX helper functions

Author  : Heiko Nitzsche
License : Public Domain

History
-------
11-May-2006: Inital version
08-Feb-2008: Some more error checking
             Drop REXX variable function
*/

#define  INCL_REXXSAA
#include <os2.h>

/* GCC on OS/2 using EMX headers has its own defines for REXX */
/* which are included via os2.h */
#ifndef RXSHV_SYSET
 #include <rexxsaa.h>
#endif

#include <string.h>

#include "rexxhlp.h"


/*********************************************************************/
/* Helper functions                                                  */
/*********************************************************************/

/*********************************************************************/
/* SetRexxVariable - Set string value (with \0) of a REXX variable   */
/*********************************************************************/
BOOL SetRexxVariable(PCSZ name,  /* REXX variable to set      */
                     PCSZ value) /* value to assign           */
{
  return SetRexxBinaryVariable(name, value, strlen(value));
}

/*********************************************************************/
/* SetRexxBinaryVariable - Set the binary data to a REXX variable    */
/*********************************************************************/
BOOL SetRexxBinaryVariable(PCSZ name,  /* REXX variable to set       */
                           PCSZ data,  /* data to assign             */
                           unsigned long datalen)
{
  const int namelen  = strlen(name);
  SHVBLOCK block;                     /* variable pool control block */
  block.shvcode = RXSHV_SYSET;        /* do a symbolic set operation */
  block.shvret  = (UCHAR)0;           /* clear return code field     */
  block.shvnext = NULL;               /* no next block               */
                                      /* set variable name string    */
                                      /* set variable name string    */
  MAKERXSTRING(block.shvname, name, namelen);
                                      /* set value string            */
  MAKERXSTRING(block.shvvalue, data, datalen);
  block.shvvaluelen = datalen;                 /* set value length   */
  if (RexxVariablePool(&block) != RXSHV_NOAVL) /* set the variable   */
  {
     return TRUE;
  }
  return FALSE;
}

/*********************************************************************/
/* GetRexxVariable - Get the value of a REXX variable                */
/*********************************************************************/
BOOL GetRexxVariable(PCSZ name,     /* REXX variable to set   */
                     PSZ  value,    /* value buffer           */
                     unsigned long valuelen) /* length of return buffer*/
{
  const int namelen = strlen(name);
  SHVBLOCK block;                     /* variable pool control block */
  block.shvcode = RXSHV_SYFET;        /* do a symbolic get operation */
  block.shvret=(UCHAR)0;              /* clear return code field     */
  block.shvnext= NULL;                /* no next block               */
                                      /* set variable name string    */
  MAKERXSTRING(block.shvname, name, namelen);

  if (valuelen < 1)
  {
     return FALSE;
  }
  memset(value, 0, valuelen);

  block.shvvalue.strptr    = value;
  block.shvvalue.strlength = valuelen-1;
  block.shvvaluelen        = valuelen-1;

  if (RexxVariablePool(&block) != RXSHV_NOAVL) /* get the variable   */
  {
     if (block.shvret == RXSHV_OK)
     {
         if (RXVALIDSTRING(block.shvvalue))
         {
            block.shvvalue.strptr[block.shvvalue.strlength] = 0;
            return TRUE;
         }
     }
  }
  return FALSE;
}

/*********************************************************************/
/* GetRexxBinaryVariable - Get the binary data of a REXX variable    */
/*********************************************************************/
BOOL GetRexxBinaryVariable(PCSZ name,    /* REXX variable to get     */
                           PSZ  data,    /* value buffer             */
                           unsigned long datalen) /* length of return buffer */
{
  const int namelen = strlen(name);
  SHVBLOCK block;                     /* variable pool control block */
  block.shvcode = RXSHV_SYFET;        /* do a symbolic get operation */
  block.shvret=(UCHAR)0;              /* clear return code field     */
  block.shvnext= NULL;                /* no next block               */
                                      /* set variable name string    */
  MAKERXSTRING(block.shvname, name, namelen);

  if (datalen < 1)
  {
     return FALSE;
  }
  memset(data, 0, datalen);

  block.shvvalue.strptr    = data;
  block.shvvalue.strlength = datalen;
  block.shvvaluelen        = datalen;

  if (RexxVariablePool(&block) != RXSHV_NOAVL) /* get the variable   */
  {
     if (block.shvret == RXSHV_OK)
     {
         if (RXVALIDSTRING(block.shvvalue))
         {
            return TRUE;
         }
     }
  }
  return FALSE;
}

/*********************************************************************/
/* DropRexxVariable - Drop the REXX variable    */
/*********************************************************************/
BOOL DropRexxVariable(PCSZ name) /* the REXX variable to drop  */
{
  const int namelen = strlen(name);
  SHVBLOCK block;                     /* variable pool control block */
  block.shvcode = RXSHV_SYDRO;        /* do symbolic drop operation  */
  block.shvret=(UCHAR)0;              /* clear return code field     */
  block.shvnext= NULL;                /* no next block               */
                                      /* set variable name string    */
  MAKERXSTRING(block.shvname, name, namelen);

  block.shvvalue.strptr    = NULL;
  block.shvvalue.strlength = 0;
  block.shvvaluelen        = 0;

  if (RexxVariablePool(&block) != RXSHV_NOAVL) /* drop the variable  */
  {
     if (block.shvret == RXSHV_OK)
     {
        return TRUE;
     }
  }
  return FALSE;
}

/*********************************************************************/
/* Function:  RegisterRexxFunctions                                  */
/*********************************************************************/
BOOL RegisterRexxFunctions(PSZ modBaseName,
                           PSZ functionNames[], int functionNamesLength)
{

  int j;

  for (j = 0; j < functionNamesLength; j++)
  {
    RexxRegisterFunctionDll(functionNames[j], modBaseName, functionNames[j]);
  }

  return TRUE;
}


/*********************************************************************/
/* Function:  UnregisterRexxFunctions                                */
/*********************************************************************/
BOOL UnregisterRexxFunctions(PSZ functionNames[], int functionNamesLength)
{
  int j;

  for (j = 0; j < functionNamesLength; j++)
  {
    RexxDeregisterFunction(functionNames[j]);
  }

  return TRUE;
}







