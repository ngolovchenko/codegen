#include <stdio.h>
#include <string.h>
#include "reg.h"
#include "pic16.h"

extern PIC16* ALU;


Reg :: Reg()
{
	Leads = 0;
	BytesUsed = 0;
	Bits = 0;
	Name = new char[sizeof(char)];
	Name[0] = '\0';
	Sign = false;
	Zero = true;
	//wasUsed = false;
	endian = big;
	clearWasUsed();
}

Reg :: Reg(char *N, char b, endianType endi)
{
	Leads = (8 - b % 8) & 0x07;
	BytesUsed = (b + 7) / 8;
	Bits = b;
	Name = new char[(strlen(N) + 1) * sizeof(char)];
	strcpy(Name, N);
	Zero = false;
	Sign = false;
	//wasUsed = false;
	endian = endi;
	clearWasUsed();
}

Reg :: Reg(const char *N, char b, endianType endi)
{
	Leads = (8 - b % 8) & 0x07;
	BytesUsed = (b + 7) / 8;
	Bits = b;
	Name = new char[(strlen(N) + 1) * sizeof(char)];
	strcpy(Name, N);
	Zero = false;
	Sign = false;
	//wasUsed = false;
	endian = endi;
	clearWasUsed();
}

Reg :: Reg(Reg& twin)
{
	Leads = twin.Leads;
	BytesUsed = twin.BytesUsed;
	Bits = twin.Bits;
	Name =  new char[(strlen(twin.Name) + 1) * sizeof(char)];
	strcpy(Name, twin.Name);
	Zero = false;
	Sign = false;
	endian = twin.endian;
	clearWasUsed();
}

Reg :: ~Reg()
{
	delete(Name);
}

char *Reg :: ShowUsedRegs()
{
	char i;

	str[0] = 0;
	for(i = 0; i < BytesUsed; i++)
	{
		if(getWasUsed(i))
		{
			if(ALU->whatCPU() == CPU_PIC16)
			{
				sprintf(str + strlen(str),"\t%s%d\n", Name, i);
			}
			else
			{
				sprintf(str + strlen(str),"%s%d\tDS\t1\n", Name, i);
			}
		}
	}
	return str;
}

char *Reg :: ShiftLeft(char Shift)
{
	Zero = false;

	ALU->ClearSnippet();
	ALU->TakeCarry();
	ALU->SetNoteCarry(false, this);
	ALU->SetValidCarry(false, this);
	if(endian == big)
	{
		return ShiftLeftBE(Shift);
	}
	else
	{
		return ShiftLeftLE(Shift);
	}
}

char *Reg :: ShiftLeftBE(char Shift)
{
//NOTE: not tested for signed regs!
	char i, j;				//Counter

	if(Shift > Leads)
	{
		//reg needs extension
		if((Shift % 8) <= Leads)
		{
			//just add trailing zero bytes
			j = ((Shift - Leads) + 7) / 8; //bytes to add
		}
		else
		{
			//reserve one leading byte and add trailing zero bytes if needed
			for(j = GetBytes(), i = j - 1; i >= 0; i--, j--)
			{
				ALU->movfw(this, i);
				ALU->movwf(this, j);
			}
			ALU->clrf(this, 0);
			if(Sign)
			{
				ALU->btfsc(this, 1, 7);
				ALU->comff(this, 0);
			}
			Leads += 8;
			CheckBytesUsed();
			//add trailing zero bytes
			j = ((Shift - Leads) + 7) / 8 - 1; //bytes to add
		}
		//clear trailing bytes
		for(i = GetBytes(); j > 0; j--, i++)
		{
			ALU->clrf(this, i);
			Bits += 8;
			CheckBytesUsed();
		}

	}

	//final shift
	Shift %= 8;				//bits to shift
	switch(Shift)
	{
	case 6:
		{
			//for 6 do and break
			//reserve one trailing byte
			Bits += 8;
			CheckBytesUsed();
			Bits -= 8;
			i = GetBytesTotal();
			ALU->clrf(this, i);
			Bits += 8;
			while((Shift++) < 8)
			{
				for(j = 0; j <= i; j++)
				{
					ALU->rrff(this, j);
				}
				Bits--;
				Leads++;
			}
			for(j = 1; j <= i; j++)
			{
				ALU->movfw(this, j);
				ALU->movwf(this, j - 1);
			}
			Leads -= 8;
		}
		break;
	case 7:
		{
			//for 7 do and break
			//1)
			ALU->rrff(this, 0);
			//2)
			i = GetBytesTotal() - 1;
			for(j = 1; j <= i; j++)
			{
				ALU->rrfw(this, j);
				ALU->movwf(this, j - 1);
			}
			//3)
			ALU->clrf(this, i);
			ALU->rrff(this, i);
			Bits += 7;
			Leads -= 7;
		}
		break;
	case 4:
	case 5:
		{
			//for 4 and 5 swap and fall through to shift
			i = GetBytesTotal() - 1;
			if(Leads < 8)
			{
				//swap most significant byte  NO SIGNED ASSUMED
				ALU->swapff(this, 0);
			}
			for(j = 0; j < i; j++)
			{
				//takes 5*(BytesTotal - 1) instructions
				ALU->swapfw(this, j + 1);
				ALU->movwf(this, j + 1);
				ALU->andlw(15);
				ALU->iorwff(this, j);
				ALU->xorwff(this, j + 1);
			}
			Shift -= 4;
			Bits += 4;
			Leads -= 4;
		}
	default:
		{
			//for 1, 2, 3, 5 shift and break
			for(i = Shift; i > 0; i--)
			{
				if((i == Shift) || (Sign))
				{
					ALU->clrc();
				}
				for(j = GetBytesTotal() - 1; j >= 0 ; j--)
				{
					ALU->rlff(this, j);
				}
				Bits++;
				Leads--;
			}
		}
		break;
	}

	return ALU->GetSnippet();
}

