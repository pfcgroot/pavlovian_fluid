// DianetStarCOM.cpp : Defines the exported functions for the DLL application.
//
// This is a Windows DLL-implementation of the communication protocol
// used by Braun infusion pumps. The implementation provides function call's
// for all (documented) communication packets. 
//
// A special feature is available for keeping a connection 'alive' 
// after establishing connection. This seems to be required to prevent the 
// pump to fall back in error mode (connection lost.)
// Separate threads will be created for each connected pump to manage this
// asynchroneous 'heart beat'.
//
// A logging feature is available to dump all transaction information to a file
// or stdout. A log level can be specified to filter the output.
//
// Parts of this software are based on an example application provided by Braun.
//

#pragma once

#define DLLENTRY __stdcall

#ifdef DIANETINTERFACE_IMPL
	#define DIANETINTERFACE_DLLX __declspec( dllexport )
#else
	#define DIANETINTERFACE_DLLX __declspec( dllimport )

	#define CDS_MEDNAME_LEN	   20	/* buffer length off medication string, exlcl terminating zero (stuff with spaces) */

	// mode's for DIANETINTERFACE_SetCcMode
	#define CDS_MODE_DOKU		0x00	/* Startzustand Dokumentationsbetrieb */
	#define CDS_MODE_FERNSTEUER 0x01	/* Fernsteuerbetrieb Bedienung an Pumpe erlaubt */
	#define CDS_MODE_FERNREGEL	0x02	/* Fernregelbetrieb Bedienung an Pumpe nicht erlaubt */
	#define CDS_MODE_MONREQ		0x03	/* Monitoraktivierung wird erwartet */

	// action_id's for DIANETINTERFACE_SetAction:
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

	// action values for DIANETINTERFACE_SetAction:
	#define CDS_AKTION_EIN	 0x01	/* Funktion EIN */
	#define CDS_AKTION_AUS	 0x0	/* Funktion AUS */
	#define CDS_AKTION_NOP	 0xff	/* Funktion wird nicht veraendert */

	// state bits for DIANETINTERFACE_GetStatus
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

	// request bits for DIANETINTERFACE_GetStatus
	/* status - request   bit 0-2  char */
	#define CDS_SOLLWERT_BIT_NR				0x00	/* Pumpen bedienung durch anwender,neue Sollwerte liegen vor */
	#define CDS_LIMITS_BIT_NR				0x01	/* Das Limit Segment ist abzufragen */
	#define CDS_ALARM_OFF_BIT_NR			0x02	/* Taste zur Alarmunterdrueckung wurde betaetigt */
	#define CDS_DOSIS_BIT_NR				0x03	/* Das Dosis Segment ist abzufragen */

	// alarm bits for DIANETINTERFACE_GetStatus
	/* status - alarm bit 0-7  long */
	#define CDS_SPRITZEN_ALARM_BIT_NR		0x00	/* Sammmelalarm aus Formschlussensor, .. */
	#define CDS_SPRITZEN_VOR_ALARM_BIT_NR	0x01
	#define CDS_SPRITZE_LEER_ALARM_BIT_NR	0x02 /**/
	#define CDS_DRUCK_ALARM_BIT_NR			0x03	/* Sammelalarm aus Druckalarm im Bolusabbau, .. */
	#define CDS_LUFT_ALARM_BIT_NR			0x04
	#define CDS_TROPF_ALARM_BIT_NR			0x05 /**/
	#define CDS_AKKU_LEER_ALARM_BIT_NR		0x06
	#define CDS_AKKU_VOR_ALARM_BIT_NR		0x07 /**/
	#define CDS_PUMPENKLAPPE_AUF_ALARM_BIT_NR 0x08
	#define CDS_RATE_UNGUELTIG_ALARM_BIT_NR 0x09 /* Einstellbereich wurde vom Anwender ueberschritten*/
	#define CDS_ERINNER_ALARM_BIT_NR		0x0A	/* Erinnerungsalarm */
	#define CDS_STANDBY_ENDE_ALARM_BIT_NR	0x0B	/* Zeit wurde abgearbeite */
	#define CDS_VTBD_ENDE_ALARM_BIT_NR		0x0C	/* VTBD wurde abgearbeitet */
	#define CDS_ZEIT_ENDE_ALARM_BIT_NR		0x0D	/* Zeitvorwahl wurde abgearbeitet */
	#define CDS_CC_ALARRM_BIT_NR			0x0E	/* Kommunikationsfehler (wird nur vom fm - System verwendet */
	#define CDS_KVO_ENDE_ALARM_BIT_NR		0x0F	/* KVO beendet */
	#define CDS_FERNREGEL_ALARM_BIT_NR		0x10	/* Taste start/stop im Fernregelbetrie betaetigt */
	#define CDS_KVO_VORALARM_BIT_NR			0x11	/* Pumpe laeuft mit Keep Vene Open - Rate */
	#define CDS_MINDRUCK_ALARM_BIT_NR		0x12	/* Mindestdruckunterschreitung ( Voralarm ) */
	#define CDS_PCA_TASTER_ALARM_BIT_NR		0x13        /* PCA-Taster nicht angeschlossen */
	#define CDS_PCA_HLIMVOL_ALARM_BIT_NR	0x14        /* Limit - Stunden-Volumen erreicht */
	/* status - alarm bit 21 - 31 Frei */

	/*  DIANETINTERFACE_GetLimits - personal bits */
	#define CDS_STATISCH_BIT_NR				0x00	/* Statisch */
	#define CDS_DYNAMISCH_BIT_NR			0x01	/* Dynamisch */
	#define CDS_AUSALARM_BIT_NR				0x02	/* Ausalarm */
	#define CDS_TESTIMPULS_BIT_NR			0x03	/* Testimpuls */
	#define CDS_VORALARM_BIT_NR             0x04	/* Voralarm */

	/*  DIANETINTERFACE_GetLimits - specialfn  bits */
	#define CDS_LIM_SF_BOLUS_BIT_NR			0x00	/* Bolus */
	#define CDS_LIM_SF_STANDBY_BIT_NR		0x01	/* Standby - Funktion */
	#define CDS_LIM_SF_DRUCK_BIT_NR			0x02	/* Druckberenzung */
	#define CDS_LIM_SF_DATALOCK_BIT_NR		0x03	/* Data Lock */
	#define CDS_LIM_SF_TROPF_BIT_NR			0x04	/* Tropfkontrolle bei Schlauch-Pumpen */
	#define CDS_LIM_SF_DOSIS_BIT_NR			0x05	/* Dosiskalkuation */

	/* DLLENTRY DIANETINTERFACE_GetDosis - units*/
	/* bytewise */
	#define CDS_EINHEIT_NC			0x00	/* nicht benuzt */
	/* unit group */
	#define CDS_EINHEIT_MG			0x20	/* Milligramm */
	#define CDS_EINHEIT_UG			0x40	/* Mikrogramm */
	#define CDS_EINHEIT_IE			0x60	/* IE */
	#define CDS_EINHEIT_MMOL		0x80	/* Millimol */
	/* volume group*/
	#define CDS_EINHEIT_ML			0x08	/* Milliliter */
	#define CDS_EINHEIT_KG			0x10	/* Kilogramm */
	/* time group */
	#define CDS_EINHEIT_SEC			0x01	/* Sekunden */
	#define CDS_EINHEIT_MIN			0x02	/* Minuten */
	#define CDS_EINHEIT_H			0x03	/* Stunden */
	#define CDS_EINHEIT_24H			0x04	/* 24 Stunden */

