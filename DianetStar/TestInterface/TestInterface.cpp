// TestInterface.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DianetStarCOM.h"
#include <time.h>


DWORD GetAndPrintIdent(HANDLE h)
{
	// -------------- READ IDENT --------------
	// declare a few variables to receive info
	DWORD dwSerial, dwType, dwUnixTime;
	USHORT uProtoVers;
	CHAR szSwVers[12];
	// get info from device
	DWORD ret = DIANETINTERFACE_GetIdent(h, &dwSerial, &dwType, &uProtoVers, &dwUnixTime, szSwVers, sizeof(szSwVers) );
	// logging stream will show output
	return ret;
}

DWORD SwitchMode(HANDLE h, DWORD newMode, DWORD unixTime=-1)
{
	DWORD ret = DIANETINTERFACE_SetMode(h, 1, newMode, unixTime);
	// logging stream will show output
	return ret;
}

DWORD SendAndPrint(HANDLE h, DWORD action_id, DWORD value)
{
	DWORD ret = DIANETINTERFACE_SetAction(h, action_id, value);
	// logging stream will show output
	return ret;
}

DWORD GetAndPrintStatus(HANDLE h)
{
	DWORD request, alarm, state, rate, remainvtbd, remaintime;
	DWORD ret = DIANETINTERFACE_GetStatus(h,&request, &alarm, &state, &rate, &remainvtbd, &remaintime);
	// logging stream will show output
	return ret;
}

DWORD GetAndPrintTargetValue(HANDLE h)
{
	DWORD rate, vtbd, time, bolrate, bolvol, pressurelimitmax, stdby_minutes;
	char szMedication[CDS_MEDNAME_LEN+1];
	DWORD ret = DIANETINTERFACE_GetTargetValue(h, &rate, &vtbd, &time, &bolrate, &bolvol, &pressurelimitmax, &stdby_minutes, szMedication);
	// logging stream will show output
	return ret;
}

DWORD SendAndPrintTargetValue(HANDLE h, DWORD rate_10ul_h, DWORD vtbd_100ul, DWORD time, DWORD bolrate_10ul_h, DWORD bolvol_100ul, DWORD pressurelimitmax, DWORD stdby_minutes, const char* szMedication)
{
	DWORD ret = DIANETINTERFACE_SetTargetValue(h, rate_10ul_h, vtbd_100ul, time, bolrate_10ul_h, bolvol_100ul, pressurelimitmax, stdby_minutes, szMedication);
	// logging stream will show output
	return ret;
}

DWORD GetAndPrintCurrentValue(HANDLE h)
{
	DWORD pressure_pct, infusedvol_100ul, infusedtime_min, bolusintervaltimeleft_min, batterytimeleft_min, standbytimeleft_min,
		syringevol_ml, bolusintervaltime_min, bolusintervalvolume_100ul, actualbolusvolume_100ul;
	DWORD ret = DIANETINTERFACE_GetCurrentValue(h, &pressure_pct, &infusedvol_100ul, &infusedtime_min, 
		&bolusintervaltimeleft_min, &batterytimeleft_min, &standbytimeleft_min,
		&syringevol_ml, &bolusintervaltime_min, &bolusintervalvolume_100ul, &actualbolusvolume_100ul);
	// logging stream will show output
	return ret;
}

DWORD GetAndPrintLimits(HANDLE h)
{
	DWORD bolmax_10ul_h;		/* max.Bolusrate [0.01ml/h] */
	DWORD personal;				/* Pump reflects the type of personnel call */
	DWORD resdisp;				/* Number of decimal places for the rate in up ml/h */
	DWORD ratemax_10ul_h;		/* Current max. delivery rate, depending on operating mode. [0.01ml/h] */
	DWORD ratemin_10ul_h;		/* Current min. delivery rate, depending on operating mode. [0.01ml/h] */
	DWORD syringeprealarm_min;	/* Syringe pre-alarm [min] */
	DWORD specialfn;			/* Special functions available for Dianet Star */
	DWORD pressmax;				/* Max. adjustable pressure level [1..9] */
	DWORD vtbdmax_100ul;		/* Max. adjustable volume for VTBD, also applies for RELOAD_VTBD [0.1ml] */
	DWORD bolvolmax_100ul;		/* Max. adjustable bolus volume [0.1ml] */
	DWORD bolratemin_10ul_h;	/* Min. bolus rate [0.01ml/h] */
	DWORD bolvolmin_100ul;		/* Min. adjustable bolus volume [0.1ml] */
	DWORD resbol;				/* Number of bolus rate decimal places */
	DWORD caratemax_10ul_h;		/* Max. delivery rate in CD/CA mode [0.01ml/h] */
	DWORD caratemin_10ul_h;		/* Min. delivery rate in CD/CA mode [0.01ml/h] */
	DWORD ccratemax_10ul_h;		/* Max. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */
	DWORD ccratemin_10ul_h;		/* Min. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */

	DWORD ret = DIANETINTERFACE_GetLimits(h,
		&bolmax_10ul_h, &personal, &resdisp, &ratemax_10ul_h, &ratemin_10ul_h,
		&syringeprealarm_min, &specialfn, &pressmax, &vtbdmax_100ul,
		&bolvolmax_100ul, &bolratemin_10ul_h, &bolvolmin_100ul, &resbol,
		&caratemax_10ul_h, &caratemin_10ul_h, &ccratemax_10ul_h, &ccratemin_10ul_h
	);
	// logging stream will show output
	return ret;
}


