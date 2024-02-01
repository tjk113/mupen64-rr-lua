#include "tracelog.h"

#include "disasm.h"
#include "LuaConsole.h"
#include "r4300.h"

// TODO: Rename to less generic name, e.g.: vr_op
extern unsigned long op;

namespace tracelog
{
	bool enableTraceLog = false;
	bool traceLogMode;
	HANDLE TraceLogFile;

	//�Ƃ肠����lua�ɓ���Ƃ�
	char traceLoggingBuf[0x10000];
	char* traceLoggingPointer = traceLoggingBuf;

	bool active()
	{
		return enableTraceLog;
	}


	void TraceLoggingBufFlush()
	{
		DWORD writeen;
		WriteFile(TraceLogFile,
				  traceLoggingBuf, traceLoggingPointer - traceLoggingBuf, &writeen,
				  NULL);
		traceLoggingPointer = traceLoggingBuf;
	}

	void TraceLoggingWriteBuf()
	{
		const char* const buflength = traceLoggingBuf + sizeof(traceLoggingBuf) -
			512;
		if (traceLoggingPointer >= buflength)
		{
			TraceLoggingBufFlush();
		}
	}


void TraceLoggingBin(r4300word pc, r4300word w)
{
	char*& p = traceLoggingPointer;
	INSTDECODE decode;
	//little endian
#define HEX8(n) 	*(r4300word*)p = n; p += 4

	DecodeInstruction(w, &decode);
	HEX8(pc);
	HEX8(w);
	INSTOPERAND& o = decode.operand;
	//n�͌ォ��f�R�[�h����΂킩��
#define REGCPU(n) \
	HEX8(reg[n])
#define REGCPU2(n,m) \
		REGCPU(n);\
		REGCPU(m);
	//10�i��
#define REGFPU(n) \
	HEX8(*(r4300word*)reg_cop1_simple[n])
#define REGFPU2(n,m) REGFPU(n);REGFPU(m)
#define NONE *(r4300word*)p=0;p+=4
#define NONE2 NONE;NONE

	switch (decode.format)
	{
	case INSTF_NONE:
		NONE2;
		break;
	case INSTF_J:
	case INSTF_0BRANCH:
		NONE2;
		break;
	case INSTF_LUI:
		NONE2;
		break;
	case INSTF_1BRANCH:
	case INSTF_JR:
	case INSTF_ISIGN:
	case INSTF_IUNSIGN:
		REGCPU(o.i.rs);
		NONE;
		break;
	case INSTF_2BRANCH:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_ADDRW:
		HEX8(reg[o.i.rs] + (r4300halfsigned)o.i.immediate);
		REGCPU(o.i.rt);
		break;
	case INSTF_ADDRR:
		HEX8(reg[o.i.rs] + (r4300halfsigned)o.i.immediate);
		NONE;
		break;
	case INSTF_LFW:
		HEX8(reg[o.lf.base] + (r4300halfsigned)o.lf.offset);
		REGFPU(o.lf.ft);
		break;
	case INSTF_LFR:
		HEX8(reg[o.lf.base] + (r4300halfsigned)o.lf.offset);
		NONE;
		break;
	case INSTF_R1:
		REGCPU(o.r.rd);
		NONE;
		break;
	case INSTF_R2:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_R3:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_MTC0:
	case INSTF_MTC1:
	case INSTF_SA:
		REGCPU(o.r.rt);
		NONE;
		break;
	case INSTF_R2F:
		REGFPU(o.cf.fs);
		NONE;
		break;
	case INSTF_R3F:
	case INSTF_C:
		REGFPU2(o.cf.fs, o.cf.ft);
		break;
	case INSTF_MFC0:
		NONE2;
		break;
	case INSTF_MFC1:
		REGFPU(((FPUREG)o.r.rs));
		NONE;
		break;
	}
	TraceLoggingWriteBuf();
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
}

void LuaTraceLoggingPure()
{
	if (!traceLogMode)
	{
		TraceLogging(interp_addr, op);
	} else
	{
		TraceLoggingBin(interp_addr, op);
	}
}

void LuaTraceLoggingInterpOps()
{
	if (enableTraceLog)
	{
		if (!traceLogMode)
		{
			TraceLogging(PC->addr, PC->src);
		} else
		{
			TraceLoggingBin(PC->addr, PC->src);
		}
	}
	PC->s_ops();
}


	// TODO: Remove this, do the path picking at view callsite
void LuaTraceLogState()
{
	TraceLogStart("trace.log");
}





