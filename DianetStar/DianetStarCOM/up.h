/******************************************************************************
 * $Id: up.h 1.1 1999/08/25 12:00:18 krughsde Serie $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J. Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   dianet - star
 * Inhalt:	   Prozessorspez.Definitionen
 * ----------------------------------------------------------------------------
 *****************************************************************************/

#ifndef _UP_INCLUDED
#define _UP_INCLUDED

//#define CDS_LITTLE_ENDIAN < compiler argument def
//#define CDS_ENDIAN_PUMPE < only for device software?
//#define CDS_ENDIAN_LR < compiler argument def

#define _ROM_ const
#define  CDS_TESTPUFFER_SIZE  255


#endif
/******************************************************************************
 * History:
 * $Log: up.h $
 * Revision 1.1  1999/08/25 12:00:18  krughsde
 * Initial revision
 * Revision 1.4  1999/08/25 11:00:18  krughsde
 * little/big endian Wandlung nicht mehr im kopierten Puffer
 * Revision 1.3  1999/08/11 15:33:49  krughsde
 * Wandlung ueber ifdef
 * Revision 1.2  1999/07/12 08:40:01Z  krughsde
 * define fuer Testpuffergroesse eingebaut
 * Revision 1.1  1999/06/25 12:21:53Z  krughsde
 * Initial revision
 *
 *****************************************************************************/

