/******************************************************************************
 * $Id: dstarcom.h 1.44 2001/11/21 08:36:39 krughsde Serie $
 *
 * Copyright B.Braun Melsungen AG
 * Autor:		H.J.Krug
 * ----------------------------------------------------------------------------
 * Projekt:	   dianet - star
 * Inhalt:	   Segmentdefinitionen
 * ----------------------------------------------------------------------------
 *****************************************************************************/
#ifndef _COMDSTAR_INCLUDED
#define _COMDSTAR_INCLUDED

#include "up.h"
#include "dscomdef.h"
#include "dscomcrc.h"

/* paul included byte packing instructions for data transaction structures */
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

#ifndef PERF_IMPELLA
#define CDS_PROT_VERSION   0x000e	/* Protokollversion Standard */
#else
#define CDS_PROT_VERSION   0x010e	/* Protokollversion Perfusor Recover Fa.Impella */
#endif
/* MSB : ff Entwicklungsversion */
/* LSB : 0-9 dianet */
/* LSB : a-.. dianet - star */


#define CDS_BOLUSMAN_TIMEOUT	(1.2)	/* [Sekunden] DianetStar Bolus manuell Timeout */
#define CDS_CC_TIMEOUT (1.2)	/* [Sekunden] CC - Timeout */
#define CDS_COMP_FRAME_TOUT (0.4)	/* [Sekunden] Frame - Timeout fuer Compact */
#ifdef KP_COMPACT
#define CDS_FRAME_TIMEOUT (CDS_COMP_FRAME_TOUT)		/* [Sekunden] Frame - Timeout */
#else
#ifdef FP_COMPACT
#define CDS_FRAME_TIMEOUT (CDS_COMP_FRAME_TOUT)		/* [Sekunden] Frame - Timeout */
#else
#define CDS_FRAME_TIMEOUT (0.3)	/* [Sekunden] Frame - Timeout fuer den Rest */
#endif
#endif
#define CDS_USER_TIMEOUT (2.0)	/* [Sekunden] User Timeout, danach sind Write-Seg.gueltig */

/* defines fuer Kommunikation */
#define CDS_RAHMEN_BYTE		0x7E	/* Rahmenanfang / Rahmenende */
#define CDS_MONITOR_BYTE	0x80	/* Monitoraktivierungsbyte */
#define CDS_STUFF_BYTE		0x7D	/* stuffing erkennungs byte */
#define CDS_STUFFCODE_BYTE	0x20	/* Fuer exor bei Stuffing */

#define CDS_ADR_INVALID	   0	/* Kennzeichnung ungueltige Adresse      hc11 */
#define CDS_ADR_MIN		   'A'	/* erste gueltige Adresse */
#define CDS_ADR_MAX		   'Z'	/* letzte gueltige Adresse */

/* defines fuer Acknowledge */
#define CDS_EVENT_ACK	 0x06	/* ACK erkennungs Byte */
#define CDS_EVENT_NAK	 0x15	/* NAK erkennungs Byte */

/* defines fuer Segment AKTION   hc11 */
#define CDS_AKTION_EIN	 0x01	/* Funktion EIN */
#define CDS_AKTION_AUS	 0x0	/* Funktion AUS */
#define CDS_AKTION_NOP	 0xff	/* Funktion wird nicht veraendert */

/* defines fuer die Modi im Segment CC_MODE  (IST Pumpen Modus) */
#define CDS_MODE_DOKU		0x00	/* Startzustand Dokumentationsbetrieb */
#define CDS_MODE_FERNSTEUER 0x01	/* Fernsteuerbetrieb Bedienung an Pumpe erlaubt */
#define CDS_MODE_FERNREGEL	0x02	/* Fernregelbetrieb Bedienung an Pumpe nicht erlaubt */
#define CDS_MODE_MONREQ		0x03	/* Monitoraktivierung wird erwartet */
/* Ab hier interne Zustaende */
#define CDS_MODE_FERNSTOP	0x04	/* Bedienung an der Pumpe im Fernsteuerbetrieb */
#define CDS_MODE_DOKUSTOP	0x05	/* Bedienung an der Pumpe im Dokumentationsbetrieb */
#define CDS_MODE_CCOFF		0x06	/* Keine aktive Kommunikation, Startzustand */
#define CDS_MODE_CCOFFSTOP	0x07	/* Bedienung aus CCOFF */

/* defines fuer Fehlercodes im NAK - Paket */
#define CDS_ERR_NC			0x00	/* kein        error zur zeit vorhanden */
#define CDS_ERR_CRC			0x01	/* error code fuer CRC fehler  (Pruefsummenfehler) */
#define CDS_ERR_FRAME		0x02	/* error code fuer Rahmen fehler */
#define CDS_ERR_SEG			0x03	/* error code fuer Segmentkennung nicht vorhanden */
#define CDS_ERR_USER		0x04	/* error code fuer Bedienung durch Anwender */
#define CDS_ERR_IDENT		0x05	/* error code fuer Indertierte IDENT falsch */
#define CDS_ERR_RANGE		0x06	/* error code fuer Datenbereich ungueltig */
#define CDS_ERR_SEG_LENGTH	0x07	/* error code fuer laengen fehler */
#define CDS_ERR_ADRESS		0x08	/* error code fuer Adressen fehler */
#define CDS_ERR_MODE		0x09	/* error code fuer Pumpe im falschen mode  */
/* 0x0A ist frei */
#define CDS_ERR_UNIX		0x0B	/* error code fuer Setzen der Echtzeituhr im laufenden Betrieb */
#define CDS_ERR_AKTION		0x0C	/* error code fuer Aktion nicht durchfuehrbar */
#define CDS_ERR_CONTEXT		0x0D	/* error code Daten passen nicht zueinander */

#ifdef __WIN32__
#define CDS_ERR_STUFFING	0xF0	/* Stuffing Fehler im LR erkannt */

/* paul copied this one from D-Star sources:*/
#define CDS_ERR_TIMEOUT			0x0F
#define CDS_ERR_NOTCONNECTED	0x10
#define CDS_ERR_RS232_ERROR		0x20
#define CDS_ERR_CANNOT_OPEN		0x30
#define CDS_ERR_CANNOT_CLOSE	0x40

#endif

/* array groessen */
#define CDS_SWVER_LEN	   10	/* Ident:Softwareversionslaenge */
#define CDS_MEDNAME_LEN	   20	/* Sollwert:Medikamentennamenlaenge */
#define CDS_HISTORY_LEN	   10	/* History - Groesse, Anzahl d. Elemente */


