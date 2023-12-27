/******************************************************************************
 * $Id: dsendian.c 1.12 2001/09/17 14:00:23 krughsde Serie $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:		Dianet star
 * Inhalt:	   Wandlung little /big endian
 *             Bedingte Compilierung ueber setzen folgender defines in up.h :
 *             #define CDS_LITTLE_ENDIAN  : Little endian, Bytes werden gedreht
 *             #define CDS_ENDIAN_PUMPE   : Pumpenfunktionen werden uebersetzt
 *             #define CDS_ENDIAN_LR      : LR - Funktionen werden uebersetzt
 *
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#include "stdafx.h"
#include <string.h>
#include "dstarcom.h"
#include "dsend_pr.h"


static UINT8 *cnv2buff(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize);
static UINT8 *cnv2structr(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize);
#ifdef CDS_MITPCA
static UINT8 *cnv2structf(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize);
#endif
static void cnvit(UINT8 * psrc, UINT8 * pdest, UINT8 size);

#ifdef CDS_ENDIAN_LR
static UINT8 *dsrcvhead2buff(TDS_RECV_HEAD * prcvhead, UINT8 * pbuff, UINT8 * psize);
#endif
static UINT8 *dsbuff2rcvhead(UINT8 * pbuff, TDS_RECV_HEAD * prcvhead, UINT8 * psize);

static UINT8 *dsrplhead2buff(TDS_REPLY_HEAD * prplhead, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2rplhead(UINT8 * pbuff, TDS_REPLY_HEAD * prplhead, UINT8 * psize);
#endif
#ifdef CDS_ENDIAN_LR
static UINT8 *dsaktion2buff(TDS_DATA_AKTION * paktion, UINT8 * pbuff, UINT8 * psize);
#endif
static UINT8 *dsbuff2aktion(UINT8 * pbuff, TDS_DATA_AKTION * paktion, UINT8 * psize);

#ifdef CDS_ENDIAN_LR
static UINT8 *dsccmode2buff(TDS_DATA_CCMODE * pccmode, UINT8 * pbuff, UINT8 * psize);
#endif
static UINT8 *dsbuff2ccmode(UINT8 * pbuff, TDS_DATA_CCMODE * pccmode, UINT8 * psize);

static UINT8 *dsnak2buff(TDS_DATA_NAK * pnak, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2nak(UINT8 * pbuff, TDS_DATA_NAK * pnak, UINT8 * psize);
#endif
static UINT8 *dsident2buff(TDS_DATA_IDENT * pident, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2ident(UINT8 * pbuff, TDS_DATA_IDENT * pident, UINT8 * psize);
#endif
static UINT8 *dsiw2buff(TDS_DATA_ISTWERT * piw, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2iw(UINT8 * pbuff, TDS_DATA_ISTWERT * piw, UINT8 * psize);
#endif
static UINT8 *dslimits2buff(TDS_DATA_LIMITS * plimits, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2limits(UINT8 * pbuff, TDS_DATA_LIMITS * plimits, UINT8 * psize);
#endif
static UINT8 *dsstatus2buff(TDS_DATA_STATUS * pstatus, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2status(UINT8 * pbuff, TDS_DATA_STATUS * pstatus, UINT8 * psize);
#endif
static UINT8 *dssw2buff(TDS_DATA_SOLLWERT * psw, UINT8 * pbuff, UINT8 * psize);
static UINT8 *dsbuff2sw(UINT8 * pbuff, TDS_DATA_SOLLWERT * psw, UINT8 * psize);

static UINT8 *dsprop2buff(TDS_DATA_PROPOSAL * pprop, UINT8 * pbuff, UINT8 * psize);
static UINT8 *dsbuff2prop(UINT8 * pbuff, TDS_DATA_PROPOSAL * pprop, UINT8 * psize);

static UINT8 *dsreload2buff(TDS_DATA_RELOAD * preload, UINT8 * pbuff, UINT8 * psize);
static UINT8 *dsbuff2reload(UINT8 * pbuff, TDS_DATA_RELOAD * preload, UINT8 * psize);

static UINT8 *dsdosis2buff(TDS_DATA_DOSIS * pdosis, UINT8 * pbuff, UINT8 * psize);
static UINT8 *dsbuff2dosis(UINT8 * pbuff, TDS_DATA_DOSIS * pdosis, UINT8 * psize);

static UINT8 *dshistory2buff(TDS_DATA_HISTORY * phistory, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2history(UINT8 * pbuff, TDS_DATA_HISTORY * phistory, UINT8 * psize);
#endif

#ifdef CDS_MITPCA
static UINT8 *dspcaprotread2buff(TDS_DATA_PCAPROT_READ * ppcaprotread, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2pcaprotread(UINT8 * pbuff, TDS_DATA_PCAPROT_READ * ppcaprotread, UINT8 * psize);
#endif

static UINT8 *dspcaprot2buff(TDS_DATA_PCAPROT * ppcaprot, UINT8 * pbuff, UINT8 * psize);
#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2pcaprot(UINT8 * pbuff, TDS_DATA_PCAPROT * ppcaprot, UINT8 * psize);
#endif
#endif

#define MCNV2BUFF( src ) \
                   cnv2buff( (UINT8 *)&src, ptr, sizeof( src ), psize )

#define MCNV2STRUCTR( src ) \
                   cnv2structr( ptr, (UINT8 *)&src, sizeof( src ), psize )

#define MCNV2STRUCTF( src ) \
                   cnv2structf( ptr, (UINT8 *)&src, sizeof( src ), psize )


#ifdef CDS_ENDIAN_PUMPE
static TDS_RECV_HEAD lRcvHead;
#endif

#ifdef CDS_ENDIAN_LR
static TDS_REPLY_HEAD lRplHead;
#endif

/******************************************************************************
 * UINT8 dsrcvbuff2struct( UINT8 buffsize, UINT8 *pbuff, TDS_RECV *pmsg )
 *
 *      Wandelt den empfangenen Datenstrom in die Struktur TDS_RECV
 *      in der Pumpe
 *
 * IN:
 *		- UINT8  buffsize : Die Anzahl der empfangenen Daten
 *      - UINT8 *pbuff    : Zeiger auf die empfangenen Daten
 *
 * OUT:
 *		- TDS_RECV *pmsg  : Zeiger auf die Zieldaten
 *        UINT8    return : Die Anzahl der gewandelten Daten
 *
 * USED:
 *		-
 *****************************************************************************/
