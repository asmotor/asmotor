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

#ifndef	INCLUDE_PARSE_H
#define	INCLUDE_PARSE_H

extern	BOOL	parse_Do(void);
extern	void	parse_GetToken(void);
extern	SExpression* parse_Expression(void);
extern	SExpression* parse_CheckRange(SExpression* expr, SLONG low, SLONG high);
extern	SExpression* parse_CreateConstExpr(SLONG value);
extern	SExpression* parse_CreateABSExpr(SExpression* expr);
extern	SExpression* parse_CreateORExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateADDExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateSUBExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateANDExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateXORExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreatePCRelExpr(SExpression* expr, int nAdjust);
extern	SExpression* parse_CreateBITExpr(SExpression* right);
extern	SExpression* parse_CreateSHLExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateSHRExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_CreateMULExpr(SExpression* left, SExpression* right);
extern	SExpression* parse_DuplicateExpr(SExpression* expr);
extern	void	parse_FreeExpression(SExpression* expr);
extern	SLONG	parse_ConstantExpression(void);
extern BOOL	parse_ExpectChar(char ch);
extern BOOL parse_ExpectComma(void);

#endif	/*INCLUDE_PARSE_H*/