/* Segment Kennungen  bei Aenderungen auch Meldungsgenerator aendern */
#define CDS_WSEG_NULL		0x00	/* Segment Typ -- write -- NULL Kennung (Pumpe hat keine gueltige K. bekommen) */
#define CDS_RSEG_IDENT		0x01	/* Segment Typ -- read -- Versionsinformationen */
#define CDS_RSEG_STATUS		0x02	/* Segment Typ -- read -- Statusinformationen */
#define CDS_RSEG_ISTWERT	0x03	/* Segment Typ -- read -- Istwertinformationen */
#define CDS_RSEG_SOLLWERT	0x04	/* Segment Typ -- read -- Sollwertinformationen */
#define CDS_RSEG_PROPOSAL	0x05	/* Segment Typ -- read -- Vorschlagsdaten */
#define CDS_RSEG_LIMITS		0x06	/* Segment Typ -- read -- Service Parameter */
#define CDS_RSEG_DOSIS		0x07	/* Segment Typ -- read -- Aktuellewerte (einheitn,konzentration,..) */
#define CDS_RSEG_HISTORY	0x08	/* Segment Typ -- read -- History-Daten */
#define CDS_RSEG_RELOAD 	0x09	/* Segment Typ -- read -- Reload-Daten */
#ifdef CDS_MITPCA
#define CDS_RSEG_PCAPROT        0x0a	/* Segment Typ -- read -- PCA - Protokoll - Daten */
#endif
#define CDS_WSEG_CC_MODE	0x10	/* Segment Typ -- write -- Fernsteuer betrieb einschalten */
#define CDS_WSEG_AKTION		0x11	/* Segment Typ -- write -- Service Parameter */
#define CDS_WSEG_SOLLWERT	0x12	/* Segment Typ -- write -- Sollwertinformationen */
#define CDS_WSEG_PROPOSAL	0x13	/* Segment Typ -- write -- Vorschlagsdaten */
#define CDS_WSEG_RELOAD 	0x14	/* Segment Typ -- write -- Reload-Daten */
#ifdef	CDS_MITDOSISWRITE
#define CDS_WSEG_DOSIS		0x15	/* Segment Typ -- write -- Aktuellewerte (einheitn,konzentration,..) */
#define CDS_LASTVALID_SEG	CDS_WSEG_DOSIS	/* Letzte gueltige Segmentkennung */
#else
#define CDS_LASTVALID_SEG	CDS_WSEG_RELOAD		/* Letzte gueltige Segmentkennung */
#endif
/* CDS_LASTVALID_SEG : bei Erweiterung immmer mit anpassen ! */

/* defines fuer Dosiskalkulation */
/* byte vergleich */
#define CDS_EINHEIT_NC			0x00	/* nicht benuzt */
#define CDS_EINHEIT_MG			0x20	/* Milligramm */
#define CDS_EINHEIT_UG			0x40	/* Mikrogramm */
#define CDS_EINHEIT_IE			0x60	/* IE */
#define CDS_EINHEIT_MMOL		0x80	/* Millimol */
#define CDS_EINHEIT_ML			0x08	/* Milliliter */
#define CDS_EINHEIT_KG			0x10	/* Kilogramm */
#define CDS_EINHEIT_SEC			0x01	/* Sekunden */
#define CDS_EINHEIT_MIN			0x02	/* Minuten */
#define CDS_EINHEIT_H			0x03	/* Stunden */
#define CDS_EINHEIT_24H			0x04	/* 24 Stunden */

/* Masken fuer CDS_DOSE*   hc11 */
#define CDS_EINHEIT_TIME_MSK   0x0F		/* Maske fuer Zeit */
#define CDS_EINHEIT_VOL_MSK	   0x18		/* Maske fuer Volumen */
#define CDS_EINHEIT_EINH_MSK   0xE0		/* Maske fuer Einheiten */


/******************************************************************************/
/* interne Aktionssegment Kennungen  */
#define CDS_AKT_START_STOP		0x00	/* Pumpe starten / stoppen */
#define CDS_AKT_ALARM_AUS		0x01	/* Alarme sperren / freigeben */
#define CDS_AKT_DATA_LOCK		0x02	/* Eingabe sperren / freigeben */
#define CDS_AKT_INFUSED_ZEIT_0	0x03	/* 1= loescht Gesamtzeit seit Einschalten */
#define CDS_AKT_INFUSED_VOL_0	0x04	/* 1= loescht Gesamtvolumen seit Einschalten */
#define CDS_AKT_BOL_MAN_START	0x05	/* startet Bolus fuer 1s / Stop Bolus */
#define CDS_AKT_BOL_VOL_START	0x06	/* startet Volumenbolus / Stop Bolus & loesche <BOL_VOL> */
#define CDS_AKT_KONTR_TROPF_EIN 0x07	/* Tropfueberwachung ein / aus */
#define CDS_AKT_STDBY			0x08	/* Standby-Funktion ein / aus */
#define CDS_AKT_HISTORY_CLEAR	0x09	/* 1= History-Speicher loeschen */
#define CDS_AKT_DOSIS_OFF		0x0a	/* 1=Dosiskalkulation ausschalten */
#define CDS_AKT_BOL_INTV_OFF	0x0b	/* 1=Intervallbolus ausschalten */
#define CDS_LASTVALID_AKT		CDS_AKT_BOL_INTV_OFF	/* Letzte gueltige Aktionskennung */
/* CDS_LASTVALID_AKT : bei Erweiterung immmer mit anpassen ! */

/******************************************************************************/
/****			 Segment interne Bit definitionen						   ****/
/******************************************************************************/
/* status - request   bit 0-2  char */
#define CDS_SOLLWERT_BIT_NR				0x00	/* Pumpen bedienung durch anwender,neue Sollwerte liegen vor */
#define CDS_LIMITS_BIT_NR				0x01	/* Das Limit Segment ist abzufragen */
#define CDS_ALARM_OFF_BIT_NR			0x02	/* Taste zur Alarmunterdrueckung wurde betaetigt */
#define CDS_DOSIS_BIT_NR				0x03	/* Das Dosis Segment ist abzufragen */

/******************************************************************************/
/* status - alarm bit 0-7  long */
#define CDS_SPRITZEN_ALARM_BIT_NR		0x00	/* Sammmelalarm aus Formschlussensor, .. */
#define CDS_SPRITZEN_VOR_ALARM_BIT_NR	0x01
/**/
#define CDS_SPRITZE_LEER_ALARM_BIT_NR	0x02 /**/
#define CDS_DRUCK_ALARM_BIT_NR			0x03	/* Sammelalarm aus Druckalarm im Bolusabbau, .. */
#define CDS_LUFT_ALARM_BIT_NR			0x04
/**/
#define CDS_TROPF_ALARM_BIT_NR			0x05 /**/
#define CDS_AKKU_LEER_ALARM_BIT_NR		0x06
/**/
#define CDS_AKKU_VOR_ALARM_BIT_NR		0x07 /**/
/* status - alarm bit 8-15   long */
#define CDS_PUMPENKLAPPE_AUF_ALARM_BIT_NR 0x08
/**/
#define CDS_RATE_UNGUELTIG_ALARM_BIT_NR 0x09 /* Einstellbereich wurde vom Anwender ueberschritten*/
#define CDS_ERINNER_ALARM_BIT_NR		0x0A	/* Erinnerungsalarm */
#define CDS_STANDBY_ENDE_ALARM_BIT_NR	0x0B	/* Zeit wurde abgearbeite */
#define CDS_VTBD_ENDE_ALARM_BIT_NR		0x0C	/* VTBD wurde abgearbeitet */
#define CDS_ZEIT_ENDE_ALARM_BIT_NR              0x0D	/* Zeitvorwahl wurde abgearbeitet */
#define CDS_CC_ALARRM_BIT_NR			0x0E	/* Kommunikationsfehler (wird nur vom fm - System verwendet */
#define CDS_KVO_ENDE_ALARM_BIT_NR		0x0F	/* KVO beendet */
/* status - alarm bit 16-17   long */
#define CDS_FERNREGEL_ALARM_BIT_NR		0x10	/* Taste start/stop im Fernregelbetrie betaetigt */
#define CDS_KVO_VORALARM_BIT_NR		0x11	/* Pumpe laeuft mit Keep Vene Open - Rate */
#define CDS_MINDRUCK_ALARM_BIT_NR   0x12	/* Mindestdruckunterschreitung ( Voralarm ) */
#define CDS_PCA_TASTER_ALARM_BIT_NR  0x13        /* PCA-Taster nicht angeschlossen */
#define CDS_PCA_HLIMVOL_ALARM_BIT_NR 0x14        /* Limit - Stunden-Volumen erreicht */
/* status - alarm bit 21 - 31 Frei */

