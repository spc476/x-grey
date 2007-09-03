
#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void	set_signal		(int,void (*)(int));
void	check_signals		(void);
void	sighandler_critical	(int);
void	sighandler_sigs		(int);
void	sighandler_chld		(int);

#endif