#ifdef CDS_ENDIAN_PUMPE
UINT8 dsrcvbuff2struct(UINT8 buffsize, UINT8 * pbuff, TDS_RECV * pmsg)
{
	UINT8 *ptr = pbuff;
	UINT8 size = 0;
	UINT8 alignsize = 0;
	ptr = pbuff + sizeof(lRcvHead);
	dsbuff2rcvhead(ptr, &lRcvHead, &size);

	ptr = pbuff + buffsize;		/* Der Zeiger zeigt jetzt hinter das letzte Byte */

	switch (lRcvHead.segnr)
	{
	case CDS_RSEG_HISTORY:
	case CDS_RSEG_IDENT:
	case CDS_RSEG_STATUS:
	case CDS_RSEG_ISTWERT:
	case CDS_RSEG_LIMITS:
	case CDS_RSEG_SOLLWERT:
	case CDS_RSEG_PROPOSAL:
	case CDS_RSEG_RELOAD:
	case CDS_RSEG_DOSIS:
		alignsize = CDS_RECV_READ_LEN;
		break;
#ifdef CDS_MITPCA
	case CDS_RSEG_PCAPROT:
		ptr = dsbuff2pcaprotread(ptr, &pmsg->_data.pcaprotread, &size);
		alignsize = CDS_RECV_PCAPROT_LEN;
		break;
#endif
	case CDS_WSEG_AKTION:
		ptr = dsbuff2aktion(ptr, &pmsg->_data.aktion, &size);
		alignsize = CDS_RECV_AKTION_LEN;
		break;
	case CDS_WSEG_CC_MODE:
		ptr = dsbuff2ccmode(ptr, &pmsg->_data.ccmode, &size);
		alignsize = CDS_RECV_CCMODE_LEN;
		break;
	case CDS_WSEG_SOLLWERT:
		ptr = dsbuff2sw(ptr, &pmsg->_data.sollwert, &size);
		alignsize = CDS_RECV_SOLLWERT_LEN;
		break;
	case CDS_WSEG_PROPOSAL:
		ptr = dsbuff2prop(ptr, &pmsg->_data.proposal, &size);
		alignsize = CDS_RECV_PROPOSAL_LEN;
		break;
	case CDS_WSEG_RELOAD:
		ptr = dsbuff2reload(ptr, &pmsg->_data.reload, &size);
		alignsize = CDS_RECV_RELOAD_LEN;
		break;
#ifdef  CDS_MITDOSISWRITE
	case CDS_WSEG_DOSIS:
		ptr = dsbuff2dosis(ptr, &pmsg->_data.dosis, &size);
		alignsize = CDS_RECV_DOSIS_LEN;
		break;
#endif
	default:
		alignsize = 0;
		break;
	}
	if (size == buffsize)
	{
		/* Empfangene Groesse stimmt mit konvertierter Grosse ueberein */
		/* Also Groesse fuer darueberliegende Funktion auf Strukturgroesse */
		/* anpassen */
		size = alignsize;
	}
	memcpy(&pmsg->head, &lRcvHead, sizeof(TDS_RECV_HEAD));
	return (size);
}
#endif


/******************************************************************************
 * UINT8 dsrplstruct2buff( TDS_REPLY *pmsg, UINT8 *pbuff )
 *
 *      Wandelt die zu sendende Struktur TDS_REPLY in den Datenstrom
 *      in der Pumpe
 *
 * IN:
 *		- TDS_REPLY *pmsg : Zeiger auf die zu sendende Struktur
 *
 * OUT:
 *		- UINT8   *pbuff  : Zeiger auf die Zieldatenstrom
 *        UINT8    return : Die Anzahl der zu sendenden Daten
 *
 * USED:
 *		-
 *****************************************************************************/
