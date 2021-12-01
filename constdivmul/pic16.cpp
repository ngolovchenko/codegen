#include <stdio.h>
#include <string.h>
#include "reg.h"
#include "pic16.h"

PIC16 :: PIC16(void)
{
	initialize();
}

PIC16::~PIC16(void)
{
}


void
PIC16::initialize()
{
	cHolder = NULL;
	wHolder = NULL;
	vNoteCarry = false;
	vValidCarry = false;
	ClearSnippet();
	clearCounter();
}

void PIC16 :: SetNoteCarry(bool val, Reg *R)	//Set vNoteCarry flag
{
	vNoteCarry = val;
	cHolder = R;
}
void PIC16 :: SetValidCarry(bool val, Reg *R)	//Set vValidCarry flag
{
	vValidCarry = val;
	cHolder = R;
}
bool PIC16 :: GetNoteCarry(Reg *R)	//Get vNoteCarry flag
{
	if(R == cHolder)
	{
		return vNoteCarry;
	}
	return false;
}
bool PIC16 :: GetValidCarry(Reg *R)	//Get vValidCarry flag
{
	if(R == cHolder)
	{
		return vValidCarry;
	}
	return false;
}

char *PIC16 :: TakeCarry()
{
	char i, j, gbt;
	
	if(GetNoteCarry(cHolder))
	{
		if(cHolder->endian == big)
		{
			for(j = cHolder->GetBytesTotal(), i = j - 1; i >= 0; i--, j--)
			{
				movfw(cHolder, i);
				movwf(cHolder, j);
			}
			clrf(cHolder, 0);
			if(cHolder->Sign)
			{
				skpc();
				comff(cHolder, 0);
			}
			else
			{
				rlff(cHolder, 0);
			}
		}
		else
		{
			gbt = cHolder->GetBytesTotal();
			clrf(cHolder, gbt);
			if(cHolder->Sign)
			{
				skpc();
				comff(cHolder, gbt);
			}
			else
			{
				rlff(cHolder, gbt);
			}
		}
		SetNoteCarry(false, cHolder);
		SetValidCarry(false, cHolder);
		cHolder->SetSize(cHolder->GetSize() - cHolder->Sign + 1, 7 + cHolder->Sign);
	}
	return str;
}

char *PIC16 :: movwf(Reg *File, char Index)
{
	sprintf(str + strlen(str), "\tmovwf\t%s%d\n", File->Name, Index);
	counter++;
	wHolder = File;
	wIndex = Index;
	return str;
}
char *PIC16 :: movfw(Reg *File, char Index)
{
	if(!wValid(File, Index))
	{
		sprintf(str + strlen(str), "\tmovf\t%s%d, w\n", File->Name, Index);
		counter++;
		wHolder = File;
		wIndex = Index;
		File->setWasUsed(true, Index);
	}
	return str;
}
char *PIC16 :: movlw(char unsigned literal)
{
	sprintf(str + strlen(str), "\tmovlw\t0x%.2X\n", literal);
	counter++;
	wHolder = NULL;
	return str;
}
char *PIC16 :: andlw(char unsigned literal)
{
	sprintf(str + strlen(str), "\tandlw\t0x%.2X\n", literal);
	counter++;
	wHolder = NULL;
	return str;
}
char *PIC16 :: clrw()
{
	sprintf(str + strlen(str), "\tclrw\n");
	counter++;
	wHolder = NULL;
	return str;
}
char *PIC16 :: clrf(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tclrf\t%s%d\n", File->Name, Index);
	counter++;
	return str;
}
char *PIC16 :: swapfw(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tswapf\t%s%d, w\n", File->Name, Index);
	counter++;
	return str;
}
char *PIC16 :: swapff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tswapf\t%s%d, f\n", File->Name, Index);
	counter++;
	return str;
}
char *PIC16 :: btfsc(Reg *File, char Index, char Bit)
{
	sprintf(str + strlen(str), "\tbtfsc\t%s%d, %d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: btfss(Reg *File, char Index, char Bit)
{
	sprintf(str + strlen(str), "\tbtfss\t%s%d, %d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: comff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tcomf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: clrc()
{
	sprintf(str + strlen(str), "\tclrc\n");
	counter++;
	return str;
}
char *PIC16 :: com_msb(Reg *File, char Index)
{
	movlw(0x80);
	xorwff(File, Index);
	sprintf(str + strlen(str) - 1, "\t;invert bit shifted from carry\n");
	return str;
}
char *PIC16 :: skpc()
{
	sprintf(str + strlen(str), "\tskpc\n");
	counter++;
	return str;
}
char *PIC16 :: skpnc()
{
	sprintf(str + strlen(str), "\tskpnc\n");
	counter++;
	return str;
}
char *PIC16 :: skpnz()
{
	sprintf(str + strlen(str), "\tskpnz\n");
	counter++;
	return str;
}
char *PIC16 :: rlff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\trlf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: rlfw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\trlf\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: rrff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\trrf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: rrfw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\trrf\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: addwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\taddwf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: iorwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tiorwf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: iorlw(char unsigned literal)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tiorlw\t0x%.2X\n", literal);
	counter++;
	return str;
}
char *PIC16 :: bsf(Reg *File, char Index, char Bit)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tbsf\t%s%d, %d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: subwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tsubwf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: incfszw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tincfsz\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: incff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tincf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *PIC16 :: decff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tdecf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
bool PIC16 :: wValid(Reg *File, char Index)
{
	if((File == wHolder) && (wIndex == Index))
	{
		return true;
	}
	return false;
}
char *PIC16 :: xorwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\txorwf\t%s%d, f\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}

char*
PIC16::jmp(char* x)
{
	sprintf(str + strlen(str), "\tgoto\t%s\n", x);
	counter++;
	return str;
}

char*
PIC16::label(char* x)
{
	sprintf(str + strlen(str), "%s\n", x);
	return str;
}


CPU_Code
PIC16::whatCPU()
{
	return(CPU_PIC16);
}

void 
PIC16::clearCounter()
{
	counter = 0;
}

int 
PIC16::getCounter()
{
	return(counter);
}

