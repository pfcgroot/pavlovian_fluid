#pragma once

#define DLLENTRY __stdcall
#ifdef IO_RS232_IMPL
#	define IO_RS232_DLLX __declspec( dllexport )
#else
#	define IO_RS232_DLLX __declspec( dllimport )
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	IO_RS232_DLLX HANDLE DLLENTRY IO_RS232_Connect(LPCSTR szPort);
	IO_RS232_DLLX BOOL   DLLENTRY IO_RS232_IsConnected(HANDLE hDevice);
	IO_RS232_DLLX VOID   DLLENTRY IO_RS232_Disconnect(HANDLE hDevice);
	IO_RS232_DLLX VOID   DLLENTRY IO_RS232_ResetSerial(HANDLE hDevice);
	IO_RS232_DLLX DWORD  DLLENTRY IO_RS232_SendBuffer(HANDLE hDevice, LPCSTR pBuf, INT nBytes);
	IO_RS232_DLLX DWORD  DLLENTRY IO_RS232_ReceiveBuffer(HANDLE hDevice, LPSTR pBuf, INT nBytes, INT* pnBytesRead);
	IO_RS232_DLLX DWORD  DLLENTRY IO_RS232_ReadShort(HANDLE hDevice, SHORT* s);
	IO_RS232_DLLX DWORD  DLLENTRY IO_RS232_ReadByte(HANDLE hDevice, CHAR* c);
	IO_RS232_DLLX DWORD  DLLENTRY IO_RS232_GetDllVersion(SHORT* pVersion, INT nLevels/*up to 4 version levels supported*/);

#ifdef __cplusplus
} // extern "C"

#endif