#ifdef CDS_ENDIAN_PUMPE
UINT8 dsrplstruct2buff(TDS_REPLY * pmsg, UINT8 * pbuff)
{
	UINT8 *ptr = pbuff;
	UINT8 size = 0;
	ptr = dsrplhead2buff(&pmsg->head, ptr, &size);
	if (pmsg->head._code == CDS_EVENT_ACK)
	{
		switch (pmsg->head.segnr)
		{
		case CDS_RSEG_IDENT:
			ptr = dsident2buff(&pmsg->_data.ident, ptr, &size);
			break;
		case CDS_RSEG_STATUS:
			ptr = dsstatus2buff(&pmsg->_data.status, ptr, &size);
			break;
		case CDS_RSEG_ISTWERT:
			ptr = dsiw2buff(&pmsg->_data.istwert, ptr, &size);
			break;
		case CDS_RSEG_LIMITS:
			ptr = dslimits2buff(&pmsg->_data.limits, ptr, &size);
			break;
		case CDS_RSEG_SOLLWERT:
			ptr = dssw2buff(&pmsg->_data.sollwert, ptr, &size);
			break;
		case CDS_RSEG_PROPOSAL:
			ptr = dsprop2buff(&pmsg->_data.proposal, ptr, &size);
			break;
		case CDS_RSEG_DOSIS:
			ptr = dsdosis2buff(&pmsg->_data.dosis, ptr, &size);
			break;
		case CDS_RSEG_RELOAD:
			ptr = dsreload2buff(&pmsg->_data.reload, ptr, &size);
			break;
		case CDS_RSEG_HISTORY:
			ptr = dshistory2buff(&pmsg->_data.history, ptr, &size);
			break;
#ifdef CDS_MITPCA
		case CDS_RSEG_PCAPROT:
			ptr = dspcaprot2buff(&pmsg->_data.pcaprot, ptr, &size);
			break;
#endif
		case CDS_WSEG_AKTION:
		case CDS_WSEG_CC_MODE:
		case CDS_WSEG_SOLLWERT:
		case CDS_WSEG_PROPOSAL:
		case CDS_WSEG_RELOAD:
#ifdef  CDS_MITDOSISWRITE
		case CDS_WSEG_DOSIS:
#endif
			break;
		default:
			break;
		}
	}
	else
	{
		ptr = dsnak2buff(&pmsg->_data.nak, ptr, &size);
	}
	return (size);
}
#endif

/******************************************************************************
 * UINT8 dslrsndstruct2buff( TDS_RECV *pmsg, UINT8 *pbuff )
 *
 *      Wandelt die zu sendende Struktur TDS_RECV in den Datenstrom
 *      im LR
 *
 * IN:
 *		- TDS_RECV *pmsg  : Zeiger auf  die zu sendende Struktur
 *
 * OUT:
 *		- UINT8   *pbuff  : Zeiger auf die Zieldatenstrom
 *        UINT8    return : Die Anzahl der zu sendenden Daten
 *
 * USED:
 *		-
 *****************************************************************************/
#ifdef CDS_ENDIAN_LR
UINT8 dslrsndstruct2buff(TDS_RECV * pmsg, UINT8 * pbuff)
{
	UINT8 *ptr = pbuff;
	UINT8 size = 0;

	ptr = dsrcvhead2buff(&pmsg->head, ptr, &size);
	switch (pmsg->head.segnr)
	{
	case CDS_RSEG_IDENT:
	case CDS_RSEG_STATUS:
	case CDS_RSEG_ISTWERT:
	case CDS_RSEG_LIMITS:
	case CDS_RSEG_SOLLWERT:
	case CDS_RSEG_PROPOSAL:
	case CDS_RSEG_HISTORY:
	case CDS_RSEG_DOSIS:
	case CDS_RSEG_RELOAD:
		break;
#ifdef CDS_MITPCA
	case CDS_RSEG_PCAPROT:
		ptr = dspcaprotread2buff(&pmsg->_data.pcaprotread, ptr, &size);
		break;
#endif
	case CDS_WSEG_AKTION:
		ptr = dsaktion2buff(&pmsg->_data.aktion, ptr, &size);
		break;
	case CDS_WSEG_CC_MODE:
		ptr = dsccmode2buff(&pmsg->_data.ccmode, ptr, &size);
		break;
	case CDS_WSEG_SOLLWERT:
		ptr = dssw2buff(&pmsg->_data.sollwert, ptr, &size);
		break;
	case CDS_WSEG_PROPOSAL:
		ptr = dsprop2buff(&pmsg->_data.proposal, ptr, &size);
		break;
	case CDS_WSEG_RELOAD:
		ptr = dsreload2buff(&pmsg->_data.reload, ptr, &size);
		break;
#ifdef  CDS_MITDOSISWRITE
	case CDS_WSEG_DOSIS:
		ptr = dsdosis2buff(&pmsg->_data.dosis, ptr, &size);
		break;
#endif
	default:
		break;
	}
	return (size);
}
#endif

