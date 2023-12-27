/******************************************************************************
 * $Id: dsend_pr.h 1.2 1999/12/06 08:42:26 krughsde Serie $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   dssim
 * Inhalt:	   Prototypen Wandlung little / big endian
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#ifndef _DSEND_PR_INCLUDED
#define _DSEND_PR_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

/* Funktionen aus dsendian.c */
/* Pumpe */
#ifdef CDS_ENDIAN_PUMPE
UINT8 dsrcvbuff2struct( UINT8 buffsize, UINT8 *pbuff, TDS_RECV *pmsg );
UINT8 dsrplstruct2buff( TDS_REPLY *pmsg, UINT8 *pbuff );
#endif

/* LR */
#ifdef CDS_ENDIAN_LR
UINT8 dslrsndstruct2buff( TDS_RECV *pmsg, UINT8 *pbuff );
UINT8 dslrrplbuff2struct( UINT8 buffsize, UINT8 *perror, UINT8 *pbuff, TDS_REPLY *pmsg );
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif

/******************************************************************************
 * History:
 * $Log: dsend_pr.h $
 * Revision 1.2  1999/12/06 08:42:26  krughsde
 * Prototyp fuer Laengenpruefung ergaenzt
 * Revision 1.1  1999/08/25 10:36:28  krughsde
 * Initial revision
 * Revision 1.2  1999/08/25 10:36:28  krughsde
 * Header in dskp.h zusammengefasst
 * Revision 1.1  1999/08/25 10:36:28  krughsde
 * Initial revision
 *
 *****************************************************************************/
