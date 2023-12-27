/******************************************************************************
 * $Id: dscrcfkt.c 1.2 1999/06/29 11:03:14Z seidfrde Exp $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   Dianet-Star
 * Inhalt:	   CRC-Berechnung
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#include "stdafx.h"
#include "dscomdef.h"
#include "dscomcrc.h"
#include "up.h"

extern _ROM_ TDS_CRC Crc_tab[CRC_TAB_SIZE];

TDS_CRC dscrcbyte(TDS_CRC Old_crc_us, UINT8 New_dat_uc)
{
	TDS_CRC ret_val_us;
	TDS_CRC temp1_us, temp2_us;
	ret_val_us = (TDS_CRC) New_dat_uc;
	temp1_us = (TDS_CRC) (Old_crc_us >> 8);
	temp2_us = (TDS_CRC) ((Old_crc_us & 0x00ff) << 8);
	ret_val_us ^= temp2_us;
	ret_val_us ^= Crc_tab[temp1_us];
	return (ret_val_us);
}
/******************************************************************************
 * History:
 * $Log: dscrcfkt.c $
 * Revision 1.2  1999/06/29 11:03:14Z  seidfrde
 * fuer Keil C51 angepasst
 * Revision 1.1  1999/06/25 12:04:28  krughsde
 * Initial revision
 *
 *****************************************************************************/
