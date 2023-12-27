// AmsSerial.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "IO_RS232.h"
#pragma comment( lib, "version" )	// for GetFileVersionInfoSize() etc.
#include <stdio.h> // for sprintf_s and sscanf in GetDllVersion
#include <stdlib.h> // for _MAX_PATH
#include <set> // for storing open port handles

#ifdef _MANAGED
#pragma managed(push, off)
#endif

// also export undecorated names (for non-C/C++ languages)
#if defined(_WIN64)
	#error "might be tricky: not investigated yet"
	#pragma comment(linker, "/EXPORT:IO_RS232_IsConnected")
#else

	#pragma comment(linker, "/EXPORT:IO_RS232_GetDllVersion=_IO_RS232_GetDllVersion@8")
	#pragma comment(linker, "/EXPORT:IO_RS232_IsConnected=_IO_RS232_IsConnected@4")
	#pragma comment(linker, "/EXPORT:IO_RS232_Connect=_IO_RS232_Connect@4")
	#pragma comment(linker, "/EXPORT:IO_RS232_Disconnect=_IO_RS232_Disconnect@4")
	#pragma comment(linker, "/EXPORT:IO_RS232_ResetSerial=_IO_RS232_ResetSerial@4")
	#pragma comment(linker, "/EXPORT:IO_RS232_SendBuffer=_IO_RS232_SendBuffer@12")
	#pragma comment(linker, "/EXPORT:IO_RS232_ReceiveBuffer=_IO_RS232_ReceiveBuffer@16")
	#pragma comment(linker, "/EXPORT:IO_RS232_ReadShort=_IO_RS232_ReadShort@8")
	#pragma comment(linker, "/EXPORT:IO_RS232_ReadByte=_IO_RS232_ReadByte@8")
#endif 

//static HANDLE _hDevice = INVALID_HANDLE_VALUE; 
static std::set<HANDLE> _handles;
static HMODULE _hDllModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: 
		_hDllModule = hModule;
		break;
	case DLL_PROCESS_DETACH: 
		IO_RS232_Disconnect(INVALID_HANDLE_VALUE); // close all
		break;
	}
    return TRUE;
}