char *Reg :: ShiftLeftLE(char Shift)
{
//NOTE: doesn't work for signed regs!

	char i, j, gb, ShiftB;

	gb = GetBytesTotal();

	ShiftB = Shift / 8;	//remember bytes to shift
	Shift %= 8;


	//shift bits 
	switch(Shift)
	{
	case 6:
		{
			//for 6 do and break
			//shift left 8 times
			for(j = gb - 1; j >= 0; j--)
			{
				ALU->movfw(this, j);
				ALU->movwf(this, j + 1);
			}
			ALU->clrf(this, 0);
			Bits += 8;
			CheckBytesUsed();
			//shift right two times
			if(Leads < 6)
			{
				//clear carry if the msb will be used after shifts
				ALU->clrc();
			}
			while((Shift++) < 8)
			{
				for(j = gb; j >= 0; j--)
				{
					ALU->rrff(this, j);
				}
			}
			Bits -= 2;
			Leads += 2;
			Leads %= 8;	//discard high byte if not needed
		}
		break;
	case 7:
		{
			//for 7 do and break
			//1)
			if(Leads < 7)
			{
				ALU->clrc();
				ALU->rrfw(this, gb - 1);
				ALU->movwf(this, gb);
				Leads += 8;
				CheckBytesUsed();
			}
			else
			{
				ALU->rrff(this, gb - 1);
			}
			//2)
			for(j = gb - 2; j >= 0; j--)
			{
				ALU->rrfw(this, j);
				ALU->movwf(this, j + 1);
			}
			//3)
			ALU->clrf(this, 0);
			ALU->rrff(this, 0);
			Bits += 7;
			Leads -= 7;
		}
		break;
	case 4:
	case 5:
		{
			//for 4 and 5 swap and fall through to shift
			//make swaps
			for(j = gb - 1; j >= 0; j--)
			{
				ALU->swapff(this, j);
				if(Leads < 4)
				{
					ALU->movfw(this, j);
					ALU->andlw(15);
					ALU->xorwff(this, j);
					ALU->movwf(this, j + 1);
					//make room for extra 4 bits
					Leads += 8;
					CheckBytesUsed();
				}
				else if(gb > 1 && j != gb - 1)
				{
					ALU->movfw(this, j);
					ALU->andlw(15);
					ALU->xorwff(this, j);
					ALU->iorwff(this, j + 1);
				}
			}
			Shift -= 4;
			Bits += 4;
			Leads -= 4;
		}
	default:
		{
			//for 1, 2, 3, 5 shift and break
			gb = GetBytesTotal();
			for(i = Shift; i > 0; i--)
			{
				if(i == Shift)
				{
					ALU->clrc();
				}
				for(j = 0; j < gb ; j++)
				{
					ALU->rlff(this, j);
				}
				Bits++;
				Leads--;
				if(Leads < 0)
				{
					ALU->clrf(this, gb);
					ALU->rlff(this, gb++);
					Leads += 8;
					CheckBytesUsed();
				}
			}
		}
		break;
	}

	//finally, move bytes
	gb = GetBytesTotal();
	if(ShiftB > 0)
	{
		for(i = gb - 1 + ShiftB, j = gb - 1; j >= 0; i--, j--)
		{
			ALU->movfw(this, j);
			ALU->movwf(this, i);
		}
		for(i = 0; i < ShiftB; i++)
		{
			ALU->clrf(this, i);
		}
		Bits += ShiftB * 8;
		CheckBytesUsed();
	}
	return ALU->GetSnippet();
}