	void TraceLogStart(const char* name, BOOL append)
	{
		if (TraceLogFile = CreateFile(name, GENERIC_WRITE,
									  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
									  append ? OPEN_ALWAYS : CREATE_ALWAYS,
									  FILE_ATTRIBUTE_NORMAL |
									  FILE_FLAG_SEQUENTIAL_SCAN, NULL))
		{
			if (append)
			{
				SetFilePointer(TraceLogFile, 0, NULL, FILE_END);
			}
			enableTraceLog = true;
			if (interpcore == 0)
			{
				RecompileNextAll();
				RecompileNow(PC->addr);
			}
			// TODO: Update ui in callsite
		}
	}

	void TraceLogStop()
	{
		enableTraceLog = false;
		CloseHandle(TraceLogFile);
		TraceLogFile = NULL;
		// TODO: Update ui in callsite
		TraceLoggingBufFlush();
	}

void TraceLogging(r4300word pc, r4300word w)
{
	char*& p = traceLoggingPointer;
	INSTDECODE decode;
	const char* const x = "0123456789abcdef";
#define HEX8(n) 	p[0] = x[(r4300word)(n)>>28&0xF];\
	p[1] = x[(r4300word)(n)>>24&0xF];\
	p[2] = x[(r4300word)(n)>>20&0xF];\
	p[3] = x[(r4300word)(n)>>16&0xF];\
	p[4] = x[(r4300word)(n)>>12&0xF];\
	p[5] = x[(r4300word)(n)>>8&0xF];\
	p[6] = x[(r4300word)(n)>>4&0xF];\
	p[7] = x[(r4300word)(n)&0xF];\
	p+=8;

	DecodeInstruction(w, &decode);
	HEX8(pc);
	*(p++) = ':';
	*(p++) = ' ';
	HEX8(w);
	*(p++) = ' ';
	const char* ps = p;
	if (w == 0x00000000)
	{
		*(p++) = 'n';
		*(p++) = 'o';
		*(p++) = 'p';
	} else
	{
		for (const char* q = GetOpecodeString(&decode); *q; q++)
		{
			*(p++) = *q;
		}
		*(p++) = ' ';
		p = GetOperandString(p, &decode, pc);
	}
	for (int i = p - ps + 3; i < 24; i += 4)
	{
		*(p++) = '\t';
	}
	*(p++) = ';';
	INSTOPERAND& o = decode.operand;
#define REGCPU(n) if((n)!=0){\
			for(const char *l = CPURegisterName[n]; *l; l++){\
				*(p++) = *l;\
			}\
			*(p++) = '=';\
			HEX8(reg[n]);\
	}
#define REGCPU2(n,m) \
		REGCPU(n);\
		if((n)!=(m)&&(m)!=0){C;REGCPU(m);}
	//10�i��
#define REGFPU(n) *(p++)='f';\
			*(p++)=x[(n)/10];\
			*(p++)=x[(n)%10];\
			*(p++) = '=';\
			p+=sprintf(p,"%f",*reg_cop1_simple[n])
#define REGFPU2(n,m) REGFPU(n);\
		if((n)!=(m)){C;REGFPU(m);}
#define C *(p++) = ','

	if (delay_slot)
	{
		*(p++) = '#';
	}
	switch (decode.format)
	{
	case INSTF_NONE:
		break;
	case INSTF_J:
	case INSTF_0BRANCH:
		break;
	case INSTF_LUI:
		break;
	case INSTF_1BRANCH:
	case INSTF_JR:
	case INSTF_ISIGN:
	case INSTF_IUNSIGN:
		REGCPU(o.i.rs);
		break;
	case INSTF_2BRANCH:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_ADDRW:
		REGCPU(o.i.rt);
		if (o.i.rt != 0) { C; }
	case INSTF_ADDRR:
		*(p++) = '@';
		*(p++) = '=';
		HEX8(reg[o.i.rs] + (r4300halfsigned)o.i.immediate);
		break;
	case INSTF_LFW:
		REGFPU(o.lf.ft);
		C;
	case INSTF_LFR:
		*(p++) = '@';
		*(p++) = '=';
		HEX8(reg[o.lf.base] + (r4300halfsigned)o.lf.offset);
		break;
	case INSTF_R1:
		REGCPU(o.r.rd);
		break;
	case INSTF_R2:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_R3:
		REGCPU2(o.i.rs, o.i.rt);
		break;
	case INSTF_MTC0:
	case INSTF_MTC1:
	case INSTF_SA:
		REGCPU(o.r.rt);
		break;
	case INSTF_R2F:
		REGFPU(o.cf.fs);
		break;
	case INSTF_R3F:
	case INSTF_C:
		REGFPU2(o.cf.fs, o.cf.ft);
		break;
	case INSTF_MFC0:
		break;
	case INSTF_MFC1:
		REGFPU(((FPUREG)o.r.rs));
		break;
	}
	*(p++) = '\n';

	TraceLoggingWriteBuf();
#undef HEX8
#undef REGCPU
#undef REGFPU
#undef REGCPU2
#undef REGFPU2
#undef C
}
}
