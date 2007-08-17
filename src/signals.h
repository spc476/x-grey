
#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void	set_extsignal		(int,void (*)(int,siginfo_t *,void *));
void	set_signal		(int,void (*)(int));
void	check_signals		(void);
void	sighandler_critical	(int,siginfo_t *,void *);
void	sighandler_sigs		(int);
void	sighandler_chld		(int);

#endif