extern "C"
{
	IO_RS232_DLLX BOOL DLLENTRY IO_RS232_IsConnected(HANDLE hDevice)
	{
		BOOL b = FALSE;
		if (hDevice!=INVALID_HANDLE_VALUE)
			b = _handles.find(hDevice) != _handles.end();
		return b;
	}

	IO_RS232_DLLX VOID DLLENTRY IO_RS232_Disconnect(HANDLE hDevice)
	{
		if (hDevice!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(hDevice);
			_handles.erase(hDevice);
		}
		else
		{
			for (std::set<HANDLE>::iterator i=_handles.begin(); i!=_handles.end(); i++)
				CloseHandle(*i);
			_handles.clear();
		}
	}

	IO_RS232_DLLX HANDLE DLLENTRY IO_RS232_Connect(LPCSTR szPort /*\\.\COM1 baud=1200 parity=N data=8 stop=1*/)
	{
		HANDLE hDevice = INVALID_HANDLE_VALUE;
		DCB dcb;
		int msec_per_byte = 0;
		COMMTIMEOUTS to;
		DWORD hr = ERROR_SUCCESS;

		// be sure no connection is active
		//IO_RS232_Disconnect();

		char buf[256] = "";
		if (szPort[0]!='\\')
			strcpy_s(buf, sizeof(buf), "\\\\.\\"); 
		strcat_s(buf,sizeof(buf), szPort);
		if(strstr(szPort, "baud")==0)
			strcat_s(buf,sizeof(buf)," baud=9600 parity=N data=8 stop=1"); 

		// first part of string must be device name (i.e. \\.\COM1)
		size_t pos = strcspn(buf," \t;,");
		if (pos<strlen(buf))
			buf[pos++] = '\0';
		else
			pos=0;
		/*
			hr = ERROR_INVALID_PARAMETER;
			goto exit;
		*/

	#if COUT_LEVEL>0
		std::cout << str.c_str() << std::endl;
	#endif
		// open I/O handle to COM device
		hDevice = CreateFile(
			buf,							// pointer to name of the file (device)
			GENERIC_READ|GENERIC_WRITE,		// access (read-write) mode 
			0,								// share mode 
			NULL,							// pointer to security descriptor 
			OPEN_EXISTING,					// how to create 
			FILE_ATTRIBUTE_NORMAL,			// file attributes: enable async. I/O
			0								// handle to file with attributes to copy  
			);

		if (INVALID_HANDLE_VALUE==hDevice)
		{
			//MessageBox(NULL,buf,"cannot open",MB_OK);
	#if COUT_LEVEL>0
			std::cout << "Failed to open device " << str.c_str() << std::endl;
	#endif
			hr = GetLastError();
			goto exit;
		}

		ZeroMemory(&dcb,sizeof(dcb));
		if (!BuildCommDCB(/*str.c_str()+1+*/buf+pos, &dcb)) 
		{
	#if COUT_LEVEL>0
			std::cout << "Could not build DCB using " << (str.c_str()+pos+1) << std::endl;
	#endif
			hr = GetLastError();
			goto exit;
		}
		// Note: RTS and DTR are set/cleared and keep their state until
		// the device is closed.
	//	dcb.fRtsControl = RTS_CONTROL_ENABLE; // clear bit to set high (+10V)
	//	dcb.fDtrControl = DTR_CONTROL_DISABLE; // set bit to set low (-10V)
		dcb.fAbortOnError = 0;
		if (!SetCommState(hDevice, &dcb))
		{
	#if COUT_LEVEL>0
			std::cout << "Could not set DCB using " << (str.c_str()+pos+1) << std::endl;
	#endif
			hr = GetLastError();
			goto exit;
		}
	//	Sleep(200); // sleep a while to establish stable power lines

		// Check if CTS (clear-to-send) is on
	/*	if (bTestCTS)
		{
			DWORD dwModemStat(0);
			if (!GetCommModemStatus(m_arg.m_hCommHandle, &dwModemStat) || (dwModemStat&MS_CTS_ON)==0)
			{
				// MS_CTS_ON	The CTS (clear-to-send) signal is on.
				// MS_DSR_ON	The DSR (data-set-ready) signal is on.
				// MS_RING_ON	The ring indicator signal is on.
				// MS_RLSD_ON	The RLSD (receive-line-signal-detect) signal is on.
				VERIFY(CloseHandle(m_arg.m_hCommHandle));
				m_arg.m_hCommHandle = INVALID_HANDLE_VALUE;
				return csCableNotAvailable;
			}
		}
	*/
		msec_per_byte = 2 * 10000 / dcb.BaudRate + 1;	// assume 10 bits per character, and double the value just in case
		to.ReadIntervalTimeout = 0;						// ignore inter-character times
		to.ReadTotalTimeoutMultiplier = msec_per_byte;	// usually 1 if baudrate>=9600
		to.ReadTotalTimeoutConstant = 1000;				// allow a response latency of about 2000 msec
		to.WriteTotalTimeoutMultiplier = msec_per_byte;	// usually 1 if baudrate>=9600
		to.WriteTotalTimeoutConstant = 500; 			// allow a response latency of about 2000 msec
		if (!SetCommTimeouts(hDevice, &to))
		{
	#if COUT_LEVEL>0
			std::cout << "Could not set COM timeouts" << std::endl;
	#endif
			hr = GetLastError();
			goto exit;
		}

		IO_RS232_ResetSerial(hDevice);

		_handles.insert(hDevice);
		return hDevice;

	exit:
	#if COUT_LEVEL>0
		std::cout << "Connection error: " << hr << std::endl;
		ATLTRACE("Connection error 0x%08x\n",hr);
	#endif
		IO_RS232_Disconnect(hDevice);
		SetLastError(hr);
		return INVALID_HANDLE_VALUE;
	}

	IO_RS232_DLLX DWORD DLLENTRY IO_RS232_SendBuffer(HANDLE hDevice, LPCSTR pBuf, INT nBytes)
	{
		DWORD nBytesWritten = 0;
		DWORD hr = ERROR_SUCCESS;
	#if COUT_LEVEL>1
		int i;
	#endif

		if (hDevice==INVALID_HANDLE_VALUE)
		{
			hr = ERROR_DEVICE_NOT_CONNECTED;
			goto _exit;
		}

		if (!WriteFile(hDevice, pBuf, nBytes, &nBytesWritten, NULL))
		{
			hr = GetLastError();
			goto _exit;
		}

		if (nBytesWritten!=nBytes)
		{
	#if COUT_LEVEL>0
			std::cout << "DID NOT SEND #bytes:" << nBytes-nBytesWritten << std::endl;
	#endif
			hr = ERROR_BAD_LENGTH;
			goto _exit;
		}

	#if COUT_LEVEL>1
		std::cout << "send stream: "  << std::endl;
		char buf [256];
		for (i=0; i<nBytes; i++)
		{
			sprintf(buf,"%02x",(int)pBuf[i]);
			std::cout << buf;
		}
		std::cout << std::endl;
		std::cout.setf(std::ios::dec);
	#endif

		return ERROR_SUCCESS;

	_exit:
		return hr; // NO Error() in non-interface functions!!!
	}

	// virtual
	IO_RS232_DLLX DWORD DLLENTRY IO_RS232_ReceiveBuffer(HANDLE hDevice, LPSTR pBuf, INT nBytes, INT* pnBytesRead)
	{
		DWORD nBytesRead = 0;
		DWORD hr = ERROR_SUCCESS;
	#if COUT_LEVEL>1
		int i;
	#endif

		if (hDevice==INVALID_HANDLE_VALUE)
		{
			hr = ERROR_DEVICE_NOT_CONNECTED;
			goto _exit;
		}


		if (!ReadFile(hDevice, pBuf, nBytes, &nBytesRead, NULL))
		{
			hr = GetLastError();
			goto _exit;
		}

		if (pnBytesRead!=NULL)
			*pnBytesRead = nBytesRead;
		else if (nBytesRead!=nBytes) 
		{
			// only return this as an error if the number of bytes read cannot be returned
	#if COUT_LEVEL>0
			if (nBytesRead==0)
				std::cout << "NOTHING RECEIVED!   requested #" << nBytes << std::endl;
			else
				std::cout << "MISSING #bytes:" << nBytes-nBytesRead << std::endl;
	#endif
			hr = ERROR_BAD_LENGTH;
			goto _exit;
		}

	#if COUT_LEVEL>1
		std::cout << "received stream: "  << std::endl;
		char buf [256];
		for (i=0; i<nBytes; i++)
		{	sprintf(buf,"%02x",(int)pBuf[i]);
			std::cout << buf;
		}
		std::cout << std::endl;
		std::cout.setf(std::ios::dec);
	#endif

		return ERROR_SUCCESS;

	_exit:
		return hr; // NO Error() in non-interface functions!!!
	}

	IO_RS232_DLLX VOID DLLENTRY IO_RS232_ResetSerial(HANDLE hDevice)
	{
		if (hDevice==INVALID_HANDLE_VALUE)
			return;
		PurgeComm(hDevice,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
//		Sleep(10); 
	}

	IO_RS232_DLLX DWORD DLLENTRY IO_RS232_ReadShort(HANDLE hDevice, SHORT* s)
	{
		if (hDevice==INVALID_HANDLE_VALUE)
			return ERROR_DEVICE_NOT_CONNECTED;

		DWORD hr = IO_RS232_ReceiveBuffer(hDevice, (LPSTR)s, sizeof(SHORT), NULL);
	//	if (hr==ERROR_SUCCESS && _amsModel==1)
	//		*s = (((*s)&0x00FF)<<8) | (((*s)&0xFF00)>>8);

		return hr;
	}

	IO_RS232_DLLX DWORD DLLENTRY IO_RS232_ReadByte(HANDLE hDevice, CHAR* c)
	{
		if (hDevice==INVALID_HANDLE_VALUE)
			return ERROR_DEVICE_NOT_CONNECTED;

		return IO_RS232_ReceiveBuffer(hDevice, (LPSTR)c, sizeof(CHAR), NULL);
	}


	IO_RS232_DLLX DWORD DLLENTRY IO_RS232_GetDllVersion(SHORT* pVersion, INT nLevels)
	{
		DWORD res = ERROR_NOT_SUPPORTED;
		int a(0),b(0),c(0),d(0);
		DWORD h;
		char strAppName[_MAX_PATH];// = AfxGetAppName();
		if (GetModuleFileName(_hDllModule, strAppName, sizeof(strAppName))==0)
			return GetLastError();

		unsigned int size;
		if (size=GetFileVersionInfoSize(
						strAppName,  // pointer to filename string 
						&h  // pointer to variable to receive zero 
			))
		{
			void* p = new char[size];
			if (p && GetFileVersionInfo(strAppName, h, size, p))
			{
				struct LANGANDCODEPAGE {
					WORD wLanguage;
					WORD wCodePage;
				} *lpTranslate;

				// user first entry in language translation table (i.e. Englisch US)
				if (VerQueryValue( p, "\\VarFileInfo\\Translation", (void**)&lpTranslate, &size))
				{
					char strQuery[256];
					sprintf_s(strQuery, sizeof(strQuery), "\\StringFileInfo\\%4.4X%4.4X\\", 
						lpTranslate[0].wLanguage,
						lpTranslate[0].wCodePage);
					const size_t len = strlen(strQuery);

					TCHAR* pInfo;
					strcpy_s(strQuery+len, sizeof(strQuery)-len, "FileVersion");
					if (VerQueryValue( p, strQuery, (void**)&pInfo, &size))
					{
					//	MessageBox(NULL,(const char*)pInfo,"titel",MB_OK);
						res = ERROR_SUCCESS;
						sscanf_s((const char*)pInfo,"%d,%d,%d,%d",&a,&b,&c,&d);
					}
				}
			}
			delete [] p;
		}
		if (nLevels>=1) pVersion[0] = a;
		if (nLevels>=2) pVersion[1] = b;
		if (nLevels>=3) pVersion[2] = c;
		if (nLevels>=4) pVersion[3] = d;
		return res;
	}

} // extern "C"

#ifdef _MANAGED
#pragma managed(pop)
#endif