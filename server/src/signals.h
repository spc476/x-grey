/***************************************************************************
*
* Copyright 2007 by Sean Conner.
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

#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <unistd.h>

void	check_signals			(void);
void	sighandler_critical		(int);
void	sighandler_critical_child	(int);
void	sighandler_sigs			(int);
void	sighandler_chld			(int);
pid_t	gld_fork			(void);
void	save_state			(void);

#endif

