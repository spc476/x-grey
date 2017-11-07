/***************************************************************************
*
* Copyright 2010 by Sean Conner.
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
#define C_IFT		 6
#define C_REASONS	 8

extern struct chars_int const c_facilities[C_FACILITIES];
extern struct chars_int const c_levels    [C_LEVELS];
extern struct chars_int const c_ift       [C_IFT];
extern struct chars_int const c_reason	  [C_REASONS];

#endif