char *Reg :: ShiftRight(char Shift)
{
	if(Shift >= GetSize())
	{
		Bits = 0;
		Leads = 0;
		Zero = true;
		return "";
	}
	ALU->ClearSnippet();
	Zero = false;
	if(endian == big)
	{
		return ShiftRightBE(Shift);
	}
	else
	{
		return ShiftRightLE(Shift);
	}
}

char *Reg :: ShiftRightBE(char Shift)
{
	char i, j;
	char invertMsb = 0;

	//Find modulus 8 of Shift and correct bits number after bytes move
	//Move bytes by substracting from Bits
	Bits -= Shift;
	Shift %= 8;
	Bits += Shift;
	switch(Shift)
	{
	case 6:
		{
			//do this if shift=6 and break
			if((Leads - Sign) < 2)
			{
				//reserve one byte
				for(j = GetBytesTotal() - 1; j >= 0; j--)
				{
					ALU->movfw(this, j);
					ALU->movwf(this, j + 1);
				}
				Bits += 8;
				CheckBytesUsed();
				Bits -= 8;
				//initialize MSByte
				ALU->clrf(this, 0);
				if(ALU->cHolder != this)
				{
					//free carry if it's busy
					ALU->TakeCarry();
					ALU->SetNoteCarry(false, this);
					ALU->SetValidCarry(false, this);
				}
				if(!ALU->GetNoteCarry(this))
				{
					if(Sign)
					{
						ALU->btfsc(this, 1, 7);
						ALU->comff(this, 0);
					}
				}
				else
				{
					if(Sign)
					{
						ALU->skpc();
						ALU->comff(this, 0);
					}
					else
					{
						ALU->skpnc();
						ALU->bsf(this, 0, 0);
					}
				}
			}
			else
			{
				Leads -= 8;
			}
			//left shifts
			Shift = 8 - Shift;
			while((Shift--) > 0)
			{
				for(j = GetBytesTotal(); j >= 0; j--)
				{
					ALU->rlff(this, j);
				}
			}
			Leads += 6;
			Bits -= 6;
		}
		break;
	case 7:
		{
			//do this if shift=7 and break
			//free carry if it's busy
			ALU->TakeCarry();
			ALU->SetNoteCarry(false, this);
			ALU->SetValidCarry(false, this);
			//1)
			j = GetBytesTotal() - 1;
			ALU->rlff(this, j);
			//2)
			if(Leads == 0)
			{
				for(i = j - 1; i >= 0; i--, j--)
				{
					ALU->rlfw(this, i);
					ALU->movwf(this, j);
				}
				ALU->clrf(this, 0);
				if(Sign)
				{
					ALU->skpnc();
					ALU->comff(this, 0);
				}
				else
				{
					ALU->rlff(this, 0);
				}
			}
			else
			{
				for(j = GetBytesTotal() - 2; j >= 0; j--)
				{
					ALU->rlff(this, j);
				}
				Leads -= 8;
			}
			Leads += 7;
			Bits -= 7;
		}
		break;
	case 4:
	case 5:
		{
			//do if Bytes=1 else fall through
			//if(GetBytes() == 1)
			{
				if(ALU->cHolder != this)
				{
					//free carry if it's busy
					ALU->TakeCarry();
					ALU->SetNoteCarry(false, this);
					ALU->SetValidCarry(false, this);
				}
				for(j = GetBytesTotal() - 1; j >= 0 ; j--)
				{
					if(j == GetBytesTotal() - 1)
					{
						//least significant byte
						ALU->swapfw(this, j);
						ALU->andlw(0x0f);
						ALU->movwf(this, j);
					}
					else
					{
						ALU->swapfw(this, j);
						ALU->movwf(this, j);
                        ALU->andlw(256-16);
						ALU->iorwff(this, j + 1);
						ALU->xorwff(this, j);
					}
				}
				if(!ALU->GetNoteCarry(this))
				{
					if(Sign)
					{
						ALU->movlw(256-16);
						ALU->btfsc(this, 0, 3);
						ALU->iorwff(this, 0);
					}
				}
				else
				{
					if(Sign)
					{
						ALU->movlw(256-16);
						ALU->skpc();
						ALU->iorwff(this, 0);
					}
					else
					{
						ALU->skpnc();
						ALU->bsf(this, 0, 4);
					}
				}
				Shift -= 4;
				Leads += 4;
				Bits -= 4;
				ALU->SetNoteCarry(false, this);
				ALU->SetValidCarry(false, this);
				//make the final shift if shift = 5
			}
		}
	default:
		{
			//do for shift = 0..3, and 4, 5 if Bytes > 1
			for(i = Shift; i > 0; i--)
			{
				if(ALU->cHolder != this)
				{
					//free carry if it's busy
					ALU->TakeCarry();
					ALU->SetNoteCarry(false, this);
					ALU->SetValidCarry(false, this);
				}
				if(!ALU->GetNoteCarry(this))
				{
					if(Sign)
					{
						ALU->rlfw(this, 0);
					}
					else
					{
						if(!ALU->GetValidCarry(this))
						{
							ALU->clrc();
							//ALU->SetValidCarry(false, this);
						}
					}
				}
				else
				{
					if(Sign)
					{
						invertMsb = 1; //don't forget to invert carry
										//used in the following shift
					}
				}
				for(j = 0; j < GetBytesTotal(); j++)
				{
					ALU->rrff(this, j);
					if(invertMsb && j == 0)
					{
						invertMsb = 0;	//don't repeat in the future shifts
						ALU->com_msb(this, j);
					}
				}
				ALU->SetNoteCarry(false, this);
				ALU->SetValidCarry(false, this);
				Bits--;
				Leads++;
			}
		}
		break;
	}

	//if leads are more then a byte, remove them
	if((Leads / 8) > 0)
	{
		for(i = Leads / 8, j = 0; i < GetBytesTotal(); i++, j++)
		{
			ALU->movfw(this, i);
			ALU->movwf(this, j);
		}
		Leads %= 8;
	}

	ALU->SetNoteCarry(false, this);
	ALU->SetValidCarry(false, this);
	return ALU->GetSnippet();
}