/******************************************************************************/
/* status - zustand bit 0-7   long */
/* status - zustand bit 0 Frei */
#define CDS_CC_MODE_DOKU_BIT_NR			0x01	/* Pumpe ist im Dokumentationsmode */
#define CDS_CC_FERNSTEUER_BIT_NR		0x02	/* Pumpe ist im Fernsteuermode */
#define CDS_CC_FERNREGEL_BIT_NR			0x03	/* Pumpe ist im Fernregelmode */
#define CDS_NETZBETRIEB_STATUS_BIT_NR	0x04	/* Pumpe ist im Netzbetrieb */
#define CDS_STANDBY_STATUS_BIT_NR		0x05	/* Pumpe befindet sich im Standby */
#define CDS_RUN_READY_STATUS_BIT_NR		0x06	/* Pumpe kann gestartet werden */
#define CDS_RUN_STATUS_BIT_NR			0x07	/* Pumpe (Medikation) laeuft */
/* status - zustand bit 8-15    long */
#define CDS_BOLUS_READY_STATUS_BIT_NR	0x08	/* Bolus kann gestartet werden */
#define CDS_BOLUS_MAN_STATUS_BIT_NR		0x09	/* Bolus wird gegeben */
#define CDS_BOLUS_VOL_STATUS_BIT_NR		0x0A	/* Bolus mit Volumenvorwahl wird abgearbeitet */
#define CDS_BEDIENSTATUS_AKTIV_BIT_NR	0x0B	/* Bedienung aktiv */
#define CDS_BOLUS_INT_STATUS_BIT_NR		0x0C	/* Intervallbolus wird gegeben mit Bolrate */
#define CDS_KVO_STATUS_BIT_NR			0x0D	/* Pumpe laeuft mit Keep Vene Open - Rate */
#define CDS_ALARM_OFF_AKTIV_BIT_NR		0x0E	/* Alarm wird fuer 2 Minuten unterdrueckt, Timer ist aktiv */
#define CDS_AKUST_ALARM_ON_BIT_NR		0x0F	/* Akustischer Alarm ist eingeschaltet */
/*  status - zustand bit 16-20  long */
#define CDS_DATALOCK_STATUS_BIT_NR		0x10	/* Pumpe ist in der Bedienung blockiert */
#define CDS_TROPFUEB_OFF_BIT_NR			0x11	/* Tropfenueberwachung ist aktiviert */
#define CDS_HISTORY_BIT_NR				0x12	/* HISTORY - Speicher ist gefuellt */
#define CDS_VORLAUF_STATUS_BIT_NR		0x13	/* Schnellen Vorlauf zum Greifen der Spritze */
#define CDS_ANTR_SPRITZE_RUECK_BIT_NR	0x14	/* Antriebskopf wird zurueckgefahren */
#define CDS_STATUS_PROTOKOL_BIT_NR		0x15	/* Pumpen Fahrtenschreiber ist aktiv */
#define CDS_BOLUS_INT_AKTIV_BIT_NR		0x16	/* Intervallbolus konfiguriert */
#define CDS_DOSIS_AKTIV_BIT_NR			0x17	/* Dosiskalkulation aktiv */
#define CDS_RELOAD_AKTIV_BIT_NR                 0x18	/* Reload aktiv */
#define CDS_PIGGYBACK_AKTIV_BIT_NR              0x19	/* Piggyback aktiv */
#define CDS_PCA_AKTIV_BIT_NR                    0x1A    /* PCA aktiv */
#define CDS_PROFIL_AKTIV_BIT_NR                 0x1B    /* Profil aktiv */
#define CDS_RAMPTAPER_AKTIV_BIT_NR              0x1C    /* Ramp-Taper aktiv */
#define CDS_DELAY_AKTIV_BIT_NR                  0x1D    /* Delay aktiv */
/* status bits 0x1d - 0x1f ( 30 - 31 )  sind frei */

/* START ******************* Entwicklungshilfe ********************************** */
#ifndef _lint
struct _ds_status_zustand_low	/* debug flagstructur STATUS ZUSTAND */
{
	unsigned int smonitor:1;	/*0 */
	unsigned int doku:1;		/*1 */
	unsigned int fsteuer:1;		/*2 */
	unsigned int fregel:1;		/*3 */
	unsigned int netz:1;		/*4 */
	unsigned int standby:1;		/*5 */
	unsigned int runready:1;	/*6 */
	unsigned int run_stat:1;	/*7 */
	unsigned int bol_read:1;	/*8 */
	unsigned int bol_man_:1;	/*9 */
	unsigned int bol_vol_:1;	/*10 */
	unsigned int bedienst:1;	/*11 */
	unsigned int bol_int:1;		/*12 */
	unsigned int kvo_stat:1;	/*13 */
	unsigned int alarm_of:1;	/*14 */
	unsigned int akust_al:1;	/*15 */
};

typedef struct _ds_status_zustand_low TDS_STATUS_ZUSTAND_LOW;

struct _ds_status_zustand_high	/* debug flagstructur STATUS ZUSTAND */
{
	unsigned int datalock:1;	/*16 */
	unsigned int tropfueb:1;	/*17 */
	unsigned int history_:1;	/*18 */
	unsigned int vorlauf_:1;	/*19 */
	unsigned int antr_spr:1;	/*20 */
	unsigned int st_proto:1;	/*21 */
	unsigned int bol_int_:1;	/*22 */
	unsigned int dosis_ak:1;	/*23 */
	unsigned int reload:1;		/*24 */
	unsigned int piggyback:1;	/*25 */
        unsigned int pcaaktiv:1;        /*26 */
        unsigned int profilaktiv:1;     /*27 */
        unsigned int ramptaperaktiv:1;  /*28 */
        unsigned int delayaktiv:1;      /*29 */
};
typedef struct _ds_status_zustand_high TDS_STATUS_ZUSTAND_HIGH;


struct _ds_status_zustand		/* debug flagstructur STATUS ZUSTAND */
{
	TDS_STATUS_ZUSTAND_HIGH high;	/* */
	TDS_STATUS_ZUSTAND_LOW low;	/* */
};
typedef struct _ds_status_zustand TDS_STATUS_ZUSTAND;

