/*  Copyright 2008 Carsten Sørensen

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xlink.h"
#include <math.h>

#define	fix2double(i)	((double)((i)/65536.0))
#define	double2fix(d)	((SLONG)((d)*65536.0))

#ifndef	PI
#define	PI				(acos(-1))
#endif

#define	STACKSIZE	256

static	SLONG	stack[STACKSIZE];
static	SLONG	stack_index;

static	void	push(SLONG value)
{
	if(stack_index<STACKSIZE)
	{
		stack[stack_index++]=value;
	}
	else
	{
		Error("patch too complex");
	}
}

static	SLONG	pop(void)
{
	if(stack_index>0)
	{
		return stack[--stack_index];
	}
	else
	{
		Error("mangled patch");
		return 0;
	}
}

static	void	pop2(SLONG* left, SLONG* right)
{
	*right=pop();
	*left=pop();
}



SLONG	calc_patch(SPatch* patch, SSection* sect)
{
	SLONG	size;
	UBYTE* expr;

	size=patch->ExprSize;
	expr=patch->pExpr;
	stack_index=0;

	while(size>0)
	{
		SLONG	left,
				right;

		size-=1;

		switch(*expr++)
		{
			case	OBJ_OP_SUB:
			{
				pop2(&left, &right);
				push(left-right);
				break;
			}
			case	OBJ_OP_ADD:
			{
				pop2(&left, &right);
				push(left+right);
				break;
			}
			case	OBJ_OP_XOR:
			{
				pop2(&left, &right);
				push(left^right);
				break;
			}
			case	OBJ_OP_OR:
			{
				pop2(&left, &right);
				push(left|right);
				break;
			}
			case	OBJ_OP_AND:
			{
				pop2(&left, &right);
				push(left&right);
				break;
			}
			case	OBJ_OP_SHL:
			{
				pop2(&left, &right);
				push(left<<right);
				break;
			}
			case	OBJ_OP_SHR:
			{
				pop2(&left, &right);
				push(left>>right);
				break;
			}
			case	OBJ_OP_MUL:
			{
				pop2(&left, &right);
				push(left*right);
				break;
			}
			case	OBJ_OP_DIV:
			{
				pop2(&left, &right);
				push(left/right);
				break;
			}
			case	OBJ_OP_MOD:
			{
				pop2(&left, &right);
				push(left%right);
				break;
			}
			case	OBJ_OP_LOGICOR:
			{
				pop2(&left, &right);
				push((left||right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICAND:
			{
				pop2(&left, &right);
				push((left&&right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICNOT:
			{
				push(pop()?0:1);
				break;
			}
			case	OBJ_OP_LOGICGE:
			{
				pop2(&left, &right);
				push((left>=right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICGT:
			{
				pop2(&left, &right);
				push((left>right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICLE:
			{
				pop2(&left, &right);
				push((left<=right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICLT:
			{
				pop2(&left, &right);
				push((left<right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICEQU:
			{
				pop2(&left, &right);
				push((left==right)?1:0);
				break;
			}
			case	OBJ_OP_LOGICNE:
			{
				pop2(&left, &right);
				push((left!=right)?1:0);
				break;
			}
			case	OBJ_FUNC_LOWLIMIT:
			{
				pop2(&left, &right);
				if(left>=right)
				{
					push(left);
				}
				else
				{
					Error("LOWLIMIT failed");
				}
				break;
			}
			case	OBJ_FUNC_HIGHLIMIT:
			{
				pop2(&left, &right);
				if(left<=right)
				{
					push(left);
				}
				else
				{
					Error("HIGHLIMIT failed");
				}
				break;
			}
			case	OBJ_FUNC_FDIV:
			{
				pop2(&left, &right);
				push(double2fix(fix2double(left)/fix2double(right)));
				break;
			}
			case	OBJ_FUNC_FMUL:
			{
				pop2(&left, &right);
				push(double2fix(fix2double(left)*fix2double(right)));
				break;
			}
			case	OBJ_FUNC_ATAN2:
			{
				pop2(&left, &right);
				push(double2fix(atan2(fix2double(left),fix2double(right))/2/PI*65536));
				break;
			}
			case	OBJ_FUNC_SIN:
			{
				push(double2fix(sin(fix2double(pop()))*2*PI/65536));
				break;
			}
			case	OBJ_FUNC_COS:
			{
				push(double2fix(cos(fix2double(pop()))*2*PI/65536));
				break;
			}
			case	OBJ_FUNC_TAN:
			{
				push(double2fix(tan(fix2double(pop()))*2*PI/65536));
				break;
			}
			case	OBJ_FUNC_ASIN:
			{
				push(double2fix(asin(fix2double(pop()))/2/PI*65536));
				break;
			}
			case	OBJ_FUNC_ACOS:
			{
				push(double2fix(acos(fix2double(pop()))/2/PI*65536));
				break;
			}
			case	OBJ_FUNC_ATAN:
			{
				push(double2fix(atan(fix2double(pop()))/2/PI*65536));
				break;
			}
			case	OBJ_CONSTANT:
			{
				ULONG	d;

				d =(*expr++);
				d|=(*expr++)<<8;
				d|=(*expr++)<<16;
				d|=(*expr++)<<24;

				push(d);
				size-=4;
				break;
			}
			case	OBJ_SYMBOL:
			{
				ULONG	d;

				d =(*expr++);
				d|=(*expr++)<<8;
				d|=(*expr++)<<16;
				d|=(*expr++)<<24;

				push(sect_GetSymbolValue(sect,d));
				size-=4;
				break;
			}
			case	OBJ_FUNC_BANK:
			{
				ULONG	d;

				d =(*expr++);
				d|=(*expr++)<<8;
				d|=(*expr++)<<16;
				d|=(*expr++)<<24;

				push(sect_GetSymbolBank(sect,d));
				size-=4;
				break;
			}
		}
	}

	return pop();
}

static	void	do_section(SSection* sect)
{
	SPatches* patches;

	patches=sect->pPatches;

	if(patches)
	{
		ULONG	i;

		for(i=0; i<patches->TotalPatches; i+=1)
		{
			SLONG	value;
			SPatch* patch;

			patch=&patches->Patches[i];

			value=calc_patch(patch, sect);

			switch(patch->Type)
			{
				case	PATCH_BYTE:
				{
					if(value>=-128 && value<=255)
					{
						sect->pData[patch->Offset] = (UBYTE)value;
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case	PATCH_LWORD:
				{
					if(value>=-32768 && value<=65535)
					{
						sect->pData[patch->Offset+0] = (UBYTE)value;
						sect->pData[patch->Offset+1] = (UBYTE)(value >> 8);
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case	PATCH_BWORD:
				{
					if(value>=-32768 && value<=65535)
					{
						sect->pData[patch->Offset+0] = (UBYTE)(value>>8);
						sect->pData[patch->Offset+1] = (UBYTE)value;
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case	PATCH_LLONG:
				{
					sect->pData[patch->Offset+0] = (UBYTE)value;
					sect->pData[patch->Offset+1] = (UBYTE)(value>>8);
					sect->pData[patch->Offset+2] = (UBYTE)(value>>16);
					sect->pData[patch->Offset+3] = (UBYTE)(value>>24);
					break;
				}
				case	PATCH_BLONG:
				{
					sect->pData[patch->Offset+0] = (UBYTE)(value>>24);
					sect->pData[patch->Offset+1] = (UBYTE)(value>>16);
					sect->pData[patch->Offset+2] = (UBYTE)(value>>8);
					sect->pData[patch->Offset+3] = (UBYTE)value;
					break;
				}
				default:
				{
					Error("unhandled patch type");
				}
			}
		}
	}
}


void	patch_Process(void)
{
	SSection* sect;

	sect=pSections;

	while(sect)
	{
		if(sect->Used)
		{
			do_section(sect);
		}
		sect=sect->pNext;
	}
}