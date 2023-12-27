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
// Created by Paul F.C. Groot
// Copyright 2010-2011, Academisch Medisch Centrum Amsterdam, The Netherlands
//

#include "stdafx.h"
#include <stdio.h>			// for fprintf logging
#include <time.h>			// for time()
#include "DianetStarCOM.h"
#include "IO_RS232.h"		// Standard COM communication routines
#include "dscomcrc.h"		// CRC check
#include "dstarcom.h"		// communication structures
#include "dsend_pr.h"		// little/big endian conversion
#include <map>

// also export undecorated names (for non-C/C++ languages)
#if defined(_WIN64)
	#error "might be tricky: not investigated yet"
	#pragma comment(linker, "/EXPORT:IsConnected")
#else
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_Connect=_DIANETINTERFACE_Connect@8")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_IsConnected=_DIANETINTERFACE_IsConnected@4")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_Disconnect=_DIANETINTERFACE_Disconnect@4")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_ResetConnection=_DIANETINTERFACE_ResetConnection@4")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_InitHeartbeat=_DIANETINTERFACE_InitHeartbeat@8")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetIdent=_DIANETINTERFACE_GetIdent@28")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetMode=_DIANETINTERFACE_SetMode@16")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetAction=_DIANETINTERFACE_SetAction@12")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetStatus=_DIANETINTERFACE_GetStatus@28")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetTargetValue=_DIANETINTERFACE_GetTargetValue@36")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetTargetValue=_DIANETINTERFACE_SetTargetValue@36")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetCurrentValue=_DIANETINTERFACE_GetCurrentValue@44")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetLimits=_DIANETINTERFACE_GetLimits@72")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetDosis=_DIANETINTERFACE_GetDosis@36")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetReload=_DIANETINTERFACE_GetReload@16")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetReload=_DIANETINTERFACE_SetReload@16")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetProposal=_DIANETINTERFACE_GetProposal@12")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetProposal=_DIANETINTERFACE_SetProposal@12")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_GetErrorString=_DIANETINTERFACE_GetErrorString@4")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_OpenLogFile=_DIANETINTERFACE_OpenLogFile@16")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_CloseLogFile=_DIANETINTERFACE_CloseLogFile@0")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_LogMessage=_DIANETINTERFACE_LogMessage@8")
	#pragma comment(linker, "/EXPORT:DIANETINTERFACE_SetLogTimer=_DIANETINTERFACE_SetLogTimer@4")
#endif 

//----------------------------------------------------
// DLL GLOBALS and statics

ATL::CTraceCategory myTrace("DianetStartCOM", 1);

// define a class that is used to create instances for each communication channel
class DeviceInfo
{
public:
	DeviceInfo()
	{
		// communication stuff
		ZeroMemory(&_ident, sizeof(_ident));
		_mode		= CDS_MODE_DOKU;
		_address	= CDS_ADR_INVALID;
		_handle		= INVALID_HANDLE_VALUE;

		ZeroMemory(_szPort, sizeof(_szPort));
		
		// Heartbeat thread stuff
		_dwHeartbeatInterval = 500;
		_abortThread = false;
		ZeroMemory(&_cs, sizeof(_cs));
		_evt = 0;
		_hThread = NULL;
		_lastAck = 0;
		_dwThreadId = 0;
	}

	~DeviceInfo()
	{
		StopThread();
	}

	// com parameters
	HANDLE			_handle;
	TDS_DATA_IDENT	_ident;
	UINT8			_mode;
	UINT8			_address;

	// for logging we also keep the readable port name:
	char			_szPort[16];

	// -------------------------------------------------------------------------------------------------------
	// Heartbeat thread 
	// need a separate thread for keeping the device in cc/ca mode by repeatedly sending a nop packet
	// define a few statics here...
	volatile DWORD _dwHeartbeatInterval;
	volatile bool _abortThread;
	CRITICAL_SECTION _cs;
	HANDLE _evt;
	HANDLE _hThread;
	volatile ULONGLONG _lastAck;
	DWORD _dwThreadId;

	// member functions
	UINT8 Cmd(TDS_RECV * pData, TDS_REPLY * pReply);
	void StartThread();
	void StopThread();

protected:
	UINT8 ReceiveData(TDS_REPLY& m_RecData);
};

// create a map for storing communication instances; com handle is key
class DeviceMap : public std::map<HANDLE, DeviceInfo>
{
	bool isKey(HANDLE h) const { return find(h)!=end(); }
};

static DeviceMap		_deviceMap;

//----------------------------------------------------
// logging
// optional log file (or stdout)

static FILE* _hLogFile = NULL; // or stdout
static DWORD _iLogLevel = 0; // higher values are more detailed
static LONGLONG _i64LogTickOffset = 0; // this enables to print relative to other clock (i.e. eprime)

void PrintTimeStamp(const char* szPostfix)
{
	if (_hLogFile)
	{
		#if _WIN32_WINNT >= 0x0600
			const LONGLONG tNow = (LONGLONG)(GetTickCount64()) + _i64LogTickOffset;
		#else
			static ULONGLONG _prev_tick = 0;
			ULONGLONG ticknow = GetTickCount();
			while (ticknow<_prev_tick) // overflow after 49 days uptime
				ticknow += 0xFFFFFFFF;
			_prev_tick = ticknow;
			const LONGLONG tNow = (LONGLONG)(ticknow) + _i64LogTickOffset;
		#endif
		fprintf(_hLogFile,"----------\n%010I64d %s\n",tNow,(szPostfix?szPostfix:""));
	}
}

inline bool IsValid(DWORD v)
{
	return v!=-1;
}

inline bool matchflags(DWORD& f, DWORD bits)
{
	if ((f&bits)==bits)
	{
		f = f & (~bits); // strip matching bits
		return true;
	}
	else
		return false;
}

char* FlagToUnits(DWORD f, char* s, size_t n)
{
	if (f==-1)
		strcpy_s(s,n,"N/A");
	else
	{
		*s = '\0';
		if (matchflags(f,CDS_EINHEIT_MG))	strcat_s(s,n,"mg ");
		if (matchflags(f,CDS_EINHEIT_UG))	strcat_s(s,n,"ug ");
		if (matchflags(f,CDS_EINHEIT_IE))	strcat_s(s,n,"IE ");
		if (matchflags(f,CDS_EINHEIT_MMOL))	strcat_s(s,n,"mmol ");
		if (matchflags(f,CDS_EINHEIT_ML))	strcat_s(s,n,"ml ");
		if (matchflags(f,CDS_EINHEIT_KG))	strcat_s(s,n,"kg ");
		if (matchflags(f,CDS_EINHEIT_SEC))	strcat_s(s,n,"sec ");
		if (matchflags(f,CDS_EINHEIT_MIN))	strcat_s(s,n,"min ");
		if (matchflags(f,CDS_EINHEIT_H))	strcat_s(s,n,"h ");
		if (matchflags(f,CDS_EINHEIT_24H))	strcat_s(s,n,"24h ");
		if (f) strcat_s(s,n,"unknown bits in unit flag");
	}
	return s;
}

char* PrintableString(const INT8* snz, size_t len, char* sz, size_t n, char cUnspecChar=0)
{
	if (sz)
	{
		ZeroMemory(sz,n);
		if (snz && len)
		{
			size_t i;
			if (n!=-1 && len+1>n)
				len = n-1; // allow space for terminating zero
			for (i=0; i<len; i++)
				sz[i] = snz[i]!=INT8(0xFF) ? snz[i] : cUnspecChar; /* string without 0; 0xFF means a non-configured string (=leave as is) */
			//sz[i] = '\0'; not required because ZeroMemory did this
		}
	}
	return sz;
}

char* PrintValue(char* buf, size_t nBufSize, UINT8 val)
{
	if (val==0xFF)
		strcpy_s(buf,nBufSize,"=");
	else
		sprintf_s(buf,nBufSize,"%u",(unsigned int)val);
	return buf;
}

char* PrintValue(char* buf, size_t nBufSize, UINT16 val)
{
	if (val==0xFFFF)
		strcpy_s(buf,nBufSize,"=");
	else
		sprintf_s(buf,nBufSize,"%u",(unsigned int)val);
	return buf;
}

char* PrintValue(char* buf, size_t nBufSize, UINT32 val)
{
	if (val==0xFFFFFFFF)
		strcpy_s(buf,nBufSize,"=");
	else
		sprintf_s(buf,nBufSize,"%lu",(unsigned int)val);
	return buf;
}

//----------------------------------------------------

#define CHECK_MYHANDLE { if (dev._handle==INVALID_HANDLE_VALUE) { ATLTRACE2(myTrace, 1, "RS232 port not opened\n"); return CDS_ERR_NOTCONNECTED; } }
#define CHECK_IDENT    { if (dev._ident.identnr==0) { ATLTRACE2(myTrace, 1, "IDENT not initialized\n"); return CDS_ERR_IDENT; } }
#define CHECK_MODE(m)  { if (dev._mode!=m) { ATLTRACE2(myTrace, 1, "Wrong mode\n"); return CDS_ERR_MODE; } }
#define CHECK_MODE_REMOTE  { if (dev._mode!=CDS_MODE_FERNSTEUER && dev._mode!=CDS_MODE_FERNREGEL) { ATLTRACE2(myTrace, 1, "Not in remote control mode\n"); return CDS_ERR_MODE; } }

// minimum sizes for send and receive buffers
#define CDS_SENDBUFF_SIZE  (sizeof( TDS_RECV ) * 2)
#define CDS_RCVBUFF_SIZE   (sizeof( TDS_REPLY ) * 2)


struct TdsCommandPacket : TDS_RECV
{
	TdsCommandPacket(UINT8 adress, UINT8 segnr)
	{
		memset(this, 0xff, sizeof(TDS_RECV));
		head.adress = adress;
		head.segnr = segnr;
	}
};

