
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

