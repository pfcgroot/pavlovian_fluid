/******************************************************************************
 * $Id: dscomcrc.h 1.2 1999/06/29 11:03:50Z seidfrde Exp $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   crc
 * Inhalt:	   Prototypen
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#ifndef _COMCRC_INCLUDED
#define _COMCRC_INCLUDED

#include "dscomdef.h"

/* Typdefinition CRC */
typedef unsigned short TDS_CRC;
/* Vorbelegung CRC vor der Berechnung */
#define CRC_START_VAL ((TDS_CRC)-1)

/* Tabellengroesse des CRC */
#define CRC_TAB_SIZE   256

/* paul added funtion prototypes */
#ifdef __cplusplus
extern "C"
{
#endif
	TDS_CRC dscrcbyte(TDS_CRC Old_crc_us, UINT8 New_dat_uc);
#ifdef __cplusplus
} // extern "C"
#endif

#endif

/******************************************************************************
 * History:
 * $Log: dscomcrc.h $
 * Revision 1.2  1999/06/29 11:03:50Z  seidfrde
 * fuer Keil C51 angepasst
 * Revision 1.1  1999/06/25 12:04:56  krughsde
 * Initial revision
 *
 *****************************************************************************/