struct _ds_status_alarm_low
{
	unsigned int sp_alarm:1;	/*0 */
	unsigned int sp_voral:1;	/*1 */
	unsigned int sp_leera:1;	/*2 */
	unsigned int druck_al:1;	/*3 */
	unsigned int luft_al_:1;	/*4 */
	unsigned int tropf_al:1;	/*5 */
	unsigned int akku_lee:1;	/*6 */
	unsigned int akku_vor:1;	/*7 */
	unsigned int pumpenkl:1;	/*8 */
	unsigned int rate_ung:1;	/*9 */
	unsigned int erinner_:1;	/*10 */
	unsigned int standby_:1;	/*11 */
	unsigned int vtbd_end:1;	/*12 */
	unsigned int zeit_end:1;	/*13 */
	unsigned int cc_alarm:1;	/*14 */
	unsigned int kvo_ende:1;	/*15 */
};
typedef struct _ds_status_alarm_low TDS_STATUS_ALARM_LOW;

struct _ds_status_alarm_high
{
	unsigned int fernrege:1;	/*16 */
	unsigned int kvo_vora:1;	/*17 */
	unsigned int mindruck:1;	/*18 */
	unsigned int pcataster:1;	/*19 */
	unsigned int pcahlimvol:1;	/*20 */
};

typedef struct _ds_status_alarm_high TDS_STATUS_ALARM_HIGH;

struct _ds_status_alarm			/* debug flagstructur STATUS ZUSTAND */
{
	TDS_STATUS_ALARM_HIGH high;	/* */
	TDS_STATUS_ALARM_LOW low;	/* */
};

typedef struct _ds_status_alarm TDS_STATUS_ALARM;

#endif
/* ENDE ******************* Entwicklungshilfe ********************************** */


/******************************************************************************/
/*  limits - personal   bit 0-3  char */
#define CDS_STATISCH_BIT_NR				0x00	/* Statisch */
#define CDS_DYNAMISCH_BIT_NR			0x01	/* Dynamisch */
#define CDS_AUSALARM_BIT_NR				0x02	/* Ausalarm */
#define CDS_TESTIMPULS_BIT_NR			0x03	/* Testimpuls */
#define CDS_VORALARM_BIT_NR                     0x04	/* Voralarm */

/*  limits - SF   bit 0-7  long */
#define CDS_LIM_SF_BOLUS_BIT_NR			0x00	/* Bolus */
#define CDS_LIM_SF_STANDBY_BIT_NR		0x01	/* Standby - Funktion */
#define CDS_LIM_SF_DRUCK_BIT_NR			0x02	/* Druckberenzung */
#define CDS_LIM_SF_DATALOCK_BIT_NR		0x03	/* Data Lock */
#define CDS_LIM_SF_TROPF_BIT_NR                 0x04	/* Tropfkontrolle bei Schlauch-Pumpen */
#define CDS_LIM_SF_DOSIS_BIT_NR			0x05	/* Dosiskalkuation */
/*  limits - SF   bit 8-31  Frei */


/* personal zustaende auf byte vergleich */
#define CDS_PERSONAL_0	0x05
/**/
#define CDS_PERSONAL_1	0x02 /**/
#define CDS_PERSONAL_2	0x06
/**/
#define CDS_PERSONAL_3	0x0D /**/
#define CDS_PERSONAL_4	0x0A
/**/
#define CDS_PERSONAL_5	0x0E /**/


/* Aenderungsbittyp fuer Sollwerte, Proposal, Dosis und Reload */
typedef UINT8 TDS_VAL_CHANGE;

/******************************************************************************/
/****			   Strukturen der Datenkommunikation (Segmente)			   ****/
/******************************************************************************/

/* Datenkopf der Kommunikation Rechner -> Pumpe */
struct _dstar_recv_head
{
	UINT8 adress;				/* Adresse */
	UINT8 segnr;				/* Segmentnummer */
};

typedef struct _dstar_recv_head TDS_RECV_HEAD;

/* Datenkopf der Kommunikation Pumpe -> Rechner */
struct _dstar_reply_head
{
	UINT8 adress;				/* Adresse */
	UINT8 segnr;				/* Segmentnummer */
	UINT8 _code;				/* ACK / NAK */
};

typedef struct _dstar_reply_head TDS_REPLY_HEAD;

/* NAK - Struktur Pumpe -> Rechner */
struct _dstar_data_nak
{
	UINT8 errcode;				/* Fehlercode    */
};

typedef struct _dstar_data_nak TDS_DATA_NAK;

/* AKTION - Struktur Rechner -> Pumpe */
struct _dstar_data_aktion
{
	UINT8 kennung;				/* Aktionskennung */
	UINT8 flag;					/* 0/1 = die in kennung definierte aktion auf den neuen Zustand setzen  */
};

typedef struct _dstar_data_aktion TDS_DATA_AKTION;

/* CC_MODE - Struktur Rechner -> Pumpe */

struct _dstar_data_ccmode
{
	UINT32 identnrinv;			/* invertierte Identnummer */
	UINT8 modeswitch;			/* Neuer Modus CDS_MODE_* */
	UINT8 newadress;			/* Neue Adresse */
	UINT32 unixtime;			/* Zu setzende Uhrzeit im unix Format [s] seit 1.1.70 */
};

typedef struct _dstar_data_ccmode TDS_DATA_CCMODE;

/* IDENT - Struktur Pumpe -> Rechner */
struct _dstar_data_ident
{
	UINT32 identnr;				/* IDENT - Nummer */
	UINT16 protovers;			/* Protokollversion */
	char swvers[CDS_SWVER_LEN];	/* Softwareversion ohne 0 ! */
	UINT32 unixtime;			/* Aktuelle Uhrzeit im unix Format [s] seit 1.1.70 */
};

typedef struct _dstar_data_ident TDS_DATA_IDENT;

/* ISTWERT - Struktur Pumpe -> Rechner */
struct _dstar_data_istwert
{
	UINT8 druckwert;			/* Druckwert 0-100 Prozent der Druckstufe */
	UINT32 infusedvol;			/* Infudiertes Volumen [0.1ml] */
	UINT16 infusedtime;			/* Behandlungszeit [min] */
	UINT16 bolintvrestzeit;		/* Restzeit der Bolusintervallzeit [min] */
	UINT16 akkurest;			/* Akku-Restkapazitaet [min] */
	UINT16 standbyrest;			/* restliche standbyzeit [min] */
	UINT16 syringevol;			/* erkanntes Spritzenvolumen [ml] */
	UINT16 bolintvzeit;			/* Bolusintervallzeit [min] */
	UINT16 bolintvvol;			/* Bolusintervallvolumen [0.1ml] */
	UINT16 bolvolakt;			/* aktuelles Bolusvolumen [0.1ml] */
};

typedef struct _dstar_data_istwert TDS_DATA_ISTWERT;

