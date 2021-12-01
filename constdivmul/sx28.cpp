#include <stdio.h>
#include <string.h>
#include "reg.h"
#include "sx28.h"

SX28 :: SX28(void)
{
}

SX28 :: ~SX28(void)
{
}


char *SX28 :: movwf(Reg *File, char Index)
{
	sprintf(str + strlen(str), "\tmov\t%s%d, w\n", File->Name, Index);
	counter++;
	wHolder = File;
	wIndex = Index;
	return str;
}
char *SX28 :: movfw(Reg *File, char Index)
{
	if(!wValid(File, Index))
	{
		sprintf(str + strlen(str), "\tmov\tw, %s%d\n", File->Name, Index);
		counter++;
		wHolder = File;
		wIndex = Index;
		File->setWasUsed(true, Index);
	}
	return str;
}
char *SX28 :: movlw(char unsigned literal)
{
	sprintf(str + strlen(str), "\tmov\tw, #$%.2X\n", literal);
	counter++;
	wHolder = NULL;
	return str;
}
char *SX28 :: andlw(char unsigned literal)
{
	sprintf(str + strlen(str), "\tand\tw, #$%.2X\n", literal);
	counter++;
	wHolder = NULL;
	return str;
}
char *SX28 :: clrw()
{
	sprintf(str + strlen(str), "\tclr\tw\n");
	counter++;
	wHolder = NULL;
	return str;
}
char *SX28 :: clrf(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tclr\t%s%d\n", File->Name, Index);
	counter++;
	return str;
}
char *SX28 :: swapfw(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tmov\tw, <>%s%d\n", File->Name, Index);
	counter++;
	return str;
}
char *SX28 :: swapff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tswap\t%s%d\n", File->Name, Index);
	counter++;
	return str;
}
char *SX28 :: btfsc(Reg *File, char Index, char Bit)
{
	sprintf(str + strlen(str), "\tsnb\t%s%d.%d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: btfss(Reg *File, char Index, char Bit)
{
	sprintf(str + strlen(str), "\tsb\t%s%d.%d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: comff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tnot\t%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: clrc()
{
	sprintf(str + strlen(str), "\tclc\n");
	counter++;
	return str;
}
char *SX28 :: com_msb(Reg *File, char Index)
{
/*	sprintf(str + strlen(str), "\tclz\t\t;complement carry, using inc STATUS\n");
	sprintf(str + strlen(str), "\tincsz\tSTATUS\t;(skip never executed)\n");
	counter += 2;
	return str;*/
	movlw(0x80);
	xorwff(File, Index);
	sprintf(str + strlen(str) - 1, "\t;invert bit shifted from carry\n");
	return str;
}
char *SX28 :: skpc()
{
	sprintf(str + strlen(str), "\tsc\n");
	counter++;
	return str;
}
char *SX28 :: skpnc()
{
	sprintf(str + strlen(str), "\tsnc\n");
	counter++;
	return str;
}
char *SX28 :: skpnz()
{
	sprintf(str + strlen(str), "\tsnz\n");
	counter++;
	return str;
}
char *SX28 :: rlff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\trl\t%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: rlfw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tmov\tw, <<%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: rrff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\trr\t%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: rrfw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tmov\tw, >>%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: addwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tadd\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: iorwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tor\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: iorlw(char unsigned literal)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tor\tw, #$%.2X\n", literal);
	counter++;
	return str;
}
char *SX28 :: bsf(Reg *File, char Index, char Bit)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tsetb\t%s%d.%d\n", File->Name, Index, Bit);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: subwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tsub\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: incfszw(Reg *File, char Index)
{
	wHolder = NULL;
	sprintf(str + strlen(str), "\tmovsz\tw, ++%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: incff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tinc\t%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}
char *SX28 :: decff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\tdec\t%s%d\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}

char *SX28 :: xorwff(Reg *File, char Index)
{
	if(wValid(File, Index))
	{
		wHolder = NULL;
	}
	sprintf(str + strlen(str), "\txor\t%s%d, w\n", File->Name, Index);
	counter++;
	File->setWasUsed(true, Index);
	return str;
}

char*
SX28::jmp(char* x)
{
	sprintf(str + strlen(str), "\tjmp\t%s\n", x);
	counter++;
	return str;
}

CPU_Code
SX28::whatCPU()
{
	return(CPU_SX28);
}