char *Reg :: ShiftRightLE(char Shift)
{
	char i, j, ShiftB, gb;
	char invertMsb = 0;

	gb = GetBytesTotal();
	ShiftB = Shift / 8;
	Shift %= 8;

	//move bytes
	if(ShiftB > 0)
	{
		for(j = ShiftB, i = 0; j < gb; j++, i++)
		{
			ALU->movfw(this, j);
			ALU->movwf(this, i);
		}
		Bits -= ShiftB * 8;
	}
	//shift bits
	switch(Shift)
	{
	case 6:
		{
			//do and break
			//Take care of carry
			ALU->TakeCarry();
			ALU->SetNoteCarry(false, this);
			ALU->SetValidCarry(false, this);
			gb = GetBytesTotal();
			//shift left 2 times
			while((Shift++) < 8)
			{
				for(j = 0; j < gb; j++)
				{
					ALU->rlff(this, j);
				}
				Leads--;
				Bits++;
				if((Leads - Sign) < 0)
				{
					ALU->clrf(this, gb);
					if(Sign)
					{
						ALU->skpnc();
						ALU->comff(this, gb);
					}
					else
					{
						ALU->rlff(this, gb);
					}
					Leads += 8;
					CheckBytesUsed();
					gb = GetBytesTotal();
				}
			}
			//move right 8 times
			for(i = 0; i < (gb - 1); i++)
			{
				ALU->movfw(this, i + 1);
				ALU->movwf(this, i);
			}
			Bits -= 8;
		}
		break;
	case 7:
		{
			//do this if shift=7 and break
			//free carry if it's busy
			ALU->TakeCarry();
			ALU->SetNoteCarry(false, this);
			ALU->SetValidCarry(false, this);
			//1)
			gb = GetBytesTotal();
			ALU->rlff(this, 0);
			//2)
			for(i = 0; i < (gb - 1); i++)
			{
				ALU->rlfw(this, i + 1);
				ALU->movwf(this, i);
			}
			if((Leads - Sign) == 0)
			{
				ALU->clrf(this, gb - 1);
				if(Sign)
				{
					ALU->skpnc();
					ALU->comff(this, gb - 1);
				}
				else
				{
					ALU->rlff(this, gb - 1);
				}
			}
			Leads += 7;
			Bits -= 7;
			Leads %= 8;	//remove unnecessary leads
			CheckBytesUsed();
		}
		break;
	case 4:
	case 5:
		{
			//do if Bytes=1 else fall through
			//if(gb == 1)
			{
				if(ALU->cHolder != this)
				{
					//free carry if it's busy
					ALU->TakeCarry();
					ALU->SetNoteCarry(false, this);
					ALU->SetValidCarry(false, this);
				}
				gb = GetBytesTotal();
				for(j = 0; j < gb; j++)
				{
					if(j == 0)
					{
						ALU->swapfw(this, j);
						ALU->andlw(15);
						ALU->movwf(this, j);
					}
					else
					{
						ALU->swapfw(this, j);
						ALU->movwf(this, j);
						ALU->andlw(256-16);
						ALU->xorwff(this, j);
						ALU->iorwff(this, j - 1);
					}
				}
				j = gb - 1;
				if(!ALU->GetNoteCarry(this))
				{
					if(Sign)
					{
						ALU->movlw(256-16);
						ALU->btfsc(this, j, 3);
						ALU->iorwff(this, j);
					}
				}
				else
				{
					if(Sign)
					{
						ALU->movlw(256-16);
						ALU->skpc();
						ALU->iorwff(this, j);
					}
					else
					{
						ALU->skpnc();
						ALU->bsf(this, j, 4);
					}
				}
				Shift -= 4;
				Leads += 4;
				Bits -= 4;
				ALU->SetNoteCarry(false, this);
				ALU->SetValidCarry(false, this);
				//make the final shift if shift = 5
			}
		}
	default:
		{
			//do for shift = 0..3, and 4, 5 if Bytes > 1
			for(i = Shift; i > 0; i--)
			{
				gb = GetBytesTotal();
				if(ALU->cHolder != this)
				{
					//free carry if it's busy
					ALU->TakeCarry();
					ALU->SetNoteCarry(false, this);
					ALU->SetValidCarry(false, this);
				}
				if(!ALU->GetNoteCarry(this))
				{
					if(Sign)
					{
						if((Leads % 8) != 0)
						{
							//if sign and higher byte has significant
							//bit (at least one).
							//If Leads % 8 == 0 simple shift right will be
							//enough, because the higher byte will be
							//lost after this shift.
							ALU->rlfw(this, gb - 1);
						}
					}
					else
					{
						if(!ALU->GetValidCarry(this))
						{
							ALU->clrc();
							//ALU->SetValidCarry(false, this);
						}
					}
				}
				else
				{
					if(Sign)
					{
						invertMsb = 1; //don't forget to invert carry
										//used in the following shift
					}
				}
				for(j = gb - 1; j >= 0; j--)
				{
					ALU->rrff(this, j);
					if(invertMsb && j == (gb - 1))
					{
						invertMsb = 0;	//don't repeat in the future shifts
						ALU->com_msb(this, j);
					}

				}
				ALU->SetNoteCarry(false, this);
				ALU->SetValidCarry(false, this);
				Bits--;
				Leads++;
				if(!Sign || Leads != 8)
				{
					Leads %= 8;	//discard unneeded leads
				}
			}
		}
		break;
	}
/*
	//if leads are more then a byte, remove them
	if((Leads / 8) > 0)
	{
		for(i = Leads / 8, j = 0; i < GetBytesTotal(); i++, j++)
		{
			ALU->movfw(this, i);
			ALU->movwf(this, j);
		}
		Leads %= 8;
	}
*/
	ALU->SetNoteCarry(false, this);
	ALU->SetValidCarry(false, this);
	return ALU->GetSnippet();
}