/******************************************************************************
 * UINT8 dslrrplbuff2struct( UINT8 buffsize, UINT8 *pbuff, TDS_REPLY *pmsg )
 *
 *      Wandelt den empfangenen Datenstrom in die Struktur TDS_REPLY
 *      im LR
 *
 * IN:
 *		- UINT8  buffsize : Die Anzahl der empfangenen Daten
 *      - UINT8 *pbuff    : Zeiger auf die empfangenen Daten
 *
 * OUT:
 *		- TDS_REPLY *pmsg : Zeiger auf die Zieldaten
 *        UINT8    return : Die Anzahl der gewandelten Daten
 *
 * USED:
 *		-
 *****************************************************************************/
#ifdef CDS_ENDIAN_LR
UINT8 dslrrplbuff2struct(UINT8 buffsize, UINT8 * perror, UINT8 * pbuff, TDS_REPLY * pmsg)
{
	UINT8 *ptr = pbuff;
	UINT8 size = 0;
	UINT8 alignsize = 0;

	*perror = 0;
	ptr = pbuff + sizeof(lRplHead);

	dsbuff2rplhead(ptr, &lRplHead, &size);
	ptr = pbuff + buffsize;		/* Der Zeiger zeigt jetzt hinter das letzte Byte */

	if (lRplHead._code == CDS_EVENT_ACK)
	{
		switch (lRplHead.segnr)
		{
		case CDS_RSEG_IDENT:
			ptr = dsbuff2ident(ptr, &pmsg->_data.ident, &size);
			alignsize = CDS_REPLY_IDENT_LEN;
			break;
		case CDS_RSEG_STATUS:
			ptr = dsbuff2status(ptr, &pmsg->_data.status, &size);
			alignsize = CDS_REPLY_STATUS_LEN;
			break;
		case CDS_RSEG_ISTWERT:
			ptr = dsbuff2iw(ptr, &pmsg->_data.istwert, &size);
			alignsize = CDS_REPLY_ISTWERT_LEN;
			break;
		case CDS_RSEG_LIMITS:
			ptr = dsbuff2limits(ptr, &pmsg->_data.limits, &size);
			alignsize = CDS_REPLY_LIMITS_LEN;
			break;
		case CDS_RSEG_SOLLWERT:
			ptr = dsbuff2sw(ptr, &pmsg->_data.sollwert, &size);
			alignsize = CDS_REPLY_SOLLWERT_LEN;
			break;
		case CDS_RSEG_PROPOSAL:
			ptr = dsbuff2prop(ptr, &pmsg->_data.proposal, &size);
			alignsize = CDS_REPLY_PROPOSAL_LEN;
			break;
		case CDS_RSEG_DOSIS:
			ptr = dsbuff2dosis(ptr, &pmsg->_data.dosis, &size);
			alignsize = CDS_REPLY_DOSIS_LEN;
			break;
		case CDS_RSEG_RELOAD:
			ptr = dsbuff2reload(ptr, &pmsg->_data.reload, &size);
			alignsize = CDS_REPLY_RELOAD_LEN;
			break;
		case CDS_RSEG_HISTORY:
			ptr = dsbuff2history(ptr, &pmsg->_data.history, &size);
			alignsize = CDS_REPLY_HISTORY_LEN;
			break;
#ifdef CDS_MITPCA
		case CDS_RSEG_PCAPROT:
			ptr = pbuff + sizeof(lRplHead);
			ptr = dsbuff2pcaprot(ptr, &pmsg->_data.pcaprot, &size);
			alignsize = CDS_REPLY_PCAPROT_LEN;
			break;
#endif
		case CDS_WSEG_AKTION:
		case CDS_WSEG_CC_MODE:
		case CDS_WSEG_SOLLWERT:
		case CDS_WSEG_PROPOSAL:
		case CDS_WSEG_RELOAD:
#ifdef  CDS_MITDOSISWRITE
		case CDS_WSEG_DOSIS:
#endif
			alignsize = CDS_REPLY_ACK_LEN;
			break;
		default:
			*perror = CDS_ERR_SEG;
			alignsize = 0;
			break;
		}
	}
	else
	{
		ptr = dsbuff2nak(ptr, &pmsg->_data.nak, &size);
		alignsize = CDS_REPLY_NAK_LEN;
	}
	if (alignsize != 0)
	{
		if (size == buffsize)
		{
			/* Empfangene Groesse stimmt mit konvertierter Grosse ueberein */
			/* Also Groesse fuer darueberliegende Funktion auf Strukturgroesse */
			/* anpassen */
			size = alignsize;
		}
		else
		{
			*perror = CDS_ERR_SEG_LENGTH;
		}
	}
	memcpy(&pmsg->head, &lRplHead, sizeof(TDS_REPLY_HEAD));

	return (size);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsrcvhead2buff(TDS_RECV_HEAD * prcvhead, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(prcvhead->adress);
	ptr = MCNV2BUFF(prcvhead->segnr);
	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsbuff2rcvhead(UINT8 * pbuff, TDS_RECV_HEAD * prcvhead, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(prcvhead->segnr);
	ptr = MCNV2STRUCTR(prcvhead->adress);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsrplhead2buff(TDS_REPLY_HEAD * prplhead, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(prplhead->adress);
	ptr = MCNV2BUFF(prplhead->segnr);
	ptr = MCNV2BUFF(prplhead->_code);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2rplhead(UINT8 * pbuff, TDS_REPLY_HEAD * prplhead, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(prplhead->_code);
	ptr = MCNV2STRUCTR(prplhead->segnr);
	ptr = MCNV2STRUCTR(prplhead->adress);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsaktion2buff(TDS_DATA_AKTION * paktion, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(paktion->kennung);
	ptr = MCNV2BUFF(paktion->flag);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsbuff2aktion(UINT8 * pbuff, TDS_DATA_AKTION * paktion, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(paktion->flag);
	ptr = MCNV2STRUCTR(paktion->kennung);

	return (ptr);
}
#endif

#ifdef CDS_MITPCA
#ifdef CDS_ENDIAN_LR
static UINT8 *dspcaprotread2buff(TDS_DATA_PCAPROT_READ * ppcaprotread, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;
	ptr = MCNV2BUFF(ppcaprotread->read_event);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsbuff2pcaprotread(UINT8 * pbuff, TDS_DATA_PCAPROT_READ * ppcaprotread, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(ppcaprotread->read_event);
	return (ptr);
}
#endif
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsccmode2buff(TDS_DATA_CCMODE * pccmode, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(pccmode->identnrinv);
	ptr = MCNV2BUFF(pccmode->modeswitch);
	ptr = MCNV2BUFF(pccmode->newadress);
	ptr = MCNV2BUFF(pccmode->unixtime);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsbuff2ccmode(UINT8 * pbuff, TDS_DATA_CCMODE * pccmode, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pccmode->unixtime);
	ptr = MCNV2STRUCTR(pccmode->newadress);
	ptr = MCNV2STRUCTR(pccmode->modeswitch);
	ptr = MCNV2STRUCTR(pccmode->identnrinv);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsnak2buff(TDS_DATA_NAK * pnak, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;
	ptr = MCNV2BUFF(pnak->errcode);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2nak(UINT8 * pbuff, TDS_DATA_NAK * pnak, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pnak->errcode);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsident2buff(TDS_DATA_IDENT * pident, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(pident->identnr);
	ptr = MCNV2BUFF(pident->protovers);
	ptr = MCNV2BUFF(pident->swvers);
	ptr = MCNV2BUFF(pident->unixtime);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2ident(UINT8 * pbuff, TDS_DATA_IDENT * pident, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pident->unixtime);
	ptr = MCNV2STRUCTR(pident->swvers);
	ptr = MCNV2STRUCTR(pident->protovers);
	ptr = MCNV2STRUCTR(pident->identnr);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsiw2buff(TDS_DATA_ISTWERT * piw, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(piw->druckwert);
	ptr = MCNV2BUFF(piw->infusedvol);
	ptr = MCNV2BUFF(piw->infusedtime);
	ptr = MCNV2BUFF(piw->bolintvrestzeit);
	ptr = MCNV2BUFF(piw->akkurest);
	ptr = MCNV2BUFF(piw->standbyrest);
	ptr = MCNV2BUFF(piw->syringevol);
	ptr = MCNV2BUFF(piw->bolintvzeit);
	ptr = MCNV2BUFF(piw->bolintvvol);
	ptr = MCNV2BUFF(piw->bolvolakt);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2iw(UINT8 * pbuff, TDS_DATA_ISTWERT * piw, UINT8 * psize)
{
	UINT8 *ptr = pbuff;
	ptr = MCNV2STRUCTR(piw->bolvolakt);
	ptr = MCNV2STRUCTR(piw->bolintvvol);
	ptr = MCNV2STRUCTR(piw->bolintvzeit);
	ptr = MCNV2STRUCTR(piw->syringevol);
	ptr = MCNV2STRUCTR(piw->standbyrest);
	ptr = MCNV2STRUCTR(piw->akkurest);
	ptr = MCNV2STRUCTR(piw->bolintvrestzeit);
	ptr = MCNV2STRUCTR(piw->infusedtime);
	ptr = MCNV2STRUCTR(piw->infusedvol);
	ptr = MCNV2STRUCTR(piw->druckwert);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dslimits2buff(TDS_DATA_LIMITS * plimits, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(plimits->bolmax);
	ptr = MCNV2BUFF(plimits->personal);
	ptr = MCNV2BUFF(plimits->resdisp);
	ptr = MCNV2BUFF(plimits->ratemax);
	ptr = MCNV2BUFF(plimits->ratemin);
	ptr = MCNV2BUFF(plimits->spritzevoral);
	ptr = MCNV2BUFF(plimits->sonderfkt);
	ptr = MCNV2BUFF(plimits->druckmax);
	ptr = MCNV2BUFF(plimits->vtbdmax);
	ptr = MCNV2BUFF(plimits->bolvolmax);
	ptr = MCNV2BUFF(plimits->bolmin);
	ptr = MCNV2BUFF(plimits->bolvolmin);
	ptr = MCNV2BUFF(plimits->resbol);
	ptr = MCNV2BUFF(plimits->caratemax);
	ptr = MCNV2BUFF(plimits->caratemin);
	ptr = MCNV2BUFF(plimits->ccratemax);
	ptr = MCNV2BUFF(plimits->ccratemin);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2limits(UINT8 * pbuff, TDS_DATA_LIMITS * plimits, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(plimits->ccratemin);
	ptr = MCNV2STRUCTR(plimits->ccratemax);
	ptr = MCNV2STRUCTR(plimits->caratemin);
	ptr = MCNV2STRUCTR(plimits->caratemax);
	ptr = MCNV2STRUCTR(plimits->resbol);
	ptr = MCNV2STRUCTR(plimits->bolvolmin);
	ptr = MCNV2STRUCTR(plimits->bolmin);
	ptr = MCNV2STRUCTR(plimits->bolvolmax);
	ptr = MCNV2STRUCTR(plimits->vtbdmax);
	ptr = MCNV2STRUCTR(plimits->druckmax);
	ptr = MCNV2STRUCTR(plimits->sonderfkt);
	ptr = MCNV2STRUCTR(plimits->spritzevoral);
	ptr = MCNV2STRUCTR(plimits->ratemin);
	ptr = MCNV2STRUCTR(plimits->ratemax);
	ptr = MCNV2STRUCTR(plimits->resdisp);
	ptr = MCNV2STRUCTR(plimits->personal);
	ptr = MCNV2STRUCTR(plimits->bolmax);

	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dsstatus2buff(TDS_DATA_STATUS * pstatus, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(pstatus->request);
	ptr = MCNV2BUFF(pstatus->alarm);
	ptr = MCNV2BUFF(pstatus->zustand);
	ptr = MCNV2BUFF(pstatus->rate);
	ptr = MCNV2BUFF(pstatus->remainvtbd);
	ptr = MCNV2BUFF(pstatus->remaintime);


	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2status(UINT8 * pbuff, TDS_DATA_STATUS * pstatus, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pstatus->remaintime);
	ptr = MCNV2STRUCTR(pstatus->remainvtbd);
	ptr = MCNV2STRUCTR(pstatus->rate);
	ptr = MCNV2STRUCTR(pstatus->zustand);
	ptr = MCNV2STRUCTR(pstatus->alarm);
	ptr = MCNV2STRUCTR(pstatus->request);

	return (ptr);
}
#endif

static UINT8 *dssw2buff(TDS_DATA_SOLLWERT * psw, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(psw->rate);
	ptr = MCNV2BUFF(psw->vtbd);
	ptr = MCNV2BUFF(psw->time);
	ptr = MCNV2BUFF(psw->bolrate);
	ptr = MCNV2BUFF(psw->bolvol);
	ptr = MCNV2BUFF(psw->drucklimitmax);
	ptr = MCNV2BUFF(psw->stdby);
	ptr = MCNV2BUFF(psw->medikament);

	return (ptr);
}

static UINT8 *dsbuff2sw(UINT8 * pbuff, TDS_DATA_SOLLWERT * psw, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(psw->medikament);
	ptr = MCNV2STRUCTR(psw->stdby);
	ptr = MCNV2STRUCTR(psw->drucklimitmax);
	ptr = MCNV2STRUCTR(psw->bolvol);
	ptr = MCNV2STRUCTR(psw->bolrate);
	ptr = MCNV2STRUCTR(psw->time);
	ptr = MCNV2STRUCTR(psw->vtbd);
	ptr = MCNV2STRUCTR(psw->rate);

	return (ptr);
}

static UINT8 *dsprop2buff(TDS_DATA_PROPOSAL * pprop, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(pprop->medikament);
	ptr = MCNV2BUFF(pprop->rate);

	return (ptr);
}

static UINT8 *dsbuff2prop(UINT8 * pbuff, TDS_DATA_PROPOSAL * pprop, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pprop->rate);
	ptr = MCNV2STRUCTR(pprop->medikament);

	return (ptr);
}

static UINT8 *dsreload2buff(TDS_DATA_RELOAD * preload, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(preload->aktiv);
	ptr = MCNV2BUFF(preload->rate);
	ptr = MCNV2BUFF(preload->vtbd);

	return (ptr);
}

static UINT8 *dsbuff2reload(UINT8 * pbuff, TDS_DATA_RELOAD * preload, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(preload->vtbd);
	ptr = MCNV2STRUCTR(preload->rate);
	ptr = MCNV2STRUCTR(preload->aktiv);

	return (ptr);
}

static UINT8 *dsdosis2buff(TDS_DATA_DOSIS * pdosis, UINT8 * pbuff, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(pdosis->kg);
	ptr = MCNV2BUFF(pdosis->konz);
	ptr = MCNV2BUFF(pdosis->einhkonz);
	ptr = MCNV2BUFF(pdosis->rate);
	ptr = MCNV2BUFF(pdosis->einhrate);
	ptr = MCNV2BUFF(pdosis->gebvol);
	ptr = MCNV2BUFF(pdosis->drug);
	ptr = MCNV2BUFF(pdosis->einhdrug);

	return (ptr);
}

static UINT8 *dsbuff2dosis(UINT8 * pbuff, TDS_DATA_DOSIS * pdosis, UINT8 * psize)
{
	UINT8 *ptr = pbuff;

	ptr = MCNV2STRUCTR(pdosis->einhdrug);
	ptr = MCNV2STRUCTR(pdosis->drug);
	ptr = MCNV2STRUCTR(pdosis->gebvol);
	ptr = MCNV2STRUCTR(pdosis->einhrate);
	ptr = MCNV2STRUCTR(pdosis->rate);
	ptr = MCNV2STRUCTR(pdosis->einhkonz);
	ptr = MCNV2STRUCTR(pdosis->konz);
	ptr = MCNV2STRUCTR(pdosis->kg);

	return (ptr);
}

#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dshistory2buff(TDS_DATA_HISTORY * phistory, UINT8 * pbuff, UINT8 * psize)
{
	int i;
	UINT8 *ptr = pbuff;

	ptr = MCNV2BUFF(phistory->actualreltime);
	for (i = 0; i < CDS_HISTORY_LEN; i++)
	{
		ptr = MCNV2BUFF(phistory->_data[i].reltime);
		ptr = MCNV2BUFF(phistory->_data[i].rate);
	}
	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2history(UINT8 * pbuff, TDS_DATA_HISTORY * phistory, UINT8 * psize)
{
	int i;
	UINT8 *ptr = pbuff;

	for (i = (CDS_HISTORY_LEN - 1); i >= 0; i--)
	{
		ptr = MCNV2STRUCTR(phistory->_data[i].rate);
		ptr = MCNV2STRUCTR(phistory->_data[i].reltime);
	}
	ptr = MCNV2STRUCTR(phistory->actualreltime);
	return (ptr);
}
#endif

#ifdef CDS_MITPCA
#ifdef CDS_ENDIAN_PUMPE
static UINT8 *dspcaprot2buff(TDS_DATA_PCAPROT * ppcaprot, UINT8 * pbuff, UINT8 * psize)
{
	int i;
	UINT8 *ptr = pbuff;
	UINT16 event;
	ptr = MCNV2BUFF(ppcaprot->prtklsize);
	for (i = 0; i < CDS_PCAPROT_ELANZ; i++)
	{
		event = ppcaprot->_data[i].event;
		ptr = MCNV2BUFF(ppcaprot->_data[i].eventtime);
		ptr = MCNV2BUFF(ppcaprot->_data[i].event);
		ptr = MCNV2BUFF(ppcaprot->_data[i].alarme);

		if (event & CDS_PCAPROT_EV_IW_MASK)
		{
			/* Istwerte */
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.therapie_vol);
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.beoint_vol);
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.bolvol);
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.rate);
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.ad_verh);
			ptr = MCNV2BUFF(ppcaprot->_data[i]._data.iw.spr_vol);
		}
		else
		{
			if (event & CDS_PCAPROT_EV_SW_MASK)
			{
				/* Sollwerte */
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.basal_rate);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.lockout_time);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.beovol_limit);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.bol_vol);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.init_bol_vol);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.sw.dummy);
			}
			else
			{
				/* CFG-Werte */
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.cf.bolus_rate);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.cf.beoint_time);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.cf.einhkonz);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.cf.konz);
				ptr = MCNV2BUFF(ppcaprot->_data[i]._data.cf.dummy);
			}
		}
	}
	return (ptr);
}
#endif