struct TdsReplyPacket : TDS_REPLY
{
	TdsReplyPacket()
	{
		memset(this, 0xff, sizeof(TDS_REPLY));
	}
};

inline void assign(UINT8& lvalue, DWORD rvalue)
{
	ATLASSERT(rvalue==0xFFFFFFFF || (rvalue&0xFFFFFF00)==0);
	lvalue = (UINT8)rvalue;
}

inline void assign(UINT16& lvalue, DWORD rvalue)
{
	ATLASSERT(rvalue==0xFFFFFFFF || (rvalue&0xFFFF0000)==0);
	lvalue = (UINT16)rvalue;
}

inline void assign(UINT32& lvalue, DWORD rvalue)
{
	lvalue = rvalue;
}

inline void assign(DWORD* lvalue, UINT8 rvalue)
{
	if (lvalue)
	{
		if ((rvalue&0xFF)==0xFF)
			*lvalue = -1;
		else
			*lvalue = rvalue;
	}
}

inline void assign(DWORD* lvalue, UINT16 rvalue)
{
	if (lvalue)
	{
		if ((rvalue&0xFFFF)==0xFFFF)
			*lvalue = -1;
		else
			*lvalue = rvalue;
	}
}

inline void assign(DWORD* lvalue, UINT32 rvalue)
{
	if (lvalue)
		*lvalue = rvalue;
}

//-----------------------------------------------------------------------------
// IO buffer management (byte stuffing, framing, CRC, ...)

// a 'routine' that copies *data to pdest and performs byte stuffing
// TODO: make a inline of this...
#define SETDESTBUFFER( data )                           \
{                                                       \
	if ((data == CDS_RAHMEN_BYTE) ||                    \
		(data == CDS_MONITOR_BYTE) ||                   \
		(data == CDS_STUFF_BYTE))                       \
    {                                                   \
       *pdest = CDS_STUFF_BYTE;                         \
       pdest++;                                         \
       *pdest = (UINT8) (data ^ CDS_STUFFCODE_BYTE);    \
       pdest++;                                         \
       retcount+= (UINT8)2;                             \
    }                                                   \
    else                                                \
    {                                                   \
      *pdest = data;                                    \
      pdest++;                                          \
      retcount++;                                       \
    }                                                   \
}

static UINT8 BuildCRC(UINT8 *psrc, UINT8 uSendLen, UINT8 *pdest)
{
	UINT8 data;
	UINT8 count;
	UINT8 retcount = 0;
	UINT16 crc = (UINT16) - 1;

	*pdest = CDS_RAHMEN_BYTE;
	pdest++;
	retcount++;
	for (count = 0; count < uSendLen; count++)
	{
		data = *psrc;
		psrc++;
		crc = dscrcbyte(crc, data);
		SETDESTBUFFER(data);
	}
	data = (UINT8) (crc >> 8);
	SETDESTBUFFER(data);
	data = (UINT8) ((UINT8) crc & 0xff);
	SETDESTBUFFER(data);
	*pdest = CDS_RAHMEN_BYTE;
	retcount++;
	return retcount;
}

static bool StoreData(UINT8 data, int& nBytesStored, bool& bByteStuff, UINT8* pBuf, int nBufSize, TDS_CRC& uCalcCRC, UINT8& uRecError)
{
	bool dataready = false;
	if (data != CDS_MONITOR_BYTE)
	{
		// set stuffing flag if byte is a stuffing marker
		if (data == CDS_STUFF_BYTE)
		{
			bByteStuff = true;
		}
		else
		{
			// regular data byte: remove stuffing if marked earlier
			if (bByteStuff == true)
			{
				bByteStuff = false;
				data ^= CDS_STUFFCODE_BYTE;
			}
			// check free space in buffer
			if (nBytesStored < nBufSize)
			{
				pBuf[nBytesStored] = data;
				// Start CRC calculation after received CRC (CRC calculation excludes transfered CRC)
				if (nBytesStored >= (sizeof(TDS_CRC)))
				{
					uCalcCRC = dscrcbyte(uCalcCRC, pBuf[nBytesStored - sizeof(TDS_CRC)]);
				}
				nBytesStored++;
			}
			else
			{
				// buffer overflow
				bByteStuff = false;
				nBytesStored = 0;
				uRecError = CDS_ERR_SEG_LENGTH;
				dataready = true;
			}
		}
	}
	else
	{
		// corrupt data
		bByteStuff = false;
		nBytesStored = 0;
		uRecError = CDS_ERR_STUFFING;
		dataready = true;
	}
	return (dataready);
}

//-----------------------------------------------------------------------------

// BYTE NO.	BYTE MEANING	COMMENT
// 0		01111110		------------ Frame ----------	Packet
// 1		Address						Data set	Block	Packet
// 2		Segment ID					Data set	Block	Packet
// 3		Date MSB		Data range	Data set	Block	Packet
// :		Data array		Data range	Data set	Block	Packet
// n		Date LSB		Data range	Data set	Block	Packet
// n + 1	CRC MSB			CRC range				Block	Packet
// n + 2	CRC LSB			CRC range				Block	Packet
// n + 3 	01111110		------------ Frame ----------	Packet

UINT8 DeviceInfo::ReceiveData(TDS_REPLY& m_RecData)
{
#define CDS_RECST_WAIT_FRAME			1		// waiting for start of frame
#define CDS_RECST_WAIT_DATASET1			2		// waiting for first dataset byte
#define CDS_RECST_WAIT_DATASETX			3		// wating for rest of dataset (incl CRC)

#define CDS_REC_TIMEOUT_MS				2000	// timout after 2 sec

	bool bWatchTimeout = true;
	bool dataready = false;
	TDS_CRC uReceiveCRC = CRC_START_VAL;
	UINT8 recdata;
	UINT8 uRecError = 0;
	bool bByteStuff = false;
	int nBytesStored = 0;
	#if _WIN32_WINNT >= 0x0600
		const ULONGLONG ullTimeoutTime = GetTickCount64()+CDS_REC_TIMEOUT_MS;
	#else
		const DWORD dwStartTime = GetTickCount();
		const DWORD dwTimeoutTime = dwStartTime+CDS_REC_TIMEOUT_MS; // this will overflow if CPU uptime is a multiple off 49.7 days
	#endif
	int iReceiveState = CDS_RECST_WAIT_FRAME;
	TDS_CRC uCalcCRC = CRC_START_VAL;
	UINT8 cReceiveBuffer[CDS_RCVBUFF_SIZE];

	// buffer for logging:
	UINT8 cRawBuffer[CDS_RCVBUFF_SIZE]; 
	size_t nRawBuffer = 0;

	do
	{
		if (IO_RS232_ReadByte(_handle,(CHAR*)&recdata)==ERROR_SUCCESS)
		{
			if (_hLogFile!=NULL && nRawBuffer+1<sizeof(cRawBuffer))
				cRawBuffer[nRawBuffer++] = recdata;

			switch (iReceiveState)
			{
			case CDS_RECST_WAIT_FRAME:	// wait for start of frame
				// start of frame detected -> start waiting for data set 
				if (recdata == CDS_RAHMEN_BYTE)
				{
					iReceiveState = CDS_RECST_WAIT_DATASET1;
				}
				break;
			case CDS_RECST_WAIT_DATASET1:	// waiting for start of data set
				// accept data set if byte is not a frameing byte; else keep trying 
				if (recdata != CDS_RAHMEN_BYTE)
				{
					// translate first byte after frameing byte to receive buffer 
					iReceiveState = CDS_RECST_WAIT_DATASETX;
					dataready = StoreData(recdata, nBytesStored, bByteStuff, cReceiveBuffer, sizeof(cReceiveBuffer), uCalcCRC, uRecError);
				}
				break;
			case CDS_RECST_WAIT_DATASETX: // wait for rest of dataset
				// next byte: end of dataset?
				if (recdata == CDS_RAHMEN_BYTE)
				{
					// need at least a CRC at the end of the dataset
					if (nBytesStored > sizeof(TDS_CRC))
					{
						// step back: let index point to start of CRC
						nBytesStored -= (UINT8) sizeof(TDS_CRC);
						// assemble the CRC bytes
						uReceiveCRC = cReceiveBuffer[nBytesStored];
						uReceiveCRC <<= 8;
						uReceiveCRC |= cReceiveBuffer[nBytesStored + 1];
						// CRC must match
						if (uCalcCRC != uReceiveCRC)
						{
							// CRC mismatch: data corruption
							uRecError = CDS_ERR_CRC;
							ATLTRACE2(myTrace, 1, " CRC mismatch\n");
						}
						else
						{
							// data OK: translate bytes to structure (endian conversion) 
							nBytesStored = dslrrplbuff2struct(nBytesStored, &uRecError, cReceiveBuffer, &m_RecData);
						}
					}
					else
					{
						// CRC incomplete - framing failure
						uRecError = CDS_ERR_CRC;
						ATLTRACE2(myTrace, 1, " CRC not complete\n");
					}
					// end of packet (even if data is corrupt)
					dataready = true;
				}
				else // not the end of this packet
				{
					// just translate a regular byte of dataset to receive buffer
					dataready = StoreData(recdata, nBytesStored, bByteStuff, cReceiveBuffer, sizeof(cReceiveBuffer), uCalcCRC, uRecError);
				}
				break;
			}
		}
		if (!dataready && bWatchTimeout)
		{
			#if _WIN32_WINNT >= 0x0600
				const bool bTimeout = GetTickCount64()>ullTimeoutTime;
			#else
				const DWORD tNow = GetTickCount();
				// note that we can only timeout after an overflow if dwStartTime > dwTimeoutTime
				const bool bTimeout = tNow>dwTimeoutTime && (dwStartTime<dwTimeoutTime ? true : tNow<dwStartTime);
			#endif
			if (bTimeout)
			{
				uRecError = CDS_ERR_TIMEOUT;
				dataready = true;
				ATLTRACE2(myTrace, 2, " timeout\n");
			}
		}
	}
	while (dataready == false);

	if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_IO || uRecError!=0))
	{
		PrintTimeStamp(_szPort);
		fprintf(_hLogFile,"IO RECEIVE : %lu [%s]\n",uRecError,DIANETINTERFACE_GetErrorString(uRecError));
		fprintf(_hLogFile,"       CRC : 0x%04hx [received 0x%04hx]\n", uCalcCRC, uReceiveCRC);
		fprintf(_hLogFile,"      DATA :            :           |           :           ");
			for (size_t i=0; i<nRawBuffer; i++)
			{
				if ((i%16)==0) fprintf(_hLogFile,"\n            ");
				fprintf(_hLogFile," %02x",(unsigned int)cRawBuffer[i]);
			}
		fprintf(_hLogFile,"\n");
	}
	return uRecError;
}