DWORD GetAndPrintDosis(HANDLE h)
{
	DWORD weight_10gr;			/* body weight [10g] */
	DWORD concentration_milli;	/* concentration [0.001 <units> with IE = 1 ] */
	DWORD concentrationunits;	/* units of concentration */
	DWORD doserate_milli;		/* dose rate [0.001 <units> with IE = 1 ] */
	DWORD doserate_units;		/* dose rate units */
	DWORD gebvol_10ul;			/* volume with which the active substance was activated [0.01 ml] */
	DWORD drug_milli;			/* quantity of active substance within the package volume [0.001 <units> with IE = 1 ] */
	DWORD drug_units;			/* units of the active substance */

	DWORD ret = DIANETINTERFACE_GetDosis(h,
			&weight_10gr,
			&concentration_milli,
			&concentrationunits,
			&doserate_milli,
			&doserate_units,
			&gebvol_10ul,
			&drug_milli,
			&drug_units	
	);
	// logging stream will show output
	return ret;
}

DWORD GetAndPrintReload(HANDLE h)
{
	DWORD active;		/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
							The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
							The AKTIV state is reflected in the RELOAD_AKTIV status.
						 */
	DWORD rate_10ul_h;	/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
	DWORD vtbd_100ul;	/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */

	DWORD ret = DIANETINTERFACE_GetReload(h,
		&active,
		&rate_10ul_h,
		&vtbd_100ul
	);
	// logging stream will show output
	return ret;
}

DWORD SendAndPrintReload(
	HANDLE h,
	DWORD active,		/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
							The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
							The AKTIV state is reflected in the RELOAD_AKTIV status.
						 */
	DWORD rate_10ul_h,	/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
	DWORD vtbd_100ul	/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */
							  )
{
	DWORD ret = DIANETINTERFACE_SetReload(h,
		active,
		rate_10ul_h,
		vtbd_100ul
	);
	// logging stream will show output
	return ret;
}


DWORD GetAndPrintProposal(HANDLE h)
{
	DWORD rate_10ul_h;						/* proposed rate [0.01ml/h] */
	char szMedication[CDS_MEDNAME_LEN+1];	/* proposed medication incl. terminating zero [len<=CDS_MEDNAME_LEN] ! */

	DWORD ret = DIANETINTERFACE_GetProposal(h,
		&rate_10ul_h,
		szMedication
	);
	// logging stream will show output
	return ret;
}