#ifdef CDS_ENDIAN_LR
static UINT8 *dsbuff2pcaprot(UINT8 * pbuff, TDS_DATA_PCAPROT * ppcaprot, UINT8 * psize)
{
	int i;
	UINT8 *ptr = pbuff;
	ptr = MCNV2STRUCTF(ppcaprot->prtklsize);
	for (i = 0; i < CDS_PCAPROT_ELANZ; i++)
	{
		ptr = MCNV2STRUCTF(ppcaprot->_data[i].eventtime);
		ptr = MCNV2STRUCTF(ppcaprot->_data[i].event);
		ptr = MCNV2STRUCTF(ppcaprot->_data[i].alarme);
		if (ppcaprot->_data[i].event & CDS_PCAPROT_EV_IW_MASK)
		{
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.therapie_vol);
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.beoint_vol);
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.bolvol);
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.rate);
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.ad_verh);
			ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.iw.spr_vol);
		}
		else
		{
			if (ppcaprot->_data[i].event & CDS_PCAPROT_EV_SW_MASK)
			{
				/* Sollwerte */
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.basal_rate);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.lockout_time);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.beovol_limit);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.bol_vol);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.init_bol_vol);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.sw.dummy);
			}
			else
			{
				/* CFG-Werte */
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.cf.bolus_rate);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.cf.beoint_time);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.cf.einhkonz);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.cf.konz);
				ptr = MCNV2STRUCTF(ppcaprot->_data[i]._data.cf.dummy);
			}
		}
	}
	return (ptr);
}
#endif
#endif