// This is the central packet send-receive command.
UINT8 DeviceInfo::Cmd(TDS_RECV * pData, TDS_REPLY * pReply)
{
	if (!IO_RS232_IsConnected(_handle))
	{
		ATLTRACE2(myTrace, 1, "skip transaction: device not connected\n");
		return CDS_ERR_NOTCONNECTED;
	}

	UINT8 ret = CDS_ERR_NC;

	// make sure we don't interfere with the heartbeat thread
	if (_hThread)
		EnterCriticalSection(&_cs);

#ifdef _DEBUG
	COMSTAT stat;
	ClearCommError(_handle, &stat.cbInQue, NULL);
	if (stat.cbInQue>0)
	{
		ATLTRACE2(myTrace, 1, "buffer not empty: %ld bytes\n", stat.cbInQue);
	}
//	IO_RS232_ResetSerial(hDevice); // <<<< WAS NEEDED BECAUSE ONE OF THE FUNCTIONS ACK-ED WITH MORE BYTES THEN EXPECTED...
#endif
	UINT8 uSendBufferUnstuffed[CDS_SENDBUFF_SIZE];
	UINT8 uSendBufferStuffed[CDS_SENDBUFF_SIZE];
	UINT8 segnr = pData->head.segnr;
	UINT8 uSendLen = dslrsndstruct2buff(pData, uSendBufferUnstuffed);		// endian conversion dataset
	uSendLen = BuildCRC(uSendBufferUnstuffed,uSendLen,uSendBufferStuffed);	// stuff reserved bytes, add CRC, wrap dataset in frame

	IO_RS232_ResetSerial(_handle);
//	StartTimer(segnr);
	if (IO_RS232_SendBuffer(_handle, (LPCSTR)uSendBufferStuffed, uSendLen)!=ERROR_SUCCESS)
	{
		ATLTRACE2(myTrace, 1, "ERROR: SEND failed\n");
		ret = CDS_ERR_RS232_ERROR;
	}
	if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_IO || ret!=CDS_ERR_NC))
	{
		PrintTimeStamp(_szPort);
		fprintf(_hLogFile,"IO SEND    : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
		fprintf(_hLogFile,"    segment: %u\n",(unsigned int)segnr);
		fprintf(_hLogFile,"        len: %u\n",(unsigned int)uSendLen);
	}
	if (ret!=CDS_ERR_NC)
		return ret;

	TDS_REPLY m_RecData;
	ret = ReceiveData(m_RecData);
//	StopTimer(ret);
	if (ret == CDS_ERR_NC)
	{
		if (pReply != NULL)
		{
			memcpy(pReply, &m_RecData, sizeof(TDS_REPLY));
		}
		if (m_RecData.head._code == CDS_EVENT_NAK)
		{
			ret = m_RecData._data.nak.errcode; // TODO: loosing error info here
		}
		else
		{
			// remember the current time of this packet, so heartbeat can continue waiting...
			#if _WIN32_WINNT >= 0x0600
				_lastAck = GetTickCount64();
			#else
				_lastAck = GetTickCount();
			#endif
		}
	}
	if (ret != CDS_ERR_NC)
		ATLTRACE2(myTrace, 1, "ERROR: RECV failed with code %u\n",(unsigned int)ret);

#ifdef _DEBUG
	//COMSTAT stat;
	ClearCommError(_handle, &stat.cbInQue, NULL);
	if (stat.cbInQue>0)
	{
		ATLTRACE2(myTrace, 1, "buffer not empty: %ld bytes\n", stat.cbInQue);
	}
#endif

	// release the critical section for heartbeat thread to continue when required
	if (_hThread)
		LeaveCriticalSection(&_cs);

	return ret;	
}

// -------------------------------------------------------------------------------------------------------
// Heartbeat thread 
// need a separate thread for keeping the device in cc/ca mode by repeatedly sending a nop packet

static DWORD WINAPI ThreadProc_cc_heartbeat(LPVOID lpParameter)
{
	DeviceInfo* pDev = static_cast<DeviceInfo*>(lpParameter);

	DWORD nextsleeptime = pDev->_dwHeartbeatInterval; 
	while (!pDev->_abortThread)
	{
		bool bInterrupt = false;
		switch (WaitForSingleObject(pDev->_evt,nextsleeptime))
		{
			case WAIT_OBJECT_0: // event was signaled, either _dwHeartbeatInterval or _abortThread was changed
				// don't break; we must recalculate sleep time
				bInterrupt = true;
			case WAIT_TIMEOUT:
				nextsleeptime = pDev->_dwHeartbeatInterval; 
				if (IO_RS232_IsConnected(pDev->_handle) && !pDev->_abortThread)
				{
					// check if time between proper message and current time is almost the same as heartbeat
					#if _WIN32_WINNT >= 0x0600
						ULONGLONG tick = GetTickCount64();
					#else
						ULONGLONG tick = GetTickCount();
						while (tick<pDev->_lastAck) // overflow after 49 days uptime
							tick += 0xFFFFFFFF;
					#endif
					DWORD tdiff = DWORD(tick - pDev->_lastAck);
					if (tdiff+50>pDev->_dwHeartbeatInterval)
					{
						if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
						{
							PrintTimeStamp(pDev->_szPort);
							fprintf(_hLogFile,"PING  IDENT: diff prev ack=%lu\n",tdiff);
						}
						// just request IDENT to stay in CC/CA mode
						DWORD ret = DIANETINTERFACE_GetIdent(pDev->_handle, NULL, NULL, NULL, NULL, NULL, 0);
						if (ret==CDS_ERR_CRC)
						{
							ATLTRACE2(myTrace, 1, "ERROR: first CRC error in heartbeat, retry... \n");
							ret = DIANETINTERFACE_GetIdent(pDev->_handle, NULL, NULL, NULL, NULL, NULL, 0); // thread save call
						}
						if (ret!=CDS_ERR_NC)
						{
							ATLTRACE2(myTrace, 1, "ERROR: heartbeat failed, entering idle state\n");
							if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
							{
								PrintTimeStamp(pDev->_szPort);
								fprintf(_hLogFile,"PING ERROR : ERROR: heartbeat failed, entering idle state\n");
							}
							nextsleeptime = INFINITE;
						}
					}
					else
					{
						// postpone this heartbeat because a valid package was acknowledged a short while ago
						nextsleeptime = pDev->_dwHeartbeatInterval - tdiff; // which is at least 50 msec
						if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
						{
							PrintTimeStamp(pDev->_szPort);
							fprintf(_hLogFile,"PING %s: postpone with %lu ms (diff prev ack=%lu)\n",(bInterrupt?"IRQ   ":"TIMOUT"),nextsleeptime,tdiff);
						}
					}
				}
				else
				{
					if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
					{
						PrintTimeStamp(pDev->_szPort);
						fprintf(_hLogFile,"PING ERROR : ERROR: heartbeat failed because device is not connected, entering idle state\n");
					}
					ATLTRACE2(myTrace, 1, "ERROR: heartbeat failed because device is not connected, entering idle state\n");
					nextsleeptime = INFINITE;
				}
				break;

			default:
				pDev->_abortThread = true;
				ATLTRACE2(myTrace, 1, "ERROR: abort heartbeat thread because of unknown event\n");
		}
	}
	if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
	{
		PrintTimeStamp(pDev->_szPort);
		fprintf(_hLogFile,"PING STOP  : heartbeat thread stopped\n");
	}
	return 0;
}

void DeviceInfo::StartThread()
{
	if (_hThread==NULL)
	{
		_abortThread = false;
		_lastAck = 0;
		InitializeCriticalSection(&_cs);
		_evt = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hThread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            ThreadProc_cc_heartbeat, // thread function 
            this,              // argument to thread function 
            0,                 // use default creation flags 
            &_dwThreadId);   // returns the thread identifier 
	}
}

void DeviceInfo::StopThread()
{
	if (_hThread)
	{
		_abortThread = true;
		SetEvent(_evt);
		WaitForSingleObject(_hThread,INFINITE);	_dwThreadId = 0; _hThread = NULL;
		CloseHandle(_evt); _evt = NULL;
		DeleteCriticalSection(&_cs);
	}
}

// -------------------------------------------------------------------------------------------------------