DWORD SendAndPrintProposal(
			HANDLE h,
			DWORD rate_10ul_h,			/* proposed rate [0.01ml/h] */
			const char* szMedication	/* proposed medication incl. terminating zero [len<=CDS_MEDNAME_LEN] ! */
	)
{
	DWORD ret = DIANETINTERFACE_SetProposal(h,
		rate_10ul_h,
		szMedication
	);
	// logging stream will show output
	return ret;
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	ATL::AtlTraceLoadSettings(NULL);
#endif
	DWORD ret = 0;
	const char* szPort = "COM1";
	if (argc==2)
		szPort = argv[1];
	DWORD request, alarm, state, rate, remainvtbd, remaintime;

	DIANETINTERFACE_OpenLogFile(
			"testinterface_log.txt",	/* file to open, or null/empty to use standard output */
			false,				/* append to existing file if true */
			CDS_LOGLEVEL_INFO,	/* 0=errors only; 1=warnings; 2=all */
			0					/* current value of user defined clock in [ms] */
		);

	// --- connect to device using RS232 ---
	HANDLE h = DIANETINTERFACE_Connect(szPort, NULL);
	if (h!=INVALID_HANDLE_VALUE)
	{
		printf("Connected to %s\n",szPort);

		const bool bVolumeBolus = true; // a manual bolus has duration of 1 sec and can be repeated without setting target each time
										// a volume bolus should be reconfigured each time
		const int nBoli = 3;
		const int vol_ul = 200; // multiple of 100ul
		const int dur_sec = 1; // only affective for manual bolus (which has fixed duration of 1 sec)
		DWORD rate_10ul_h = bVolumeBolus ? 99990 : (vol_ul * 3600) / (10 * dur_sec); // volume bolus can go at max. speed

		if (ret==0) ret = GetAndPrintIdent(h);
//		if (ret==0) ret = SwitchMode(h,CDS_MODE_FERNSTEUER);				// Change remote&manual operation  
		if (ret==0) ret = SwitchMode(h,CDS_MODE_FERNREGEL);					// Change remote-only operation  
		if (ret==0) ret = SendAndPrint(h,CDS_AKT_ALARM_AUS, CDS_AKTION_EIN); 	// Make silent  
		if (ret==0) ret = GetAndPrintStatus(h);								// print current status
		if (ret==0) ret = GetAndPrintLimits(h);								// print limits

		if (ret==0) ret = SendAndPrintTargetValue(h,0, 1/*add one to prevent auto-fallback to CA mode*/+(nBoli*vol_ul)/100, 0, rate_10ul_h, vol_ul/100, 2, 0, "PAUL RULES"); // 1ml, 99990 = max rate
//		if (ret==0) ret = SendAndPrintTargetValue(h,0, (nBoli*180000)/(10*3600), 0, 180000, 0, 9, 10, "PAUL RULES"); // 1ml, 180000 = max rate

//		if (ret==0) ret = SendAndPrintReload(h,1,180000,10);				// set reload values
//		if (ret==0) ret = GetAndPrintCurrentValue(h);						// get actual values
//		if (ret==0) ret = GetAndPrintTargetValue(h);						// print target settings
//		if (ret==0) ret = GetAndPrintDosis(h);								// get dosis
//		if (ret==0) ret = GetAndPrintReload(h);								// print reload values

//		if (ret==0) ret = GetAndPrintProposal(h);							// print proposal
//		if (ret==0) ret = SendAndPrintProposal(h,0,"H2O");					// print proposal
		Sleep(100);
//		if (ret==0) ret = GetAndPrintStatus(h);								// print current status
		if (ret==0) ret = SendAndPrint(h,CDS_AKT_START_STOP, CDS_AKTION_EIN);	// Start pump
		
		DIANETINTERFACE_InitHeartbeat(h,500);

		for (int iBolus=0;iBolus<nBoli && ret==0; iBolus++)
		{
			// check if bolus can be started
			Sleep(3000);
			ret = DIANETINTERFACE_GetStatus(h,&request, &alarm, &state, &rate, &remainvtbd, &remaintime);
			if (ret==0 && (state&(1<<CDS_BOLUS_READY_STATUS_BIT_NR))==0)
				printf("\nWARNING: bolus not ready!\n");

			if (bVolumeBolus)
			{
				printf("starting new VOL bolus \n");

				// reconfigure for next volume bolus:
				if (ret==0) ret = SendAndPrintTargetValue(h,-1, -1, -1, -1, vol_ul/100, -1, -1, NULL); // 1ml, 99990 = max rate
				//if (ret==0) ret = GetAndPrintTargetValue(h);							// print target settings
				//if (ret==0) ret = GetAndPrintDosis(h);								// get dosis
				if (ret==0) ret = SendAndPrint(h,CDS_AKT_BOL_VOL_START, CDS_AKTION_EIN);

				while(ret==0)
				{
					Sleep(300);
					ret = DIANETINTERFACE_GetStatus(h,&request, &alarm, &state, &rate, &remainvtbd, &remaintime);
					if (ret==0 && state&(1<<CDS_BOLUS_VOL_STATUS_BIT_NR))
						printf("bolus still running...\n");
					else
					{
						printf("bolus ready!\n");
						break;
					}
				}
			}
			else
			{
				printf("starting new MAN bolus \n");
				if (ret==0) ret = SendAndPrint(h,CDS_AKT_BOL_MAN_START, CDS_AKTION_EIN);
			}

			if (ret==0) ret = GetAndPrintCurrentValue(h);			// get actual values
		}

//		if (ret==0) 
			ret = GetAndPrintCurrentValue(h);						// get actual values
//		if (ret==0) 
			ret = GetAndPrintStatus(h);
		if (ret==0) ret = SendAndPrint(h,CDS_AKT_START_STOP, CDS_AKTION_AUS);	// Stop pump
		DIANETINTERFACE_LogMessage("Bolus delivery ready; reset target values and return to documentation mode",CDS_LOGLEVEL_INFO);
		if (ret==0) ret = SendAndPrintTargetValue(h,0, 0, 0, 0, 0, -1, 0, "PAUL READY"); // set rate to zero to prevent invalid rate at transition to CA mode
		if (ret==0) ret = SwitchMode(h,CDS_MODE_DOKU);					// Change remote&manual operation  

		// -------------- DICONNECT --------------
		DIANETINTERFACE_Disconnect(h);
		printf("\nDisconnected\n");
	}
	else
	{
		printf("ERROR: Connection to %s failed\n",szPort);
	}
	DIANETINTERFACE_CloseLogFile();
	return 0;
}

