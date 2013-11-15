
#ifndef __REXXHLP_H__
 #define __REXXHLP_H__

/*********************************************************************/
/* SetRexxVariable - Set the value of a REXX variable                */
/*********************************************************************/
BOOL SetRexxVariable(PCSZ name,   /* REXX variable to set            */
                     PCSZ value); /* value to assign                 */

BOOL SetRexxBinaryVariable(PCSZ name,  /* REXX variable to set       */
                           PCSZ data,  /* data to assign             */
                           unsigned long datalen); /* data length    */

/*********************************************************************/
/* GetRexxVariable - Get the value of a REXX variable                */
/*********************************************************************/
BOOL GetRexxVariable(PCSZ name,      /* REXX variable to set   */
                     PSZ  value,     /* value buffer           */
                     unsigned long valuelen); /* length of return buffer*/

BOOL GetRexxBinaryVariable(PCSZ name,     /* REXX variable to set    */
                           PSZ  data,     /* value buffer            */
                           unsigned long datalen); /* length of return buffer */

/*********************************************************************/
/* DropRexxVariable - Drop the REXX variable                         */
/*********************************************************************/
BOOL DropRexxVariable(PCSZ name); /* the REXX variable to drop  */

/*********************************************************************/
/* Function:  RegisterRexxFunctions                                  */
/*********************************************************************/
BOOL RegisterRexxFunctions(PSZ modBaseName,
                           PSZ functionNames[], int functionNamesLength);

/*********************************************************************/
/* Function:  UnregisterRexxFunctions                                */
/*********************************************************************/
BOOL UnregisterRexxFunctions(PSZ functionNames[], int functionNamesLength);



#endif /* __REXXHLP_H__ */