/* LIMITS - Struktur Pumpe -> Rechner */
struct _dstar_data_limits
{
	UINT32 bolmax;				/* max.Bolusrate [0.01ml/h] */
	UINT8 personal;				/* Personalruftyp */
	UINT8 resdisp;				/* Anzahl Nachkommastellen Display */
	UINT32 ratemax;				/* Max.Rate [0.01ml/h] */
	UINT32 ratemin;				/* Min.Rate [0.01ml/h] */
	UINT8 spritzevoral;			/* Spritzenvoralarm [min] */
	UINT32 sonderfkt;			/* Sonderfunktionen */
	UINT8 druckmax;				/* Max.Druckstufe [1..9] */
	UINT32 vtbdmax;				/* max.Foerdervolumen [0.1ml] */
	UINT16 bolvolmax;			/* max.Bolusvolumen [0.1ml] */
	UINT32 bolmin;				/* min.Bolusrate [0.01ml/h] */
	UINT16 bolvolmin;			/* min.Bolusvolumen [0.1ml] */
	UINT8 resbol;				/* Anzahl Nachkommastellen der Bolusrate */
	UINT32 caratemax;			/* Max.Rate im CA-Betrieb [0.01ml/h] */
	UINT32 caratemin;			/* Min.Rate im CA-Betrieb [0.01ml/h] */
	UINT32 ccratemax;			/* Max.Rate im CC-Betrieb [0.01ml/h] */
	UINT32 ccratemin;			/* Min.Rate im CC-Betrieb [0.01ml/h] */
};

typedef struct _dstar_data_limits TDS_DATA_LIMITS;

/* STATUS - Struktur Pumpe -> Rechner */
struct _dstar_data_status
{
	UINT8 request;				/* Abfrage spez.Segmente notwendig */
	UINT32 alarm;				/* Akuelle Alarme */
	UINT32 zustand;				/* Aktueller Zustand */
	UINT32 rate;				/* Aktuelle Rate [ 10µl/h] */
	UINT32 remainvtbd;			/* Volumen seit dem letzten Setzen von vtbd [0.1ml] */
	UINT16 remaintime;			/* Zeit seit dem letzten Setzen von vtbd [min] */
};

typedef struct _dstar_data_status TDS_DATA_STATUS;

/* Sollwert - Paket Rechner <-> Pumpe */
struct _dstar_data_sollwert
{
	UINT32 rate;				/* Rate [0.01ml/h] */
	UINT32 vtbd;				/* zu infudierendes Volumen [0.1ml] */
	UINT16 time;				/* Behandlungsdauer [min] */
	UINT32 bolrate;				/* Bolusrate [0.01ml/h */
	UINT16 bolvol;				/* Bolusvolumen [0.1ml] */
	UINT8 drucklimitmax;		/* Druckstufe 1..9 */
	UINT16 stdby;				/* standby-Zeit [min] */
	INT8 medikament[CDS_MEDNAME_LEN];	/* Medikamentenname ohne 0 ! */
};

typedef struct _dstar_data_sollwert TDS_DATA_SOLLWERT;

#define CDS_SW_WRITE_RATE_BIT_NR	0	/* Rate wurde geschrieben */
#define CDS_SW_WRITE_VTBD_BIT_NR	1	/* VTBD wurde geschrieben */
#define CDS_SW_WRITE_TIME_BIT_NR	2	/* TIME wurde geschrieben */
#define CDS_SW_WRITE_BOLRATE_BIT_NR 3	/* BOLRATE wurde geschrieben */
#define CDS_SW_WRITE_BOLVOL_BIT_NR	4	/* BOLVOL wurde geschrieben */
#define CDS_SW_WRITE_DRUCK_BIT_NR	5	/* DRUCK wurde geschrieben */
#define CDS_SW_WRITE_STDBY_BIT_NR	6	/* STDBY wurde geschrieben */
#define CDS_SW_WRITE_MED_BIT_NR		7	/* MEDIKAMENT wurde geschrieben */
/* Bei mehr als 8 bits muss das typedef TDS_SW_CHANGE angepasst werden ! */

/* Vorschlagsdaten - Paket Rechner <-> Pumpe */
struct _dstar_data_proposol
{
	INT8 medikament[CDS_MEDNAME_LEN];	/* Medikamentenname ohne 0 ! */
	UINT32 rate;				/* Vorschlagsrate [0.01ml/h] */
};

typedef struct _dstar_data_proposol TDS_DATA_PROPOSAL;

#define CDS_PROP_WRITE_MED_BIT_NR	0	/* Medikament wurde geschrieben */
#define CDS_PROP_WRITE_RATE_BIT_NR	1	/* Rate wurde geschrieben */
/* Bei mehr als 8 bits muss das typedef TDS_SW_CHANGE angepasst werden ! */

/* Reloadwerte - Paket Rechner <-> Pumpe */
struct _dstar_data_reload
{
	UINT8 aktiv;				/* 1: Reloadwerte liegen vor */
	UINT32 rate;				/* Rate [0.01ml/h] */
	UINT32 vtbd;				/* zu infudierendes Volumen [0.1ml] */
};

typedef struct _dstar_data_reload TDS_DATA_RELOAD;

#define CDS_RELOAD_WRITE_AKT_BIT_NR     0	/* Aktiv-Kennung hat sich geaendert */
#define CDS_RELOAD_WRITE_RATE_BIT_NR    1	/* Rate wurde geschrieben */
#define CDS_RELOAD_WRITE_VTBD_BIT_NR    2	/* VTBD wurde geschrieben */

/* Dosiskalkulation - Paket Rechner <-> Pumpe */
struct _dstar_data_dosis
{
	UINT16 kg;					/* Koerpergewicht [10g] */
	UINT32 konz;				/* Konzentration [0.001 <Einheiten> bei IE = 1 ] */
	UINT8 einhkonz;				/* Einheit der Konzentration */
	UINT32 rate;				/* Dosisrate [0.001 <Einheiten> bei IE = 1 ] */
	UINT8 einhrate;				/* Einheit der rate */
	UINT32 gebvol;				/* Gebindevolumen [0.01 ml] */
	UINT32 drug;				/* Wirkstoffmenge im Gebindevolumen [0.001 <Einheiten> bei IE = 1 ] */
	UINT8 einhdrug;				/* Einheit fuer Wirkstoffmenge */
};

typedef struct _dstar_data_dosis TDS_DATA_DOSIS;

#define CDS_DOS_WRITE_KG_BIT_NR		  0		/* Gewicht wurde geschrieben */
#define CDS_DOS_WRITE_KONZ_BIT_NR	  1		/* Konz. wurde geschrieben */
#define CDS_DOS_WRITE_EHKONZ_BIT_NR	  2		/* Einheit d. Konz. wurde geschrieben */
#define CDS_DOS_WRITE_RATE_BIT_NR	  3		/* Rate wurde geschrieben */
#define CDS_DOS_WRITE_EHRATE_BIT_NR	  4		/* Einheit der Rate wurde geschrieben */
#define CDS_DOS_WRITE_GEBVOL_BIT_NR	  5		/* Gebindevol. wurde geschrieben */
#define CDS_DOS_WRITE_DRUG_BIT_NR	  6		/* Drug wurde geschrieben */
#define CDS_DOS_WRITE_EHDRUG_BIT_NR	  7		/* Einheit v.Drug wurde geschrieben */
/* Bei mehr als 8 bits muss das typedef TDS_SW_CHANGE angepasst werden ! */

struct _dstar_data_his_el
{
	UINT16 reltime;				/* relative Zeit seit Einschalten des Ereignisses */
	UINT32 rate;				/* Rate zum Zeitpunkt reltime */
};