#ifdef CDS_LITTLE_ENDIAN
union _cnv_u
{
	UINT32 ui32;
	UINT16 ui16;
	UINT8 ui8x2[2];
	UINT8 ui8x4[4];
};

static union _cnv_u cnv_buff;
#endif

#define MCNVIT( psrc, pdest, size )                     \
{                                                       \
  switch( size ) {                                      \
    case 1:                                             \
      *((UINT8 *)pdest) = *((UINT8 *)psrc);             \
      break;                                            \
    case 2:                                             \
      cnv_buff.ui16 = *((UINT16 *)psrc );               \
      *(((UINT8 *)pdest)+1) = cnv_buff.ui8x2[0];        \
      *((UINT8 *)pdest)   = cnv_buff.ui8x2[1];          \
      break;                                            \
    case 4:                                             \
      cnv_buff.ui32 = *((UINT32 *)psrc );               \
      *(((UINT8 *)pdest)+3) = cnv_buff.ui8x4[0];        \
      *(((UINT8 *)pdest)+2) = cnv_buff.ui8x4[1];        \
      *(((UINT8 *)pdest)+1) = cnv_buff.ui8x4[2];        \
      *((UINT8 *)pdest)   = cnv_buff.ui8x4[3];          \
      break;                                            \
    default:                                            \
      if((UINT8 *)pdest != (UINT8 *)psrc )              \
      {                                                 \
        memmove((UINT8 *)pdest,(UINT8 *)psrc, size );   \
      }                                                 \
      break;                                            \
  }                                                     \
}