void Reg :: Normalize(Reg *R2)
{
	ALU->TakeCarry();
	if(endian == big)
	{
		NormalizeBE(R2);
	}
	else
	{
		NormalizeLE(R2);
	}
}

void Reg :: NormalizeBE(Reg *R2)
{
	char i, j, d;

	d = R2->GetBytesTotal() - GetBytesTotal();
	if(d > 0)
	{
		//reserve leading bytes
		for(j = GetBytesTotal() + d - 1, i = j - 1; i >= 0; i--, j--)
		{
			ALU->movfw(this, i);
			ALU->movwf(this, j);
		}
		for(i = 0; i < d; i++)
		{
			ALU->clrf(this, i);
			if(Sign)
			{
				ALU->btfsc(this, d, 7);
				ALU->comff(this, i);
			}
			Leads += 8;
		}
		CheckBytesUsed();
	}
}

void Reg :: NormalizeLE(Reg *R2)
{
	char i, d;

	d = R2->GetBytesTotal() - GetBytesTotal();
	if(d > 0)
	{
		for(i = GetBytesTotal(); d > 0; i++, d--)
		{
			//reserve leading byte
			ALU->clrf(this, i);
			if(Sign)
			{
				ALU->btfsc(this, i - 1, 7);
				ALU->comff(this, i);
			}
			Leads += 8;
		}
		CheckBytesUsed();
	}
}