typedef struct _dstar_data_his_el TDS_DATA_HIS_EL;


struct _dstar_data_history
{
	UINT16 actualreltime;		/* relative aktuelle Zeit seit Einschalten */
	TDS_DATA_HIS_EL _data[CDS_HISTORY_LEN];		/* History - Daten */
};

typedef struct _dstar_data_history TDS_DATA_HISTORY;

#ifdef CDS_MITPCA
#define CDS_PCAPROT_EV_READ_FIRST_USR   1	/* Leseauftrag das erste Element, nur nicht gelöschte */
#define CDS_PCAPROT_EV_READ_FIRST_ALL   2	/* Leseauftrag das erste Element, alle */
#define CDS_PCAPROT_EV_READ_NEXT_USR    3	/* Leseauftrag Folgeelement, nur nicht gelöschte */
#define CDS_PCAPROT_EV_READ_NEXT_ALL    4	/* Leseauftrag Folgeelement, alle */
#define CDS_PCAPROT_EV_READ_SAME        5	/* Leseauftrag das gleiche Element nocheinmal, z.B. fuer CRC-Fehler */
#define CDS_PCAPROT_FIRST_EVENT  CDS_PCAPROT_EV_READ_FIRST_USR	/* Fuer Rangecheck min */
#define CDS_PCAPROT_LAST_EVENT   CDS_PCAPROT_EV_READ_SAME	/* Fuer Rangecheck max */


struct _dstar_data_pcaprot_read
{
	UINT8 read_event;			/* Leseauftrag, CDS_PCAPROT_EV_READ_*  */
};

typedef struct _dstar_data_pcaprot_read TDS_DATA_PCAPROT_READ;


/* PCA - Protokoll - alarm bit 0-16  int */
#define CDS_PCA_SPRITZEN_ALARM_BIT_NR		0x00	/* Sammmelalarm aus Formschlussensor, .. */
#define CDS_PCA_SPRITZEN_VOR_ALARM_BIT_NR	0x01
#define CDS_PCA_SPRITZE_LEER_ALARM_BIT_NR	0x02 /**/
#define CDS_PCA_DRUCK_ALARM_BIT_NR		0x03	/* Sammelalarm aus Druckalarm im Bolusabbau, .. */
#define CDS_PCA_AKKU_LEER_ALARM_BIT_NR		0x04
#define CDS_PCA_AKKU_VOR_ALARM_BIT_NR		0x05 /**/
#define CDS_PCA_STANDBY_ENDE_ALARM_BIT_NR	0x06	/* Zeit wurde abgearbeite */
#define CDS_PCA_PCA_TASTER_ALARM_BIT_NR         0x07        /* PCA-Taster nicht angeschlossen */
#define CDS_PCA_PCA_HLIMVOL_ALARM_BIT_NR        0x08        /* Limit - Stunden-Volumen erreicht */

struct _dstar_data_pcaprot_iw
{
	UINT16 therapie_vol;		/* [0.1ml] Inf.Volumen in der Therapie */
	UINT16 beoint_vol;			/* [0.1ml] Beobachtungsintervallvolumen */
	UINT16 bolvol;				/* [0.1ml] Letztes Bolusvolumen */
	UINT16 rate;				/* [0.1ml/h] aktuelle Rate */
	UINT8 ad_verh;				/* [%] A/D-Verhaeltnis */
	UINT8 spr_vol;				/* [ml] Spritzenvolumen */
};

typedef struct _dstar_data_pcaprot_iw TDS_DATA_PCAPROT_IW;

/* Dosiseinheiten fuer PCA -SW einhkonz */
#define CDS_PCA_DOSIS_ML         1 /* Keine Dosiseinheit */
#define CDS_PCA_DOSIS_MG         2 /* x mg/ml */
#define CDS_PCA_DOSIS_UG         3 /* x ug/ml */

struct _dstar_data_pcaprot_sw
{
	UINT16 basal_rate;			/* [0.1ml/h] Basalrate */
	UINT16 lockout_time;		/* [min] Lockoutzeit */
	UINT16 beovol_limit;		/* [0.1ml] Volumenlimit im Beobachtungsintervall */
	UINT8 bol_vol;				/* [0.1ml] Bolusvolumen */
	UINT8 init_bol_vol;			/* [0.1ml] Initialbolusvolumen */
        UINT16 dummy;
};

typedef struct _dstar_data_pcaprot_sw TDS_DATA_PCAPROT_SW;

struct _dstar_data_pcaprot_cf
{
        UINT16 bolus_rate;                      /* [0.1ml/h] Bolusrate */
	UINT8 beoint_time;			/* [h] Beobachtungsintervall */
	UINT8 einhkonz;				/* Einheit der Konzentration */
	UINT32 konz;				/* Konzentration [0.001 <Einheiten>] */
        UINT16 dummy;
};

typedef struct _dstar_data_pcaprot_cf TDS_DATA_PCAPROT_CF;

union _dstar_pcaprot_u
{
	TDS_DATA_PCAPROT_IW iw;
	TDS_DATA_PCAPROT_SW sw;
	TDS_DATA_PCAPROT_CF cf;
};

typedef union _dstar_pcaprot_u TDS_PCAPROT_U;

#define CDS_PCAPROT_ELANZ 3		/* Elemente, die gleichzeitig gelesen werden */

/* Bitdefinitionen der PCA-Protokoll-Events */
#define CDS_PCAPROT_EV_IW_PCA_READY_BIT    0	/* 1: Spritze vorhanden, foerderbereit */
#define CDS_PCAPROT_EV_IW_PCA_RUN_BIT      1	/* 1: PCA aktiv, Infusion laeuft */
#define CDS_PCAPROT_EV_IW_PCA_STOP_BIT     2	/* 1: PCA aktiv, Infusion gestoppt */
#define CDS_PCAPROT_EV_IW_BOL_START_BIT    3	/* 1: Bolus Start */
#define CDS_PCAPROT_EV_IW_BOL_STOP_BIT     4	/* 1: Bolus Stop */
#define CDS_PCAPROT_EV_IW_BOL_REFUSED_BIT  5	/* 1: Bolus abgelehnt */
#define CDS_PCAPROT_EV_IW_TASTER_BIT       6	/* 1: Taster betaetigt */
#define CDS_PCAPROT_EV_IW_H_LIMIT_BIT      7	/* 1: Stundenlimit erreicht */
#define CDS_PCAPROT_EV_SW_STBY_EIN_BIT     8	/* 1: Standby EIN */
#define CDS_PCAPROT_EV_SW_STBY_AUS_BIT     9	/* 1: Standby AUS */
#define CDS_PCAPROT_EV_SW_PCA_EIN_BIT     10	/* 1: PCA eingeschaltet */
#define CDS_PCAPROT_EV_SW_PCA_AUS_BIT     11	/* 1: PCA ausgeschaltet */
#define CDS_PCAPROT_EV_SW_PROT_CLR_BIT    12	/* 1: Protokoll wurde gelöscht */
#define CDS_PCAPROT_EV_SW_TIMESET_BIT     13	/* 1: Uhrzeit wurde gesetzt */
#define CDS_PCAPROT_EV_SW_CHANGE_SW_BIT   14	/* 1: Neue Sollwerte */
#define CDS_PCAPROT_EV_CF_CHANGE_CF_BIT   15	/* 1: Neue Konfiguration */