extern "C"
{
	DIANETINTERFACE_DLLX BOOL DLLENTRY DIANETINTERFACE_IsConnected(HANDLE h)
	{
		return IO_RS232_IsConnected(h);
	}

	DIANETINTERFACE_DLLX VOID DLLENTRY DIANETINTERFACE_Disconnect(HANDLE h)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i!=_deviceMap.end())
		{
			(*i).second.StopThread(); // first stop heartbeat thread
			IO_RS232_Disconnect(h);
			_deviceMap.erase(i);
		}
	}

	DIANETINTERFACE_DLLX HANDLE DLLENTRY DIANETINTERFACE_Connect(LPCSTR szPort /*\\.\COM1 baud=1200 parity=N data=8 stop=1*/, LPCSTR szDevice)
	{
		const HANDLE h = IO_RS232_Connect(szPort);
		if (h!=INVALID_HANDLE_VALUE)
		{
			DeviceInfo& inf = _deviceMap[h];
			inf._handle = h;
			strcpy_s(inf._szPort,sizeof(inf._szPort),szPort);
		}
		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || h==INVALID_HANDLE_VALUE))
		{
			PrintTimeStamp(szPort);
			fprintf(_hLogFile,"CONNECT    : %s [%s][%s]\n",(h==INVALID_HANDLE_VALUE?"FAILED":"OK"),szPort,szDevice);
		}
		return h;
	}

	DIANETINTERFACE_DLLX VOID DLLENTRY DIANETINTERFACE_InitHeartbeat(HANDLE h, DWORD interval_ms)
	{
		// start heartbeat thread, or change interval of running thread
		// interval_ms can be 0xFFFFFFFF to enter suspend mode
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i!=_deviceMap.end())
		{
			DeviceInfo& dev = (*i).second;
			dev._dwHeartbeatInterval = interval_ms; // this is not by the book, but OK
			if (dev._hThread==NULL)
				dev.StartThread(); // also make sure the heartbeat thread is started
			else if (dev._evt)
				SetEvent(dev._evt); // signal thread about changed interval

			if (_hLogFile!=NULL && _iLogLevel>=CDS_LOGLEVEL_INFO)
			{
				PrintTimeStamp(dev._szPort);
				fprintf(_hLogFile,"INIT HEART BEAT: %lu [ms]\n",interval_ms);
			}
		}
	}

	DIANETINTERFACE_DLLX VOID DLLENTRY DIANETINTERFACE_ResetConnection(HANDLE h)
	{
		if (h!=INVALID_HANDLE_VALUE)
			IO_RS232_ResetSerial(h);
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetIdent(HANDLE h, DWORD* pSerial, DWORD* pType, USHORT* pProtoVers, DWORD* pUnixTime, LPSTR szSwVers, USHORT nBytes)
	{
		TdsCommandPacket sendPacket(0,CDS_RSEG_IDENT);
		TdsReplyPacket   replyPacket;

		// temp. copies for logging:
		DWORD dwType(0), dwSerial(0), dwUnixTime(0); 
		USHORT uProtoVers(0);

		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;
		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret==CDS_ERR_NC)
		{
			dev._ident = replyPacket._data.ident;
			// identnr
			// Bits 0 to 17 represent the serial no. in the value range 1 .... 3FFFFH
			// Bits 18 to 31 represent the Dianet type no. in the value range 1 .... 3FFFH
			dwSerial = dev._ident.identnr & 0x0003ffff;
			dwType = dev._ident.identnr >> 18;
			dwUnixTime = dev._ident.unixtime;
			uProtoVers = dev._ident.protovers;
			if (pSerial)	*pSerial	= dwSerial;
			if (pType)		*pType		= dwType;
			if (pProtoVers)	*pProtoVers	= uProtoVers;
			if (pUnixTime)	*pUnixTime	= dwUnixTime;
			if (szSwVers && nBytes)	
			{
				memset(szSwVers,0,nBytes);
				// make sure the string only contains printable stuff
				for (int i=0; i<sizeof(dev._ident.swvers) && i+1<nBytes; i++)
					szSwVers[i] = isalnum(dev._ident.swvers[i]) ? dev._ident.swvers[i] : ' ';
			}
		}
		if (_hLogFile!=NULL && _iLogLevel>=CDS_LOGLEVEL_INFO)
		{
			if (pSerial==NULL && pType==NULL && pProtoVers==NULL && pUnixTime==NULL && szSwVers==NULL)
			{
				// nothing to return: assume NOP ping (i.e. keep-connection-alive beat)
				// fprintf(_hLogFile,"PING  IDENT: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			}
			else
			{
				PrintTimeStamp(dev._szPort);
				fprintf(_hLogFile,"      IDENT: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
				if (ret==CDS_ERR_NC)
				{
					// convert the returned time value to string
					struct tm unix_tm;
					char sz_unix_time[32] = "invalid time\n"; // note: asctime add's \n;
					if (_localtime32_s(&unix_tm, (__time32_t*)&dwUnixTime)==0)
						asctime_s(sz_unix_time,sizeof(sz_unix_time), &unix_tm); 

					const char* szType = NULL;
					switch (dwType)
					{
					case  202: szType = "Infusomat fmS"; break;
					case  203: szType = "Infusomat P"; break;
					case 1019: szType = "Perfusor fm"; break;
					case 1020: szType = "Perfusor fm ul-Aachen"; break;
					case 1200: szType = "Perfusor compact"; break;
					case 1201: szType = "Perfusor compact S"; break;
					default: szType = "Unknown"; break;
					}

					fprintf(_hLogFile,"     serial: %lu\n", dwSerial);
					fprintf(_hLogFile,"       type: %lu = %s\n", dwType, szType);
					fprintf(_hLogFile,"   protocol: %lx\n", uProtoVers);
					fprintf(_hLogFile,"       time: %lu = %s", dwUnixTime,  sz_unix_time); 
					fprintf(_hLogFile," sw version: %.*s\n", (int)CDS_SWVER_LEN, dev._ident.swvers);
					// or only returned printable stuff:
					// if (szSwVers && nBytes)
					//		fprintf(_hLogFile," sw version: %s\n", szSwVers);
				}
			}
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetMode(HANDLE h, DWORD newaddres, DWORD newmode, DWORD unixtime)
	{
		// Make sure to call DIANETINTERFACE_Connect() before using this function.
		// Make sure to call DIANETINTERFACE_GetIdent() before using this function.
		//
		// address should be [1-26] or ['A'-'Z']
		// newmode should be one off:
		//		CDS_MODE_DOKU		0x00 initial state; documentation state
		//		CDS_MODE_FERNSTEUER 0x01 remote control; manual control allowed
		//		CDS_MODE_FERNREGEL	0x02 remote control; manual control blocked
		//		CDS_MODE_MONREQ		0x03 Monitoraktivierung wird erwartet
		// unixtime should be a valid unix time(), -1 (to ignore time setting) or 0 (to set current time)

		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		CHECK_MYHANDLE				// make sure the serial port is opened
		CHECK_IDENT					// make sure IDENT structure (_ident) is valid
//		CHECK_MODE(CDS_MODE_DOKU)	// make sure mode is not already changed (however, the pump may fall back to mode 0 after a timeout)

		TdsCommandPacket sendPacket(0,CDS_WSEG_CC_MODE); // can only switch mode and assign an address if current address is 0
		TdsReplyPacket   replyPacket;

		// check address
		if (newaddres>0 && newaddres<=CDS_ADR_MAX-CDS_ADR_MIN)
		{
			newaddres = CDS_ADR_MIN + newaddres -1;
			ATLTRACE2(myTrace, 2, "SET MODE: Translating decimal address to device address: %c\n",(char)newaddres);
		}
		else if (newaddres>=CDS_ADR_MIN && newaddres<=CDS_ADR_MAX)
			; // newaddres is valid
		else
		{
			ATLTRACE2(myTrace, 1, "SET MODE: Invalid device address (dec=%lu), using '%c' instead.\n",newaddres,(char)CDS_ADR_MIN);
			newaddres = CDS_ADR_MIN; // use first valid address if no address was given
		}

		if (unixtime==0) // unixtime==-1 is allowed and means: don't update time
			unixtime = _time32(NULL);

		ATLASSERT(newmode<=CDS_MODE_MONREQ);
		ATLTRACE2(myTrace, 2, "CC_MODE: address=%c, mode=%lu, unix time=%lu\n", (char)newaddres, newmode, unixtime);

		TDS_DATA_CCMODE& t = sendPacket._data.ccmode;
		t.identnrinv	= ~dev._ident.identnr;	// must be inverted invent number
		assign(t.modeswitch,newmode);
		assign(t.newadress, newaddres);
		assign(t.unixtime, unixtime);

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret==CDS_ERR_NC)
		{
			// save relevant settings for later use
			assign(dev._mode, newmode);
			assign(dev._address,newaddres);
		}
		else if (ret==CDS_ERR_IDENT)
		{
			// In case of a faulty comparison of the inverted serial number, the pump changes to documentation mode			
			dev._mode = CDS_MODE_DOKU;
			ATLTRACE2(myTrace, 2, "ERROR: SET MODE wrong identification\n");
		}
		else
			ATLTRACE2(myTrace, 2, "ERROR: SET MODE failed with code %i\n", (int)ret);

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"MODE SWITCH: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				const char* szMode = NULL;
				switch (newmode)
				{
				case CDS_MODE_DOKU: szMode = "Start state. Documentation mode"; break;
				case CDS_MODE_FERNSTEUER: szMode = "Remote control (manual operation allowed)"; break;
				case CDS_MODE_FERNREGEL: szMode = "Remote control (manual operation blocked)"; break;
				case CDS_MODE_MONREQ: szMode = "Monitor mode"; break;
				default: szMode = "Unknown mode"; break;
				}
				fprintf(_hLogFile,"       mode: %s\n", szMode);
			}
		}

		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetAction(HANDLE h, DWORD action_id, DWORD value)
	{
		// action_id:
		// CDS_AKT_START_STOP		0x00	/* Pumpe starten / stoppen */
		// CDS_AKT_ALARM_AUS		0x01	/* Alarme sperren / freigeben */
		// CDS_AKT_DATA_LOCK		0x02	/* Eingabe sperren / freigeben */
		// CDS_AKT_INFUSED_ZEIT_0	0x03	/* 1= loescht Gesamtzeit seit Einschalten */
		// CDS_AKT_INFUSED_VOL_0	0x04	/* 1= loescht Gesamtvolumen seit Einschalten */
		// CDS_AKT_BOL_MAN_START	0x05	/* startet Bolus fuer 1s / Stop Bolus */
		// CDS_AKT_BOL_VOL_START	0x06	/* startet Volumenbolus / Stop Bolus & loesche <BOL_VOL> */
		// CDS_AKT_KONTR_TROPF_EIN	0x07	/* Tropfueberwachung ein / aus */
		// CDS_AKT_STDBY			0x08	/* Standby-Funktion ein / aus */
		// CDS_AKT_HISTORY_CLEAR	0x09	/* 1= History-Speicher loeschen */
		// CDS_AKT_DOSIS_OFF		0x0a	/* 1=Dosiskalkulation ausschalten */
		// CDS_AKT_BOL_INTV_OFF		0x0b	/* 1=Intervallbolus ausschalten */
		// CDS_LASTVALID_AKT		CDS_AKT_BOL_INTV_OFF	/* Letzte gueltige Aktionskennung */

		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_WSEG_AKTION); 
		TdsReplyPacket   replyPacket;

		ATLASSERT(action_id<=CDS_LASTVALID_AKT);
		ATLASSERT(value<=1);

		TDS_DATA_AKTION& t = sendPacket._data.aktion;
		assign(t.kennung, action_id);
		assign(t.flag, value);

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: SET ACTION failed with code %i\n", (int)ret);

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"SEND ACTION: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			const char* szAction = NULL;
			switch (action_id)
			{
			case CDS_AKT_START_STOP:		szAction = (value==0?"Stop pump":"Start pump"); break;
			case CDS_AKT_ALARM_AUS:			szAction = (value==0?"Alarm enabled":"Alarm disabled"); break;
			case CDS_AKT_DATA_LOCK:			szAction = (value==0?"Data input lock off":"Data input lock on"); break;
			case CDS_AKT_INFUSED_ZEIT_0:	szAction = (value==0?"unknown action for infused time":"Clear infused time"); break;
			case CDS_AKT_INFUSED_VOL_0:		szAction = (value==0?"unknown action for infused volume":"Clear infused volume"); break;
			case CDS_AKT_BOL_MAN_START:		szAction = (value==0?"Stop manual 1-sec bolus":"Start manual 1-sec bolus"); break;
			case CDS_AKT_BOL_VOL_START:		szAction = (value==0?"Stop volume bolus and clear VB":"Start volume bolus"); break;
			case CDS_AKT_KONTR_TROPF_EIN:	szAction = (value==0?"Enable drop monitoring":"Disable drop monitoring"); break;
			case CDS_AKT_STDBY:				szAction = (value==0?"Standby function off":"Standby function on"); break;
			case CDS_AKT_HISTORY_CLEAR:		szAction = (value==0?"Unknown action for clear history":"Clear history"); break;
			case CDS_AKT_DOSIS_OFF:			szAction = (value==0?"Unknown action for dosis calculation":"Switch off dosis calculation"); break;
			case CDS_AKT_BOL_INTV_OFF:		szAction = (value==0?"Unknown action for interval bolus":"Switch off interval bolus"); break;
			default: szAction = "Unknown action ID"; break;
			}
			fprintf(_hLogFile,"     action: %s\n", szAction);
			fprintf(_hLogFile,"      value: %lu\n", value);
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetStatus(HANDLE h, DWORD* request, DWORD* alarm, DWORD* zustand, DWORD* rate, DWORD* remainvtbd, DWORD* remaintime)
	{

		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_STATUS); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET STATUS failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_STATUS& status = replyPacket._data.status;
			assign(request, status.request);		/* actions have occurred on teh pump: operation, limit value mod., alarm suppression */
			assign(alarm, status.alarm);			/* actual alarm */
			assign(zustand, status.zustand);		/* operating mode on pump */
			assign(rate, status.rate);				/* actual rate [ 10�l/h] */
			assign(remainvtbd, status.remainvtbd);	/* remaining volume to be delivered (vtbd) [0.1ml] */
			assign(remaintime, status.remaintime);	/* remaining time of TIME for vtbd [min] */
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET STATUS : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				const TDS_DATA_STATUS& status = replyPacket._data.status;
				DWORD t;

				t = status.request;
				fprintf(_hLogFile,"    request: 0x%08lx\n", t);
				if (t&(1<<CDS_SOLLWERT_BIT_NR))				fprintf(_hLogFile,"           : Target values changed and must be queried\n");
				if (t&(1<<CDS_LIMITS_BIT_NR))				fprintf(_hLogFile,"           : Target values changed and LIMITS must be queried\n");
				if (t&(1<<CDS_ALARM_OFF_BIT_NR))			fprintf(_hLogFile,"           : Push button for alarm suppression was activated\n");
				if (t&(1<<CDS_DOSIS_BIT_NR))				fprintf(_hLogFile,"           : Dose values changed and must be queried\n");

				t = status.alarm;
				fprintf(_hLogFile,"      alarm: 0x%08lx\n", t);
				if (t&(1<<CDS_SPRITZEN_ALARM_BIT_NR))		fprintf(_hLogFile,"           : syringe locking sensor alarm\n");
				if (t&(1<<CDS_SPRITZEN_VOR_ALARM_BIT_NR))	fprintf(_hLogFile,"           : syringe almost empty alarm\n");
				if (t&(1<<CDS_SPRITZE_LEER_ALARM_BIT_NR))	fprintf(_hLogFile,"           : syringe empty alarm\n");
				if (t&(1<<CDS_DRUCK_ALARM_BIT_NR))			fprintf(_hLogFile,"           : pressure alarm\n");
				if (t&(1<<CDS_LUFT_ALARM_BIT_NR))			fprintf(_hLogFile,"           : air alarm\n");
				if (t&(1<<CDS_TROPF_ALARM_BIT_NR))			fprintf(_hLogFile,"           : drip alarm\n");
				if (t&(1<<CDS_AKKU_LEER_ALARM_BIT_NR))		fprintf(_hLogFile,"           : battery empty alarm\n");
				if (t&(1<<CDS_AKKU_VOR_ALARM_BIT_NR))		fprintf(_hLogFile,"           : battery almost empty alarm\n");
				if (t&(1<<CDS_PUMPENKLAPPE_AUF_ALARM_BIT_NR))	fprintf(_hLogFile,"           : pump alarm\n");
				if (t&(1<<CDS_RATE_UNGUELTIG_ALARM_BIT_NR))	fprintf(_hLogFile,"           : invalid rate after transition from CC to CA\n");
				if (t&(1<<CDS_ERINNER_ALARM_BIT_NR))		fprintf(_hLogFile,"           : reminder alarm\n");
				if (t&(1<<CDS_STANDBY_ENDE_ALARM_BIT_NR))	fprintf(_hLogFile,"           : standby alarm\n");
				if (t&(1<<CDS_VTBD_ENDE_ALARM_BIT_NR))		fprintf(_hLogFile,"           : VTBD has been processed\n");
				if (t&(1<<CDS_ZEIT_ENDE_ALARM_BIT_NR))		fprintf(_hLogFile,"           : TIME has been processed\n");
				if (t&(1<<CDS_CC_ALARRM_BIT_NR))			fprintf(_hLogFile,"           : Communication error\n");
				if (t&(1<<CDS_KVO_ENDE_ALARM_BIT_NR))		fprintf(_hLogFile,"           : KVO ready\n");
				if (t&(1<<CDS_FERNREGEL_ALARM_BIT_NR))		fprintf(_hLogFile,"           : Remote control interrupted by operator\n");
				if (t&(1<<CDS_KVO_VORALARM_BIT_NR))			fprintf(_hLogFile,"           : KVO almost ready\n");
				if (t&(1<<CDS_MINDRUCK_ALARM_BIT_NR))		fprintf(_hLogFile,"           : Pressure-low alarm\n");
				if (t&(1<<CDS_PCA_TASTER_ALARM_BIT_NR))		fprintf(_hLogFile,"           : PCA button not connected\n");
				if (t&(1<<CDS_PCA_HLIMVOL_ALARM_BIT_NR))	fprintf(_hLogFile,"           : PCA limit reached\n");

				t = status.zustand;
				fprintf(_hLogFile,"      state: 0x%08lx\n", t);
				if (t&(1<<CDS_CC_MODE_DOKU_BIT_NR))			fprintf(_hLogFile,"           : Documentation Mode\n");
				if (t&(1<<CDS_CC_FERNSTEUER_BIT_NR))		fprintf(_hLogFile,"           : Remote&Manual Mode\n");
				if (t&(1<<CDS_CC_FERNREGEL_BIT_NR))			fprintf(_hLogFile,"           : Remote-only Mode\n");
				if (t&(1<<CDS_NETZBETRIEB_STATUS_BIT_NR))	fprintf(_hLogFile,"           : Mains powered\n");
				if (t&(1<<CDS_STANDBY_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Standby\n");
				if (t&(1<<CDS_RUN_READY_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Ready to start\n");
				if (t&(1<<CDS_RUN_STATUS_BIT_NR))			fprintf(_hLogFile,"           : Pump active\n");
				if (t&(1<<CDS_BOLUS_READY_STATUS_BIT_NR))	fprintf(_hLogFile,"           : Bolus ready to start\n");
				if (t&(1<<CDS_BOLUS_MAN_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Bolus running\n");
				if (t&(1<<CDS_BOLUS_VOL_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Bolus (volume) running\n");
				if (t&(1<<CDS_BEDIENSTATUS_AKTIV_BIT_NR))	fprintf(_hLogFile,"           : Manual operation active\n");
				if (t&(1<<CDS_BOLUS_INT_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Bolus (interval) running\n");
				if (t&(1<<CDS_BOLUS_INT_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Pump active with Keep Vene Open option\n");
				if (t&(1<<CDS_ALARM_OFF_AKTIV_BIT_NR))		fprintf(_hLogFile,"           : Alarm surpressed for 2 minutes. Timer active.\n");
				if (t&(1<<CDS_AKUST_ALARM_ON_BIT_NR))		fprintf(_hLogFile,"           : Acoustic alarm on\n");
				if (t&(1<<CDS_DATALOCK_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Manual opertion blocked\n");
				if (t&(1<<CDS_TROPFUEB_OFF_BIT_NR))			fprintf(_hLogFile,"           : Drop monitoring active\n");
				if (t&(1<<CDS_HISTORY_BIT_NR))				fprintf(_hLogFile,"           : History buffer is filled\n");
				if (t&(1<<CDS_VORLAUF_STATUS_BIT_NR))		fprintf(_hLogFile,"           : Schnellen Vorlauf zum Greifen der Spritze\n");
				if (t&(1<<CDS_ANTR_SPRITZE_RUECK_BIT_NR))	fprintf(_hLogFile,"           : Retracting head\n");
				if (t&(1<<CDS_STATUS_PROTOKOL_BIT_NR))		fprintf(_hLogFile,"           : Pump tachograph active\n");
				if (t&(1<<CDS_BOLUS_INT_AKTIV_BIT_NR))		fprintf(_hLogFile,"           : Interval bolus active\n");
				if (t&(1<<CDS_DOSIS_AKTIV_BIT_NR))			fprintf(_hLogFile,"           : Dosis calculation active\n");
				if (t&(1<<CDS_RELOAD_AKTIV_BIT_NR))			fprintf(_hLogFile,"           : Reload active\n");
				if (t&(1<<CDS_PIGGYBACK_AKTIV_BIT_NR))		fprintf(_hLogFile,"           : Piggyback active\n");
				if (t&(1<<CDS_PCA_AKTIV_BIT_NR))			fprintf(_hLogFile,"           : PCA active (Patient Controled Analgesia)\n");
				if (t&(1<<CDS_PROFIL_AKTIV_BIT_NR))			fprintf(_hLogFile,"           : Profile active\n");
				if (t&(1<<CDS_RAMPTAPER_AKTIV_BIT_NR))		fprintf(_hLogFile,"           : Ramp-Taper active\n");
				if (t&(1<<CDS_DELAY_AKTIV_BIT_NR))			fprintf(_hLogFile,"           : Delay active\n");
				fprintf(_hLogFile,"actual rate: %lu [10microl/h]\n", (DWORD)status.rate);
				fprintf(_hLogFile," remainvtbd: %lu [0.1ml] (remaining volume to be delivered)\n", (DWORD)status.remainvtbd);
				fprintf(_hLogFile," remaintime: %lu [min]   (remaining time for remainvtbd)\n", (DWORD)status.remaintime);
			}
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetTargetValue(
			HANDLE h, 
			DWORD* rate_10ul_h, 
			DWORD* vtbd_100ul, 
			DWORD* time_min, 
			DWORD* bolrate_10ul_h, 
			DWORD* bolvol_100ul, 
			DWORD* pressurelimitmax, 
			DWORD* stdby_min, 
			char* szMedication
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_SOLLWERT); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET TARGET failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_SOLLWERT& t = replyPacket._data.sollwert;
			assign(rate_10ul_h, t.rate);				/* Rate [0.01ml/h] */
			assign(vtbd_100ul, t.vtbd);					/* zu infudierendes Volumen [0.1ml] */
			assign(time_min, t.time);					/* Behandlungsdauer [min] */
			assign(bolrate_10ul_h, t.bolrate);			/* Bolusrate [0.01ml/h */
			assign(bolvol_100ul, t.bolvol);				/* Bolusvolumen [0.1ml] */
			assign(pressurelimitmax, t.drucklimitmax);	/* Druckstufe 1..9 */
			assign(stdby_min, t.stdby);					/* standby-Zeit [min] */
			PrintableString(t.medikament,sizeof(t.medikament),szMedication,sizeof(t.medikament)+1); // copy to szMedication incl zero
			//if (szMedication)
			//{
			//	int i;
			//	for (i=0; i<CDS_MEDNAME_LEN; i++)
			//		szMedication[i] = t.medikament[i]!=0xFF ? t.medikament[i] : 0; /* Medikamentenname ohne 0 ! */
			//	szMedication[i] = '\0';
			//}
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET TARGET : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				const TDS_DATA_SOLLWERT& t = replyPacket._data.sollwert;
				char buf[sizeof(t.medikament)+1]; // buffer for printable versions incl zero
				fprintf(_hLogFile,"       rate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.rate));
				fprintf(_hLogFile,"       vtbd: %s [0.1ml]\n", PrintValue(buf,sizeof(buf),t.vtbd));
				fprintf(_hLogFile,"       time: %s [min]\n", PrintValue(buf,sizeof(buf),t.time));
				fprintf(_hLogFile,"    bolrate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.bolrate));
				fprintf(_hLogFile,"     bolvol: %s [0.1ml]\n", PrintValue(buf,sizeof(buf),t.bolvol));
				fprintf(_hLogFile,"presslimmax: %s [1..9]\n", PrintValue(buf,sizeof(buf),t.drucklimitmax));
				fprintf(_hLogFile,"  stdby_min: %s [min]\n", PrintValue(buf,sizeof(buf),t.stdby));
				fprintf(_hLogFile," medication: '%s'\n", PrintableString(t.medikament,sizeof(t.medikament),buf,sizeof(buf),'=')); 
			}
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetTargetValue(
			HANDLE h, 
			DWORD rate_10ul_h, 
			DWORD vtbd_100ul, 
			DWORD time_min, 
			DWORD bolrate_10ul_h, 
			DWORD bolvol_100ul, 
			DWORD pressurelimitmax, 
			DWORD stdby_min, 
			const char* szMedication
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_WSEG_SOLLWERT); 
		TdsReplyPacket   replyPacket;

		ATLASSERT(pressurelimitmax==0xFFFFFFFF || (pressurelimitmax>=1 && pressurelimitmax<=9));

		TDS_DATA_SOLLWERT& t = sendPacket._data.sollwert;
		assign(t.rate, rate_10ul_h);				/* Rate [0.01ml/h] */
		assign(t.vtbd, vtbd_100ul);					/* zu infudierendes Volumen [0.1ml] */
		assign(t.time, time_min);					/* Behandlungsdauer [min] */
		assign(t.bolrate, bolrate_10ul_h);			/* Bolusrate [0.01ml/h */
		assign(t.bolvol, bolvol_100ul);				/* Bolusvolumen [0.1ml] */
		assign(t.drucklimitmax, pressurelimitmax);	/* Druckstufe 1..9 */
		assign(t.stdby, stdby_min);					/* standby-Zeit [min] */
		const int n = szMedication ? strlen(szMedication) : 0;
		const char c = szMedication ? ' ' : 0xFF; /* null pointer == keep existing string ==> fill with 0xFF */
		for (int i=0; i<CDS_MEDNAME_LEN; i++)
			sendPacket._data.sollwert.medikament[i] = i<n ? szMedication[i] : c; /* Medikamentenname ohne 0 ! */

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: SET TARGET failed with code %i\n", (int)ret);

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			char buf[sizeof(t.medikament)+1]; // buffer for printable version incl zero
			fprintf(_hLogFile,"SET TARGET : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			fprintf(_hLogFile,"       rate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.rate));
			fprintf(_hLogFile,"       vtbd: %s [0.1ml]\n", PrintValue(buf,sizeof(buf),t.vtbd));
			fprintf(_hLogFile,"       time: %s [min]\n", PrintValue(buf,sizeof(buf),t.time));
			fprintf(_hLogFile,"    bolrate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.bolrate));
			fprintf(_hLogFile,"     bolvol: %s [0.1ml]\n", PrintValue(buf,sizeof(buf),t.bolvol));
			fprintf(_hLogFile,"presslimmax: %s [1..9]\n", PrintValue(buf,sizeof(buf),t.drucklimitmax));
			fprintf(_hLogFile,"  stdby_min: %s [min]\n", PrintValue(buf,sizeof(buf),t.stdby));
			fprintf(_hLogFile," medication: '%s'\n", PrintableString(t.medikament,sizeof(t.medikament),buf,sizeof(buf),'=')); 
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetCurrentValue(
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
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_ISTWERT); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET VALUE failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_ISTWERT& t = replyPacket._data.istwert;
			assign(pressure_pct, t.druckwert);						/* Druckwert 0-100 Prozent der Druckstufe */
			assign(infusedvol_100ul, t.infusedvol);					/* Infudiertes Volumen [0.1ml] */
			assign(infusedtime_min, t.infusedtime);					/* Behandlungszeit [min] */
			assign(bolusintervaltimeleft_min, t.bolintvrestzeit);	/* Restzeit der Bolusintervallzeit [min] */
			assign(batterytimeleft_min, t.akkurest);				/* Akku-Restkapazitaet [min] */
			assign(standbytimeleft_min, t.standbyrest);				/* restliche standbyzeit [min] */
			assign(syringevol_ml, t.syringevol);					/* erkanntes Spritzenvolumen [ml] */
			assign(bolusintervaltime_min, t.bolintvzeit);			/* Bolusintervallzeit [min] */
			assign(bolusintervalvolume_100ul, t.bolintvvol);		/* Bolusintervallvolumen [0.1ml] */
			assign(actualbolusvolume_100ul, t.bolvolakt);			/* aktuelles Bolusvolumen [0.1ml] */
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			const TDS_DATA_ISTWERT& t = replyPacket._data.istwert;
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET VALUE : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				fprintf(_hLogFile,"   pressure: %lu %\n", (DWORD)t.druckwert);
				fprintf(_hLogFile," infusedvol: %lu [0.1ml]\n", (DWORD)t.infusedvol);
				fprintf(_hLogFile,"infusedtime: %lu [min]\n", (DWORD)t.infusedtime);
				fprintf(_hLogFile," bolusinttm: %lu [min] left\n", (DWORD)t.bolintvrestzeit);
				fprintf(_hLogFile," batttmleft: %lu [min] left\n", (DWORD)t.akkurest);
				fprintf(_hLogFile,"standbyleft: %lu [min] left\n", (DWORD)t.standbyrest);
				fprintf(_hLogFile," syringevol: %lu [ml]\n", (DWORD)t.syringevol);
				fprintf(_hLogFile," bolinttime: %lu [min]\n", (DWORD)t.bolintvzeit);
				fprintf(_hLogFile,"  bolintvol: %lu [0.1ml]\n", (DWORD)t.bolintvvol);
				fprintf(_hLogFile,"  curbolvol: %lu [0.1ml]\n", (DWORD)t.bolvolakt);
			}
		}
		return ret;
	}

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
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_LIMITS); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET LIMITS failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_LIMITS& t = replyPacket._data.limits;
			assign(bolmax_10ul_h, t.bolmax);			/* max.Bolusrate [0.01ml/h] */
			assign(personal, t.personal);				/* Pump reflects the type of personnel call */
			assign(resdisp, t.resdisp);					/* Number of decimal places for the rate in up ml/h */
			assign(ratemax_10ul_h, t.ratemax);			/* Current max. delivery rate, depending on operating mode. [0.01ml/h] */
			assign(ratemin_10ul_h, t.ratemin);			/* Current min. delivery rate, depending on operating mode. [0.01ml/h] */
			assign(syringeprealarm_min, t.spritzevoral);	/* Syringe pre-alarm [min] */
			assign(specialfn, t.sonderfkt);				/* Special functions available for Dianet Star */
			assign(pressmax, t.druckmax);				/* Max. adjustable pressure level [1..9] */
			assign(vtbdmax_100ul, t.vtbdmax);			/* Max. adjustable volume for VTBD, also applies for RELOAD_VTBD [0.1ml] */
			assign(bolvolmax_100ul, t.bolvolmax);		/* Max. adjustable bolus volume [0.1ml] */
			assign(bolratemin_10ul_h, t.bolmin);		/* Min. bolus rate [0.01ml/h] */
			assign(bolvolmin_100ul, t.bolvolmin);		/* Min. adjustable bolus volume [0.1ml] */
			assign(resbol, t.resbol);					/* Number of bolus rate decimal places */
			assign(caratemax_10ul_h, t.caratemax);		/* Max. delivery rate in CD/CA mode [0.01ml/h] */
			assign(caratemin_10ul_h, t.caratemin);		/* Min. delivery rate in CD/CA mode [0.01ml/h] */
			assign(ccratemax_10ul_h, t.ccratemax);		/* Max. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */
			assign(ccratemin_10ul_h, t.ccratemin);		/* Min. delivery rate in CC mode, also applies for RELOAD_RATE [0.01ml/h] */
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			const TDS_DATA_LIMITS& t = replyPacket._data.limits;
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET LIMITS : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				fprintf(_hLogFile,"     bolmax: %lu [0.01ml/h]\n", (DWORD)t.bolmax);
				fprintf(_hLogFile,"   personal: 0x%02lx\n", (DWORD)t.personal);
				if (t.personal&(1<<CDS_STATISCH_BIT_NR))		fprintf(_hLogFile,"             0x%02lx = static\n", (1<<CDS_STATISCH_BIT_NR));
				if (t.personal&(1<<CDS_DYNAMISCH_BIT_NR))		fprintf(_hLogFile,"             0x%02lx = dynamic\n", (1<<CDS_DYNAMISCH_BIT_NR));
				if (t.personal&(1<<CDS_AUSALARM_BIT_NR))		fprintf(_hLogFile,"             0x%02lx = off-alarm\n", (1<<CDS_AUSALARM_BIT_NR));
				if (t.personal&(1<<CDS_TESTIMPULS_BIT_NR))		fprintf(_hLogFile,"             0x%02lx = test pulse\n", (1<<CDS_TESTIMPULS_BIT_NR));
				if (t.personal&(1<<CDS_VORALARM_BIT_NR))		fprintf(_hLogFile,"             0x%02lx = pre-alarm\n", (1<<CDS_VORALARM_BIT_NR));
				fprintf(_hLogFile,"    resdisp: %lu\n", (DWORD)t.resdisp);
				fprintf(_hLogFile,"    ratemax: %lu [0.01ml/h]\n", (DWORD)t.ratemax);
				fprintf(_hLogFile,"    ratemin: %lu [0.01ml/h]\n", (DWORD)t.ratemin);
				fprintf(_hLogFile,"syrprealarm: %lu [min]\n", (DWORD)t.spritzevoral);
				fprintf(_hLogFile,"  specialfn: 0x%02lx\n", (DWORD)t.sonderfkt);
				if (t.sonderfkt&(1<<CDS_LIM_SF_BOLUS_BIT_NR))	fprintf(_hLogFile,"             0x%02lx = Bolus function available\n", (1<<CDS_LIM_SF_BOLUS_BIT_NR));
				if (t.sonderfkt&(1<<CDS_LIM_SF_STANDBY_BIT_NR))	fprintf(_hLogFile,"             0x%02lx = Standby function available\n", (1<<CDS_LIM_SF_STANDBY_BIT_NR));
				if (t.sonderfkt&(1<<CDS_LIM_SF_DRUCK_BIT_NR))	fprintf(_hLogFile,"             0x%02lx = Adjustment of pressure levels is possible\n", (1<<CDS_LIM_SF_DRUCK_BIT_NR));
				if (t.sonderfkt&(1<<CDS_LIM_SF_DATALOCK_BIT_NR))fprintf(_hLogFile,"             0x%02lx = Data Lock function available\n", (1<<CDS_LIM_SF_DATALOCK_BIT_NR));
				if (t.sonderfkt&(1<<CDS_LIM_SF_TROPF_BIT_NR))	fprintf(_hLogFile,"             0x%02lx = Drip control is available for hose pumps\n", (1<<CDS_LIM_SF_TROPF_BIT_NR));
				if (t.sonderfkt&(1<<CDS_LIM_SF_DOSIS_BIT_NR))	fprintf(_hLogFile,"             0x%02lx = Dose calculation is available\n", (1<<CDS_LIM_SF_DOSIS_BIT_NR));
				fprintf(_hLogFile,"   pressmax: %lu\n", pressmax);
				fprintf(_hLogFile,"    vtbdmax: %lu [0.1ml]\n", (DWORD)t.vtbdmax);
				fprintf(_hLogFile,"  bolvolmax: %lu [0.1ml]\n", (DWORD)t.bolvolmax);
				fprintf(_hLogFile," bolratemin: %lu [0.01ml/h]\n", (DWORD)t.bolmin);
				fprintf(_hLogFile,"  bolvolmin: %lu [0.1ml]\n", (DWORD)t.bolvolmin);
				fprintf(_hLogFile,"     resbol: %lu\n", (DWORD)t.resbol);
				fprintf(_hLogFile,"  CAratemax: %lu [0.01ml/h]\n", (DWORD)t.caratemax);
				fprintf(_hLogFile,"  CAratemin: %lu [0.01ml/h]\n", (DWORD)t.caratemin);
				fprintf(_hLogFile,"  CCratemax: %lu [0.01ml/h]\n", (DWORD)t.ccratemax);
				fprintf(_hLogFile,"  CCratemin: %lu [0.01ml/h]\n", (DWORD)t.ccratemin);
			}
		}
		return ret;
	}

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
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_DOSIS); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET LIMITS failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_DOSIS& t = replyPacket._data.dosis;
			assign(weight_10gr, t.kg);				/* body weight [10g] */
			assign(concentration_milli, t.konz);	/* concentration [0.001 <units> with IE = 1 ] */
			assign(concentrationunits, t.einhkonz);	/* units of concentration */
			assign(doserate_milli, t.rate);			/* dose rate [0.001 <units> with IE = 1 ] */
			assign(doserate_units, t.einhrate);		/* dose rate units */
			assign(gebvol_10ul, t.gebvol);			/* volume with which the active substance was activated [0.01 ml] */
			assign(drug_milli, t.drug);				/* quantity of active substance within the package volume [0.001 <units> with IE = 1 ] */
			assign(drug_units, t.einhdrug);			/* units of the active substance */
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			const TDS_DATA_DOSIS& t = replyPacket._data.dosis;
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET DOSIS: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				char buf[32];
				if (IsValid(t.kg))	
					fprintf(_hLogFile," bodyweight: %lu [10gr]\n", (DWORD)t.kg);
				if (IsValid(t.konz))
				{
					fprintf(_hLogFile,"concentratn: %lu [milli units]\n", (DWORD)t.konz);
					fprintf(_hLogFile,"      units: %s\n", FlagToUnits(t.einhkonz, buf, sizeof(buf)));
				}
				if (IsValid(t.rate))
				{
					fprintf(_hLogFile,"  dose rate: %lu [milli units]\n", (DWORD)t.rate);
					fprintf(_hLogFile,"      units: %s\n", FlagToUnits(t.einhrate, buf, sizeof(buf)));
				}
				if (IsValid(t.drug))
				{
					fprintf(_hLogFile,"       drug: %lu [milli units]\n", (DWORD)t.drug);
					fprintf(_hLogFile,"      units: %s\n", FlagToUnits(t.einhdrug, buf, sizeof(buf)));
				}
				if (IsValid(t.gebvol))
					fprintf(_hLogFile,"     gebvol: %lu [0.01ml]\n", (DWORD)t.gebvol);
			}
		}

		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetReload(
			HANDLE h, 
			DWORD* active,		/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
									The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
									The AKTIV state is reflected in the RELOAD_AKTIV status.
								 */
			DWORD* rate_10ul_h,	/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
			DWORD* vtbd_100ul	/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode (MUST BE CC MODE!!!)

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_RELOAD); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET RELOAD failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_RELOAD& t = replyPacket._data.reload;
			assign(active, t.aktiv);
			assign(rate_10ul_h, t.rate);	
			assign(vtbd_100ul, t.vtbd);
		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			const TDS_DATA_RELOAD& t = replyPacket._data.reload;
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET RELOAD: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				fprintf(_hLogFile,"     active: %lu [10gr]\n", (DWORD)t.aktiv);
				fprintf(_hLogFile,"       rate: %lu [0.01ml/h]\n", (DWORD)t.rate);
				fprintf(_hLogFile,"       vtbd: %lu [0.1ml]\n", (DWORD)t.vtbd);
			}
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetReload(
			HANDLE h, 
			DWORD active,		/* 1 = RATE and VTBD are valid and are transferred to the target values after VTBD end.
									The parameter set {1;0;0} signifies delivery with rate 0 ml/h without volume pre-selection.
									The AKTIV state is reflected in the RELOAD_AKTIV status.
								 */
			DWORD rate_10ul_h,	/* Rate [0.01ml/h] Rate_1 for target values, if VTBD end occurs */
			DWORD vtbd_100ul	/* vtbd [0.1ml] VTBD for target values, if VTBD end occurs. */
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_WSEG_RELOAD); 
		TdsReplyPacket   replyPacket;

		TDS_DATA_RELOAD& t = sendPacket._data.reload;
		assign(t.aktiv, active);	
		assign(t.vtbd, vtbd_100ul);
		assign(t.rate, rate_10ul_h);

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: SET RELOAD failed with code %i\n", (int)ret);

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			char buf[32];
			fprintf(_hLogFile,"SET RELOAD : %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			printf("     active: %s [on/off]\n", PrintValue(buf,sizeof(buf),t.aktiv));
			printf("       rate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.rate));
			printf("       vtbd: %s [0.1ml]\n", PrintValue(buf,sizeof(buf),t.vtbd));
		}
		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_GetProposal(
			HANDLE h, 
			DWORD* rate_10ul_h,			/* proposed rate [0.01ml/h] */
			char* szMedication			/* proposed medication incl. terminating zero [len>CDS_MEDNAME_LEN] ! */
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode (MUST BE CC MODE!!!)

		TdsCommandPacket sendPacket(dev._address, CDS_RSEG_PROPOSAL); 
		TdsReplyPacket   replyPacket;

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: GET PROPOSAL failed with code %i\n", (int)ret);
		else
		{
			const TDS_DATA_PROPOSAL& t = replyPacket._data.proposal;
			if (rate_10ul_h)	*rate_10ul_h = t.rate;				
			fprintf(_hLogFile," medication: '%s'\n", PrintableString(t.medikament,sizeof(t.medikament),szMedication,sizeof(t.medikament)+1,'=')); 
			//if (szMedication)
			//{
			//	int i;
			//	for (i=0; i<CDS_MEDNAME_LEN; i++)
			//		szMedication[i] = t.medikament[i]!=0xFF ? t.medikament[i] : 0; /* Medikamentenname ohne 0 ! */
			//	szMedication[i] = '\0';
			//}

		}

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			const TDS_DATA_PROPOSAL& t = replyPacket._data.proposal;
			PrintTimeStamp(dev._szPort);
			fprintf(_hLogFile,"GET PROPOSAL: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			if (ret==0)
			{
				fprintf(_hLogFile,"       rate: %lu [0.01ml/h]\n", (DWORD)t.rate);
				char buf[sizeof(t.medikament)+1]; // buffer for printable version incl zero
				fprintf(_hLogFile," medication: '%s'\n", PrintableString(t.medikament,sizeof(t.medikament),buf,sizeof(buf),'=')); 
//				fprintf(_hLogFile," mediaction: [%s]\n", t.medikament); // TODO: remove unprintable stuff
			}
		}

		return ret;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_SetProposal(
			HANDLE h, 
			DWORD rate_10ul_h,			/* proposed rate [0.01ml/h] */
			const char* szMedication	/* proposed medication incl. terminating zero [len<=CDS_MEDNAME_LEN] ! */
		)
	{
		DeviceMap::iterator i = _deviceMap.find(h);
		if (i==_deviceMap.end())
			return CDS_ERR_NOTCONNECTED;

		DeviceInfo& dev = (*i).second;

		// Make sure to call DIANETINTERFACE_SetCcMode before using this function.
		//
		CHECK_MODE_REMOTE	// make sure device was switched to remote control mode

		TdsCommandPacket sendPacket(dev._address, CDS_WSEG_PROPOSAL); 
		TdsReplyPacket   replyPacket;

		TDS_DATA_PROPOSAL& t = sendPacket._data.proposal;
		assign(t.rate, rate_10ul_h);	
		int n = szMedication ? strlen(szMedication) : 0;
		for (int i=0; i<CDS_MEDNAME_LEN; i++)
			t.medikament[i] = i<n ? szMedication[i] : 0xFF; /* Medikamentenname ohne 0 ! */

		UINT8 ret = dev.Cmd(&sendPacket, &replyPacket);
		if (ret!=CDS_ERR_NC)
			ATLTRACE2(myTrace, 2, "ERROR: SET PROPOSAL failed with code %i\n", (int)ret);

		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO || ret!=CDS_ERR_NC))
		{
			PrintTimeStamp(dev._szPort);
			char buf[sizeof(t.medikament)+1]; // buffer for printable version incl zero
			fprintf(_hLogFile, "SET PROPOSAL: %lu [%s]\n",ret,DIANETINTERFACE_GetErrorString(ret));
			fprintf(_hLogFile, "       rate: %s [0.01ml/h]\n", PrintValue(buf,sizeof(buf),t.rate));
			fprintf(_hLogFile, " medication: '%s'\n", PrintableString(t.medikament,sizeof(t.medikament),buf,sizeof(buf),'=')); 
//			fprintf(_hLogFile, " mediaction: [%s]\n", t.medikament); // TODO: remove unprintable stuff
		}

		return ret;
	}

	DIANETINTERFACE_DLLX const char* DLLENTRY DIANETINTERFACE_GetErrorString(DWORD nr)
	{
		// NB must match CDS_ERR_* definitions in dstarcom.h
		static const char* szErrorTable[] = {
		  "OK",							// 0
		  "CRC checksum failure",		// 1
		  "Framing error",				// 2
		  "Illegal segment number",		// 3
		  "User mode failure",			// 4
		  "Ident failure",				// 5
		  "Data range failure",			// 6
		  "Wrong segment length",		// 7
		  "Illegal Adress",				// 8
		  "Illegal Mode",				// 9
		  "Errno #10",					// a
		  "Clock locked",				// b
		  "Action failure",				// c
		  "Context failure",			// d
		  "Timeout",					// f
		  "RS232 port not initialized",	// 10
		  "RS232 failure",				// 20
		  "Cannot open file",			// 30
		  "Cannot close file"			// 40
		};
		return nr<sizeof(szErrorTable)/sizeof(szErrorTable[0]) ? szErrorTable[nr] : "Unknown error code";
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_OpenLogFile(
			const char* szFilename,		/* file to open, or null/empty to use standard output */
			BOOL bAppend,				/* append to existing file if true */
			DWORD iLogLevel,			/* 0=errors only; 1=warnings; 2=all */
			DWORD externalClock_ms		/* current value of user defined clock in [ms] */
		)
	{
		errno_t err = 0;
		if (_hLogFile!=0)
			DIANETINTERFACE_CloseLogFile();
		if (szFilename==NULL || szFilename[0]=='\0')
			_hLogFile = stdout;
		else
			err = fopen_s(&_hLogFile,szFilename,(bAppend?"at":"wt"));
		if (err)
			ATLTRACE2(myTrace, 2, "ERROR: cannot open logfile %s; errno=%i\n", szFilename, (int)err);
		else
		{
			_iLogLevel = iLogLevel;
			DIANETINTERFACE_SetLogTimer(externalClock_ms);
		}
		return err==0 ? CDS_ERR_NC : CDS_ERR_CANNOT_OPEN;
	}

	DIANETINTERFACE_DLLX DWORD DLLENTRY DIANETINTERFACE_CloseLogFile()
	{
		if (_hLogFile==stdout || _hLogFile==NULL)
			return CDS_ERR_NC;
		else
			return fclose(_hLogFile)==0 ? CDS_ERR_NC: CDS_ERR_CANNOT_CLOSE;
	}

	DIANETINTERFACE_DLLX void DLLENTRY DIANETINTERFACE_LogMessage(
			const char* szMessage,		/* add message to logfile */
			DWORD iLogLevel				/* 0=errors only; 1=warnings; 2=all */
		)
	{
		if (_hLogFile!=NULL && (_iLogLevel>=iLogLevel))
		{
			char buf[32];
			sprintf_s(buf,sizeof(buf),"MSG%lu",iLogLevel);
			PrintTimeStamp(buf);
			fprintf(_hLogFile, "    message: %s\n", szMessage); 
		}
	}

	DIANETINTERFACE_DLLX void DLLENTRY DIANETINTERFACE_SetLogTimer(
			DWORD externalClock_ms		/* current value of user defined clock in [ms] */
		)
	{
		// first print a message using current clock offset
		if (_hLogFile!=NULL && (_iLogLevel>=CDS_LOGLEVEL_INFO))
		{
			PrintTimeStamp("TIMER RESET");
			fprintf(_hLogFile, "externalclk: %lu\n", externalClock_ms); 
		}
		#if _WIN32_WINNT >= 0x0600
			_i64LogTickOffset = (LONGLONG)(externalClock_ms) - GetTickCount64(); // start logging at 0, optionally shifted to user's clock
		#else
			_i64LogTickOffset = (LONGLONG)(externalClock_ms) - GetTickCount(); // start logging at 0, optionally shifted to user's clock
		#endif
	}

} // extern "C"