char *Reg :: Add(Reg *R2)
{
	ALU->ClearSnippet();
	Normalize(R2);
	if(endian == big)
	{
		return AddBE(R2);
	}
	else
	{
		return AddLE(R2);
	}
}

char *Reg :: AddBE(Reg *R2)
{
	char i, j;

	for(i = R2->GetBytesTotal() - 1, j = GetBytesTotal() - 1; i >= 0; i--, j--)
	{
		if(i == (R2->GetBytesTotal() - 1))
		{
			// first bytes addition
			ALU->movfw(R2, i);
			ALU->addwff(this, j);
		}
		else
		{
			// other bytes addition with carry propagation
			ALU->movfw(R2, i);
			ALU->skpnc();
			ALU->incfszw(R2, i);
			ALU->addwff(this, j);
		}
	}
	if(j >= 0)
	{
		//in case right operand had less bytes, replace them with zero literals
		ALU->movlw(1);
		while(j >= 0)
		{
			ALU->skpnc();
			ALU->addwff(this, j);
			j--;
		}
	}

	i = Leads;
	if(Leads > R2->Leads)
	{
		Leads = R2->Leads;
	}
	Leads --;	//one bit added after each addition

	ALU->SetValidCarry(true, this); //after addition carry is always valid
	ALU->SetNoteCarry(false, this);
	//Correct leads on overflow to carry
	if(Leads == -1)
	{
		ALU->SetNoteCarry(true, this);
		Leads = 0;
	}
	Bits += i - Leads;

	return ALU->GetSnippet();
}

char *Reg :: AddLE(Reg *R2)
{
	char i, j, r2b, rb;
	r2b = R2->GetBytesTotal();
	rb = GetBytesTotal();

	for(i = 0, j = 0; i < r2b; i++, j++)
	{
		if(i == 0)
		{
			// first bytes addition
			ALU->movfw(R2, i);
			ALU->addwff(this, j);
		}
		else
		{
			// other bytes addition with carry propagation
			ALU->movfw(R2, i);
			ALU->skpnc();
			ALU->incfszw(R2, i);
			ALU->addwff(this, j);
		}
	}
	if(j < rb)
	{
		//in case right operand had less bytes, replace them with zero literals
		ALU->movlw(1);
		while(j < rb)
		{
			ALU->skpnc();
			ALU->addwff(this, j);
			j++;
		}
	}

	i = Leads;
	if(Leads > R2->Leads)
	{
		Leads = R2->Leads;
	}
	Leads --;	//one bit added after each addition

	ALU->SetValidCarry(true, this); //after addition carry is always valid
	ALU->SetNoteCarry(false, this);
	//Correct leads on overflow to carry
	if(Leads == -1)
	{
		ALU->SetNoteCarry(true, this);
		Leads = 0;
	}
	Bits += i - Leads;

	return ALU->GetSnippet();
}

char *Reg :: Sub(Reg *R2)
{
	ALU->ClearSnippet();
	Normalize(R2);
	ALU->SetValidCarry(false, this);
	if(endian == big)
	{
		return SubBE(R2);
	}
	else
	{
		return SubLE(R2);
	}
}