#define CDS_PCAPROT_EV_IW_MASK            0x00FF	/* Event-Maske fuer Istwerte */
#define CDS_PCAPROT_EV_SW_MASK            0x7F00	/* Event-Maske fuer Sollwerte */
#define CDS_PCAPROT_EV_CF_MASK            0x8000        /* Event-Maske fuer Conf.werte */

struct _dstar_data_pca_prot_element
{
	UINT32 eventtime;			/* Event-Uhrzeit im unix Format [s] seit 1.1.70 */
	UINT16 event;				/* Event-Typ, Bitfeld CDS_PCAPROT_EV_*  */
	UINT16 alarme;				/* Alarme, Druck, Spritze... */
	TDS_PCAPROT_U _data;		/* Daten SW oder IW */
};

typedef struct _dstar_data_pca_prot_element TDS_PCAPROT_ELEMENT;

struct _dstar_data_pca_protokoll
{
        UINT16 prtklsize;               /* [n] Anzahl Protokolleintraege */
	TDS_PCAPROT_ELEMENT _data[CDS_PCAPROT_ELANZ];	/* Protokolldaten */
};

typedef struct _dstar_data_pca_protokoll TDS_DATA_PCAPROT;
#endif

/* Empfangs - Union */
union _dstar_recv_u
{
	TDS_DATA_AKTION aktion;		/* AKTION - Paket */
	TDS_DATA_CCMODE ccmode;		/* CC_MODE - Paket */
	TDS_DATA_SOLLWERT sollwert;	/* SOLLWERT - Paket */
	TDS_DATA_PROPOSAL proposal;	/* PROPOSAL - Paket */
	TDS_DATA_DOSIS dosis;		/* DOSIS - Paket */
	TDS_DATA_RELOAD reload;		/* RELOAD - Paket */
#ifdef CDS_MITPCA
	TDS_DATA_PCAPROT_READ pcaprotread;	/* PCA-Protokoll - Read - Paket */
#endif
};

typedef union _dstar_recv_u TDS_RECV_U;

/* Sende - Union */
union _dstar_reply_u
{
	TDS_DATA_NAK nak;			/* NAK-Paket */
	TDS_DATA_IDENT ident;		/* IDENT - Paket */
	TDS_DATA_STATUS status;		/* STATUS - Paket */
	TDS_DATA_ISTWERT istwert;	/* Istwert - Paket */
	TDS_DATA_LIMITS limits;		/* Limits - Paket */
	TDS_DATA_SOLLWERT sollwert;	/* SOLLWERT - Paket */
	TDS_DATA_PROPOSAL proposal;	/* PROPOSAL - Paket */
	TDS_DATA_DOSIS dosis;		/* DOSIS - Paket */
	TDS_DATA_HISTORY history;	/* HISTORY - Paket */
	TDS_DATA_RELOAD reload;		/* RELOAD - Paket */
#ifdef CDS_MITPCA
	TDS_DATA_PCAPROT pcaprot;	/* PCAPROT - Paktet */
#endif
};

typedef union _dstar_reply_u TDS_REPLY_U;

/* Empfangsstruktur */
struct _dstar_recv
{
	TDS_RECV_HEAD head;			/* Gemeinsame Daten */
	TDS_RECV_U _data;			/* Daten */
};

typedef struct _dstar_recv TDS_RECV;

/* Sendestruktur */
struct _dstar_reply
{
	TDS_REPLY_HEAD head;		/* Gemeinsame Daten */
	TDS_REPLY_U _data;			/* Daten */
};

typedef struct _dstar_reply TDS_REPLY;

union _dstar_recv_msg_u
{
	TDS_RECV msg;
	UINT8 buffer[sizeof(TDS_RECV) + sizeof(TDS_CRC)];
};

typedef union _dstar_recv_msg_u TDS_RECVMSG_U;

union _dstar_reply_msg_u
{
	TDS_REPLY msg;
	UINT8 buffer[sizeof(TDS_REPLY)];
};

typedef union _dstar_reply_msg_u TDS_REPLYMSG_U;

/* Fuer die Benutzung unter Win32 muessen die Strukturgroessen anders berechnet */
/* werden um das Strukturalignment zu beruecksichtigen, die Groessen stimmen */
/* dann aber nicht mit den Groessen der Pumpe ueberein !!! */
#ifndef __WIN32__
#define CDS_RECV_HEAD_SIZE	   sizeof( TDS_RECV_HEAD )
#define CDS_REPLY_HEAD_SIZE	   sizeof( TDS_REPLY_HEAD )
#else
#define CDS_RECV_HEAD_SIZE	   ( sizeof( TDS_RECV ) - sizeof( TDS_RECV_U ) )
#define CDS_REPLY_HEAD_SIZE	   ( sizeof( TDS_REPLY ) - sizeof( TDS_REPLY_U ) )
#endif

/* Empfangsgroessen */
#define CDS_RECV_MAXLEN			  ( sizeof( TDS_RECVMSG_U ) )
#define CDS_RECV_READ_LEN	   ( CDS_RECV_HEAD_SIZE )
#define CDS_RECV_AKTION_LEN	   ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_AKTION ) )
#define CDS_RECV_CCMODE_LEN	   ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_CCMODE ) )
#define CDS_RECV_SOLLWERT_LEN  ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_SOLLWERT ) )
#define CDS_RECV_PROPOSAL_LEN  ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_PROPOSAL ) )
#define CDS_RECV_DOSIS_LEN	   ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_DOSIS ) )
#define CDS_RECV_RELOAD_LEN        ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_RELOAD ) )
#ifdef CDS_MITPCA
#define CDS_RECV_PCAPROT_LEN       ( CDS_RECV_HEAD_SIZE + sizeof( TDS_DATA_PCAPROT_READ ) )
#endif
/* Sendegroessen */
#define CDS_REPLY_MAXLEN	   ( sizeof( TDS_REPLYMSG_U ) )
#define CDS_REPLY_ACK_LEN	   ( CDS_REPLY_HEAD_SIZE )
#define CDS_REPLY_NAK_LEN	   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_NAK ) )
#define CDS_REPLY_IDENT_LEN	   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_IDENT ) )
#define CDS_REPLY_ISTWERT_LEN  ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_ISTWERT ) )
#define CDS_REPLY_LIMITS_LEN   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_LIMITS ) )
#define CDS_REPLY_STATUS_LEN   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_STATUS ) )
#define CDS_REPLY_SOLLWERT_LEN ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_SOLLWERT ) )
#define CDS_REPLY_PROPOSAL_LEN ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_PROPOSAL ) )
#define CDS_REPLY_DOSIS_LEN	   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_DOSIS ) )
#define CDS_REPLY_HISTORY_LEN  ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_HISTORY ) )
#define CDS_REPLY_RELOAD_LEN   ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_RELOAD ) )
#ifdef CDS_MITPCA
#define CDS_REPLY_PCAPROT_LEN  ( CDS_REPLY_HEAD_SIZE + sizeof( TDS_DATA_PCAPROT ) )
#endif