static UINT8 *cnv2buff(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize)
{
	UINT8 *ptr;
#ifdef CDS_LITTLE_ENDIAN
	MCNVIT(psrc, pdest, size);
#else
	memmove(pdest, psrc, size);
#endif
	*psize += size;
	ptr = pdest + size;
	return (ptr);
}

static UINT8 *cnv2structr(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize)
{
	UINT8 *ptr;

	ptr = psrc - size;
#ifdef CDS_LITTLE_ENDIAN
	MCNVIT(ptr, pdest, size);
#else
	memmove(pdest, ptr, size);
#endif
	*psize += size;
	return (ptr);
}

#ifdef CDS_MITPCA
static UINT8 *cnv2structf(UINT8 * psrc, UINT8 * pdest, UINT8 size, UINT8 * psize)
{
	UINT8 *ptr;
#ifdef CDS_LITTLE_ENDIAN
	MCNVIT(psrc, pdest, size);
#else
	memmove(pdest, ptr, size);
#endif
	ptr = psrc + size;
	*psize += size;
	return (ptr);
}
#endif

/******************************************************************************
 * History:
 * $Log: dsendian.c $
 * Revision 1.12  2001/09/17 14:00:23  krughsde
 * MS-Portabilitaet hergestellt
 * Revision 1.11  2001/08/30 16:53:16  krughsde
 * Korrektur nach Test mit Pumpe
 * Revision 1.9  2001/08/27 14:32:28  krughsde
 * Protokollfuellstand und Bolusrate ergaenzt
 * Revision 1.8  2001/08/24 11:35:04  krughsde
 * PCA in ifdefs gepackt
 * Revision 1.7  2001/08/15 07:55:04  krughsde
 * PCA-Protokoll eingebaut
 * Revision 1.6  2000/02/25 11:52:20  krughsde
 * Reload-Segment implementiert
 * Revision 1.5  2000/01/21 12:50:54  krughsde
 * CA,CC-Rate Min/Max im Limitssegment ergaenzt
 * Revision 1.4  2000/01/18 07:33:00  krughsde
 * Segmentfehlererkennung korrigiert, hat Laengenfehler gemeldet
 * Revision 1.3  2000/01/13 07:54:50  krughsde
 * Anpassungen an C_10:
 * Aktion Alarm in AlarmAus umbenannt
 * ZeitEndeAlarm wieder eingefuehrt
 * LimSfSpritzenTropf in LimSfTropf umbenannt
 * BolVolAkt im Istwertsegment hinzugefuegt
 * Protokollversion auf 0x0d hochgezaehlt
 * Revision 1.2  1999/12/06 08:43:40  krughsde
 * Laengenpruefung ergaenzt,
 * resbol in Limits ergaenzt
 * Revision 1.1  1999/11/18 15:44:54  krughsde
 * Initial revision
 * Revision 1.6  1999/10/05 07:52:36  krughsde
 * bolvolmin ergaenzt
 * Revision 1.5  1999/10/04 11:29:20  krughsde
 * bolmin im Limitsegment ergaenzt
 * Revision 1.4  1999/09/30 14:50:40  krughsde
 * Aenderungen nach C_08 :
 * Istwert-, Sollwert-, Limit-, Statussegment geaendert
 * Revision 1.3  1999/09/28 14:55:42  krughsde
 * Header in dskp.h zusammengefasst
 * Revision 1.2  1999/09/07 09:05:04  krughsde
 * Optimierung fuer compact
 * Revision 1.1  1999/08/25 12:52:04  krughsde
 * Initial revision
 *
 *****************************************************************************/
