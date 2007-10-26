/***************************************************************************
*
* Copyright 2007 by Sean Conner.
*
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#ifndef GLOBALS_COMMON_H
#define GLOBALS_COMMON_H

#include "util.h"

#define C_FACILITIES	20
#define C_LEVELS	 7
#define C_IFT		 5

extern const struct chars_int c_facilities[C_FACILITIES];
extern const struct chars_int c_levels    [C_LEVELS];
extern const struct chars_int c_ift       [C_IFT];

#endif