#endif

#ifdef __cplusplus
extern "C"
{
#endif
	DIANETINTERFACE_DLLX HANDLE	DLLENTRY DIANETINTERFACE_Connect(
			LPCSTR szPort, 
			LPCSTR szDevice
		);

	DIANETINTERFACE_DLLX BOOL   DLLENTRY DIANETINTERFACE_IsConnected(HANDLE h);
	DIANETINTERFACE_DLLX VOID   DLLENTRY DIANETINTERFACE_Disconnect(HANDLE h);
	DIANETINTERFACE_DLLX VOID   DLLENTRY DIANETINTERFACE_ResetConnection(HANDLE h);

	/* start heartbeat thread, or change interval of running thread; required to stay in CC/CA mode */
	/* interval_ms can be 0xFFFFFFFF to enter suspend mode */
	DIANETINTERFACE_DLLX VOID DLLENTRY DIANETINTERFACE_InitHeartbeat(HANDLE h, DWORD interval_ms);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_GetIdent(
			HANDLE h,
			DWORD* pSerial, 
			DWORD* pType, 
			USHORT* pProtoVers, 
			DWORD* pUnixTime, 
			LPSTR szSwVers, 
			USHORT nBytes
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_SetMode(
			HANDLE h,
			DWORD newaddres, 
			DWORD newmode, // see also DIANETINTERFACE_InitHeartbeat for CC/CA modes
			DWORD unixtime
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_SetAction(
			HANDLE h,
			DWORD action_id, 
			DWORD value
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_GetStatus(
			HANDLE h,
			DWORD* request, 
			DWORD* alarm, 
			DWORD* state, 
			DWORD* rate, 
			DWORD* remainvtbd, 
			DWORD* remaintime
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_GetTargetValue(
			HANDLE h,
			DWORD* rate_10ul_h, 
			DWORD* vtbd_100ul, 
			DWORD* time_min, 
			DWORD* bolrate_10ul_h, 
			DWORD* bolvol_100ul, 
			DWORD* pressurelimitmax, 
			DWORD* stdby_min, 
			char* szMedication
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_SetTargetValue(
			HANDLE h,
			DWORD rate_10ul_h, 
			DWORD vtbd_100ul, 
			DWORD time_min, 
			DWORD bolrate_10ul_h, 
			DWORD bolvol_100ul, 
			DWORD pressurelimitmax, 
			DWORD stdby_min, 
			const char* szMedication
		);

	DIANETINTERFACE_DLLX DWORD	DLLENTRY DIANETINTERFACE_GetCurrentValue(
			HANDLE h,
			DWORD* pressure_pct,
			DWORD* infusedvol_100ul,
			DWORD* infusedtime_min,
			DWORD* bolusintervaltimeleft_min,
			DWORD* batterytimeleft_min,
			DWORD* standbytimeleft_min,
			DWORD* syringevol_ml,
			DWORD* bolusintervaltime_min,
			DWORD* bolusintervalvolume_100ul,
			DWORD* actualbolusvolume_100ul
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetLimits(
			HANDLE h,
			DWORD* bolmax_10ul_h,		/* max.Bolusrate [0.01ml/h] */
			DWORD* personal,			/* Pump reflects the type of personnel call */
			DWORD* resdisp,				/* Number of decimal places for the rate in up ml/h */
			DWORD* ratemax_10ul_h,		/* Current max. delivery rate, depending on operating mode. [0.01ml/h] */
			DWORD* ratemin_10ul_h,		/* Current min. delivery rate, depending on operating mode. [0.01ml/h] */
			DWORD* syringeprealarm_min,	/* Syringe pre-alarm [min] */
			DWORD* specialfn,			/* Special functions available for Dianet Star */
			DWORD* pressmax,			/* Max. adjustable pressure level [1..9] */
			DWORD* vtbdmax_100ul,		/* Max. adjustable volume for VTBD, also applies for RELOAD_VTBD [0.1ml] */
			DWORD* bolvolmax_100ul,		/* Max. adjustable bolus volume [0.1ml] */
			DWORD* bolratemin_10ul_h,	/* Min. bolus rate [0.01ml/h] */
			DWORD* bolvolmin_100ul,		/* Min. adjustable bolus volume [0.1ml] */
			DWORD* resbol,				/* Number of bolus rate decimal places */
			DWORD* caratemax_10ul_h,	/* Max. delivery rate in CD/CA mode [0.01ml/h] */
			DWORD* caratemin_10ul_h,	/* Min. delivery rate in CD/CA mode [0.01ml/h] */
			DWORD* ccratemax_10ul_h,	/* Max. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */
			DWORD* ccratemin_10ul_h		/* Min. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetDosis(
			HANDLE h,
			DWORD* weight_10gr,			/* body weight [10g] */
			DWORD* concentration_milli,	/* concentration [0.001 <units> with IE = 1 ] */
			DWORD* concentrationunits,	/* units of concentration */
			DWORD* doserate_milli,		/* dose rate [0.001 <units> with IE = 1 ] */
			DWORD* doserate_units,		/* dose rate units */
			DWORD* gebvol_10ul,			/* volume with which the active substance was activated [0.01 ml] */
			DWORD* drug_milli,			/* quantity of active substance within the package volume [0.001 <units> with IE = 1 ] */
			DWORD* drug_units			/* units of the active substance */
		);

	/* Reload function is only executed in CC mode! */
	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetReload(
			HANDLE h,
			DWORD active,				/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
											The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
											The AKTIV state is reflected in the RELOAD_AKTIV status.
										 */
			DWORD rate_10ul_h,			/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
			DWORD vtbd_100ul			/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetReload(
			HANDLE h,
			DWORD* active,				/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
											The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
											The AKTIV state is reflected in the RELOAD_AKTIV status.
										 */
			DWORD* rate_10ul_h,			/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
			DWORD* vtbd_100ul			/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetProposal(
			HANDLE h,
			DWORD* rate_10ul_h,			/* proposed rate [0.01ml/h] */
			char* szMedication			/* proposed medication incl. terminating zero [len>CDS_MEDNAME_LEN] ! */
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetProposal(
			HANDLE h,
			DWORD rate_10ul_h,			/* proposed rate [0.01ml/h] */
			const char* szMedication	/* proposed medication incl. terminating zero [len<=CDS_MEDNAME_LEN] ! */
		);

	/* returns one of the fixed error messages (max len==26) */
	DIANETINTERFACE_DLLX const char* DLLENTRY DIANETINTERFACE_GetErrorString(
			DWORD errnr					/* value returned by any of the DIANETINTERFACE_* functions */
		);

	#define CDS_LOGLEVEL_ERROR	0
	#define CDS_LOGLEVEL_WARN	1
	#define CDS_LOGLEVEL_INFO	2
	#define CDS_LOGLEVEL_IO		3

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_OpenLogFile(
			const char* szFilename,		/* file to open, or null/empty to use standard output */
			BOOL bAppend,				/* append to existing file if true */
			DWORD iLogLevel,			/* 0=errors only; 1=warnings; 2=all */
			DWORD externalClock_ms		/* current value of user defined clock in [ms] */
		);

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_CloseLogFile();

	DIANETINTERFACE_DLLX void DLLENTRY DIANETINTERFACE_LogMessage(
			const char* szMessage,		/* add message to logfile */
			DWORD iLogLevel				/* 0=errors only; 1=warnings; 2=all */
		);

	DIANETINTERFACE_DLLX void DLLENTRY DIANETINTERFACE_SetLogTimer(
			DWORD externalClock_ms		/* current value of user defined clock in [ms] */
		);

#ifdef __cplusplus
} // extern "C"

#endif