/* FMEA - Teststruktur */
struct _dstar_fmea_data			/* Flags fuer fmea */
{
	TBOOL NoCRCCheck;			/* TRUE: Keine CRC - Pruefung */
	TBOOL NoRangeCheck;			/* TRUE: Kein Rangecheck bei schreibenden Segmenten */
	TBOOL FalseSeg;				/* TRUE: Segment CDS_WSEG_SOLLWERT wird in CDS_WSEG_PROPOSAL veraendert */
};

typedef struct _dstar_fmea_data TDS_FMEA_DATA;	/* Flags fuer fmea */


/* paul included byte packing instructions for data transaction structures */
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif


/******************************************************************************
 * History:
 * $Log: dstarcom.h $
 * Revision 1.44  2001/11/21 08:36:39  krughsde
 * DELAY_AKTIV im Status eingebaut
 * Revision 1.43  2001/11/21 07:37:35  krughsde
 * defines fuer Statussegment Profil und Ramptaper eingefuegt
 * Revision 1.42  2001/08/28 08:38:44  krughsde
 * PCA-Sollwerte in SW und CF aufgeteilt
 * Revision 1.41  2001/08/27 14:29:52  krughsde
 * Protokollfuellstand und Bolusrate ergaenzt
 * Revision 1.40  2001/08/24 14:27:44  krughsde
 * PCA-ifdefs erganzt,
 * PCA-Dosis und Alarme ergaenzt
 * Revision 1.39  2001/08/17 11:20:42  krughsde
 * Schreibfehler korrigiert
 * Revision 1.38  2001/08/17 10:48:05  krughsde
 * PCA-Protokoll-Segment eingebaut
 * Revision 1.36  2000/09/11 09:29:20  krughsde
 * User-Timeout auf 2 sec reduziert
 * Revision 1.35  2000/07/04 09:37:12  krughsde
 * Frametimeout auf 300ms reduziert
 * Revision 1.34  2000/04/07 10:10:50  krughsde
 * Var FalseSeg in Struktur FmeaData ergaenzt
 * Revision 1.33  2000/04/05 11:17:14  krughsde
 * typedef vergessen
 * Revision 1.32  2000/04/05 10:36:24  krughsde
 * Teststruktur fuer FMEA eingefuegt
 * Revision 1.31  2000/03/13 15:24:58  krughsde
 * Voralarmbit in Limits.personal definiert
 * Revision 1.30  2000/02/28 10:19:32  krughsde
 * Kommentarfehler korrigiert
 * Revision 1.29  2000/02/28 10:14:20  krughsde
 * Reload-Segment dazu
 * Protokollversion hochgezaehlt
 * Revision 1.28  2000/01/21 11:41:36  krughsde
 * CA,CC-Rate Min/Max im Limitssegment ergaenzt
 * Revision 1.27  2000/01/13 08:02:16  krughsde
 * Anpassungen an C_10:
 * Aktion Alarm in AlarmAus umbenannt
 * ZeitEndeAlarm wieder eingefuehrt
 * LimSfSpritzenTropf in LimSfTropf umbenannt
 * BolVolAkt im Istwertsegment hinzugefuegt
 * Protokollversion auf 0x0d hochgezaehlt
 * Revision 1.26  1999/12/14 12:55:38  krughsde
 * Strukturnamen fuer Compact-Compiler gekuerzt
 * Revision 1.25  1999/12/09 15:20:16  krughsde
 * FrameTimeout war zu hoch
 * Revision 1.24  1999/12/07 13:40:10  krughsde
 * Version auf 0x0c erhoeht,
 * LIM_SF_MEDIKAMENT entfernt,
 * LIM_SF_AKKU entfernt
 * LIM_SF_UHR entfernt,
 * LIM_SF - Bits aufgerueckt
 * Limits : resbol ergaenzt,
 * Dosis : gebvol von 2 auf 4 byte erhoeht
 * Revision 1.23  1999/11/11 09:20:40  krughsde
 * ZEIT_ENDE_ALARM entfernt
 * MONITOR_STATUS_BIT entfernt
 * Debug - Bitfelder fuer Status.zustand u.Alarm definiert
 * Revision 1.22  1999/11/03 15:26:32  krughsde
 * Protokollversion von 0xff0b auf 0x000b geaendert
 * Revision 1.21  1999/10/29 13:50:44  krughsde
 * defines fuer Stringcheck entfernt
 * Revision 1.20  1999/10/20 14:39:52  krughsde
 * ERR_MONITOR entfernt
 * Revision 1.19  1999/10/20 13:58:28  krughsde
 * Min/Max Adresse eingefuegt
 * Revision 1.18  1999/10/20 10:49:52  krughsde
 * Timout Zeiten in Sekunden geaendert, BOLUS_MAN_TIMEOUT auf 1.2 geaendert
 * Revision 1.17  1999/10/14 10:19:00  seidfrde
 * CDS_BOLUSMAN_TIMEOUT eingefuegt
 * Revision 1.16  1999/10/04 16:39:46  krughsde
 * bolvolmin im Limitssegment eingefuegt
 * Revision 1.15  1999/10/04 13:57:48  krughsde
 * typedef umbenannt
 * Revision 1.14  1999/10/04 11:00:42  krughsde
 * bolmin im Limitsegment ergaenzt
 * Revision 1.13  1999/10/04 08:15:52  krughsde
 * Bitdefinitionen fuer Sollwertaenderungen eingefuegt
 * Revision 1.12  1999/09/30 15:52:50  krughsde
 * Aenderungen nach C_08 :
 * Istwert-, Sollwert-, Limit-, Statussegment geaendert
 * Revision 1.11  1999/09/28 14:36:00  krughsde
 * Header zusammengefasst
 * Revision 1.10  1999/09/08 16:01:54  prkm04de
 *	status alarm bit 17 (KVO VORALARM) fehlte
 * Revision 1.8	 1999/08/25 10:03:00  krughsde
 * defines fuer Stringueberpruefung eingefuegt
 * Revision 1.7	 1999/08/20 09:45:08  krughsde
 * Segment Status Element Request:: Bit Bedienung in Sollwert umbenannt, 
 * neues Bit fuer Dosis eingefuegt
 * Revision 1.6	 1999/08/19 13:35:32  krughsde
 * Schreibfehler bei definenamen korrigiert
 * Revision 1.5	 1999/08/19 13:15:28  krughsde
 * Statusbit Bedienstatusaktiv lt. Spez.C_07 eingefuegt,
 * Protokollversion CDS_PROT_VERSION eingefuegt
 * Revision 1.4	 1999/07/12 09:26:28  krughsde
 * BOLUS_STATUS_BIT in BOLUS_MAN_STATUS_BIT umbenannt, 
 * Fehlercode ERR_AKTION eingefuegt aus C_06
 * Revision 1.3	 1999/07/05 10:14:47Z  krughsde
 * Tropfueberwachungsbit fuer Angleich an dianet negiert
 * Revision 1.2	 1999/06/29 11:04:05Z  seidfrde
 * Schluesselwoerter data, code fuer Keil C51 umbenannt
 * Revision 1.1	 1999/06/25 12:04:47  krughsde
 * Initial revision
 *
 *****************************************************************************/
