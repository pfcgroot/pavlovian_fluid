/******************************************************************************
 * $Id: dscomdef.h 1.2 1999/07/13 07:23:27Z krughsde Exp $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   
 * Inhalt:	   Allgemeine Typdefinitionen
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#ifndef _COMDEF_INCLUDED
#define _COMDEF_INCLUDED

#ifndef _MSC_VER
	typedef signed char INT8;		/* 8bit signed integer, char */
	typedef unsigned char UINT8;	/* 8bit unsigned integer, char */
	typedef signed short INT16;		/* 16bit signed integer */
	typedef unsigned short UINT16;	/* 16bit unsigned integer */
	#ifndef __WIN32__				/* 8..16bit Prozessoren */
		typedef unsigned long UINT32;	/* 32bit unsigned integer */
		typedef signed long INT32;		/* 32bit signed integer */
	#else							/* 32bit Compiler z.B. Win32 */
		typedef unsigned int UINT32;	/* 32bit unsigned integer */
		typedef signed int INT32;		/* 32bit signed integer */
	#endif
#endif

#ifndef TBOOL
	typedef unsigned char TBOOL;	/* Typdefinition boolean */
	#ifndef TRUE
		#define TRUE		  (TBOOL)1
	#endif
	#ifndef FALSE
		#define FALSE		  (TBOOL)0
	#endif
#endif

#endif

/******************************************************************************
 * History:
 * $Log: dscomdef.h $
 * Revision 1.2  1999/07/13 07:23:27Z  krughsde
 * signed  fuer INT8, INT16, INT32 ergaenzt
 * Revision 1.1  1999/06/25 12:04:51Z  krughsde
 * Initial revision
 *
 *****************************************************************************/