char *Reg :: SubBE(Reg *R2)
{
	char i, j;

	if(Sign)
	{
		//substraction from negative number will
		//add one more significant bit
		//
		if(R2->Leads < 2)
		{
			//add a leading byte
			for(j = GetBytes(), i = j - 1; i >= 0; i--, j--)
			{
				ALU->movfw(this, i);
				ALU->movwf(this, j);
			}
			ALU->clrf(this, 0);
			ALU->btfsc(this, 1, 7);
			ALU->comff(this, 0);
			Bits += Leads - R2->Leads;	//set size of r2 for this
			Leads = 8 + R2->Leads;
			CheckBytesUsed();
		}
	}

	for(i = R2->GetBytesTotal() - 1, j = GetBytesTotal() - 1; i >= 0; i--, j--)
	{
		if(i == (R2->GetBytesTotal() - 1))
		{
			// first bytes substraction
			ALU->movfw(R2, i);
			ALU->subwff(this, j);
		}
		else
		{
			// other bytes substraction with carry propagation
			ALU->movfw(R2, i);
			ALU->skpc();
			ALU->incfszw(R2, i);
			ALU->subwff(this, j);
		}
	}
	if(j >= 0)
	{
		//in case right operand had less bytes, replace them with zero literals
		ALU->movlw(1);
		while(j >= 0)
		{
			ALU->skpc();
			ALU->subwff(this, j);
			j--;
		}
	}

	if(GetBytesTotal() == R2->GetBytesTotal())
	{
		i = Leads; //choose least number of leads
		if(Leads > R2->Leads)
		{
			Leads = R2->Leads;
		}
		Bits += i - Leads;	//correct number of bits
	}

	if(Sign)
	{
		Leads --;
		Bits ++;
	}

	ALU->SetNoteCarry(false, this);
	if(Leads == 0)
	{
		//substraction using all bits -
		//carry contains sign
		ALU->SetNoteCarry(true, this);
	}
	return ALU->GetSnippet();
}

char *Reg :: SubLE(Reg *R2)
{
	char i, j, gb, gb2;

	gb = GetBytesTotal();
	//gb2 = R2->GetBytesTotal();

	if(Sign)
	{
		//substraction from negative number will
		//add one more significant bit
		//
		if(R2->Leads <= 1)
		{
			//add a leading byte
			ALU->clrf(this, gb);
			ALU->btfsc(this, gb - 1, 7);
			ALU->comff(this, gb);
			Bits += Leads - R2->Leads;	//set size of r2 for this
			Leads = 8 + R2->Leads;
			CheckBytesUsed();
		}
	}
	//update sizes
	gb = GetBytesTotal();
	gb2 = R2->GetBytesTotal();

	for(i = 0, j = 0; i < gb2; i++, j++)
	{
		if(i == 0)
		{
			// first bytes substraction
			ALU->movfw(R2, i);
			ALU->subwff(this, j);
		}
		else
		{
			// other bytes substraction with carry propagation
			ALU->movfw(R2, i);
			ALU->skpc();
			ALU->incfszw(R2, i);
			ALU->subwff(this, j);
		}
	}
	if(j < gb)
	{
		//in case right operand had less bytes, replace them with zero literals
		ALU->movlw(1);
		while(j < gb)
		{
			ALU->skpc();
			ALU->subwff(this, j);
			j++;
		}
	}

	if(GetBytesTotal() == R2->GetBytesTotal())
	{
		i = Leads; //choose least number of leads
		if(Leads > R2->Leads)
		{
			Leads = R2->Leads;
		}
		Bits += i - Leads;	//correct number of bits
	}

	if(Sign)
	{
		Leads --;
		Bits ++;
	}

	ALU->SetNoteCarry(false, this);
	if(Leads == 0)
	{
		//substraction using all bits -
		//carry contains sign
		ALU->SetNoteCarry(true, this);
	}
	return ALU->GetSnippet();
}

char *Reg :: Round()
{
	bool label = false;
	char* labelName="no_inc";

	ShiftRight(1); //shift LSb to carry
	
	//sheck if label required
	if(GetBytesTotal() > 1 || Leads == 0)
	{
		label = true;
	}
	//check carry
	if(label)
	{
		ALU->skpc();
		ALU->jmp(labelName);
	}
	else
	{
		ALU->skpnc();
	}
	
	if(endian == big)
	{
		RoundBE();
	}
	else
	{
		RoundLE();
	}
	if(label)
	{
		ALU->label(labelName);
	}
	return ALU->GetSnippet();
}

char *Reg :: RoundBE()
{
	char i, j;

	if(Leads == 0)
	{
		//reserve one byte
		for(j = GetBytesTotal(), i = j - 1; i >= 0; i--, j--)
		{
			ALU->movfw(this, i);
			ALU->movwf(this, j);
		}
		ALU->clrf(this, 0);
		Leads += 8;
		CheckBytesUsed();
	}
	i = GetBytesTotal() - 1;
	for(j = i; i >= 0; i--)
	{
		if(i != j)
		{
			ALU->skpnz();
		}
		ALU->incff(this, i);
	}
	return ALU->GetSnippet();
}

