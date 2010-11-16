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

#ifndef XASM_COMMON_AMIGAOBJ_H_INCLUDED_
#define XASM_COMMON_AMIGAOBJ_H_INCLUDED_

extern bool_t ami_WriteObject(char* pszDestFilename, char* pszSourceFilename, bool_t bDebugInfo);
extern bool_t ami_WriteExecutable(char* pszDestFilename, bool_t bDebugInfo);

#endif