char *Reg :: RoundLE()
{
	char i, rb;

	if(Leads == 0)
	{
		//reserve byte
		ALU->clrf(this, GetBytesTotal());
		Leads += 8;
		CheckBytesUsed();
	}

	rb = GetBytesTotal();

	for(i = 0; i < rb; i++)
	{
		if(i != 0)
		{
			ALU->skpnz();
		}
		ALU->incff(this, i);
	}
	return ALU->GetSnippet();
}

char *Reg :: Neg()
{
	ALU->ClearSnippet();
	if(endian == big)
	{
		return NegBE();
	}
	else
	{
		return NegLE();
	}
}

char *Reg :: NegBE()
{
	char i, j;

	if(Leads == 0)
	{
		//reserve byte to place sign
		for(j = GetBytesTotal(), i = j - 1; i >= 0; i--, j--)
		{
			ALU->movfw(this, i);
			ALU->movwf(this, j);
		}
		ALU->clrf(this, 0);
		Leads += 8;
		CheckBytesUsed();
	}
	for(i = 0; i < GetBytesTotal(); i++)
	{
		ALU->comff(this, i);
	}
	for(i = GetBytesTotal() - 1, j = i; i >= 0; i--)
	{
		if(i != j)
		{
			ALU->skpnz();
		}
		ALU->incff(this, i);
	}
	//Leads--;
	//Bits++;
	return ALU->GetSnippet();
}

char *Reg :: NegLE()
{
	char i, rb;

	if(Leads == 0)
	{
		//reserve byte to place sign
		ALU->clrf(this, GetBytesTotal());
		Leads += 8;
		CheckBytesUsed();
	}

	rb = GetBytesTotal();

	for(i = 0; i < rb; i++)
	{
		ALU->comff(this, i);
	}
	for(i = 0; i < rb; i++)
	{
		if(i != 0)
		{
			ALU->skpnz();
		}
		ALU->incff(this, i);
	}
	return ALU->GetSnippet();
}

char *Reg :: Copy(Reg *R2)
{
	char i;
	
	ALU->ClearSnippet();
	ALU->TakeCarry();

	//Order for copying may be important
	//for temps reduction. The last copied
	//byte should be less significant to
	//allow it's direct usage in W (without temp)
	//in the following operations like addition
	//or substraction
	if(endian == big)
	{
		for(i = 0; i < R2->GetBytes(); i++)
		{
			ALU->movfw(R2, i);
			ALU->movwf(this, i);
		}
	}
	else
	{
		for(i = R2->GetBytes() - 1; i >= 0; i--)
		{
			ALU->movfw(R2, i);
			ALU->movwf(this, i);
		}
	}
	Bits = R2->Bits;
	Leads = R2->Leads;
	CheckBytesUsed();
	Sign = R2->Sign;
	Zero = R2->Zero;
	return ALU->GetSnippet();
}

char*
Reg::CopyToUsed(Reg *R2)
{
	char i;
	//specialized function, skips copying to not used regs.
	//Can't be used inside function generation.
	//It's actually a modified Copy().
	ALU->initialize(); //start anew
	if(endian == big)
	{
		for(i = 0; i < R2->GetBytes(); i++)
		{
			ALU->movfw(R2, i);
			if(getWasUsed(i))
			{
				ALU->movwf(this, i);
			}
		}
	}
	else
	{
		for(i = R2->GetBytes() - 1; i >= 0; i--)
		{
			ALU->movfw(R2, i);
			if(getWasUsed(i))
			{
				ALU->movwf(this, i);
			}
		}
	}

	return ALU->GetSnippet();
}


void Reg :: CheckBytesUsed()
{
	char i;
	i = GetBytesTotal() - BytesUsed;
	if(i > 0)
	{
		BytesUsed += i;
	}
}

void Reg :: SetSize(char b, char l)	//set size of reg in bits and leads
{
	Bits = b;
	Leads = l;
	CheckBytesUsed();
}

void 
Reg::setWasUsed(bool x, char Index)
{
	wasUsed[Index] = x;
}

bool 
Reg::getWasUsed()
{
	int i;
	int b;
	b = GetBytesTotal();
	for(i = 0; i < b; i++)
	{
		if(wasUsed[i])
		{
			return(true);
		}
	}
	return(false);

}

bool 
Reg::getWasUsed(char Index)
{
	return(wasUsed[Index]);
}

void 
Reg::clearWasUsed()
{
	int i;
	for(i = 0; i < MAXREGSNUMBER; i++)
	{
		wasUsed[i] = false;
	}
}

