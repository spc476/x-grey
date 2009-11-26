
#ifndef GREYLIST_CONF
#define GREYLIST_CONF

/********************************************************************
* 
* Configuration parameters for GLD (the SERVER).  The default parameters
* can be set here, but all can be overridden at runtime with the appropriate
* command line options.
*
********************************************************************/

	/*-----------------------------------------------
	; defines the default host the server runs on.
	; This is what the other programs will use to
	; connect to the server.  This can be a hostname
	; (like 'localhost' or 'gld.example.net' or an
	; IP address as a string, like "10.0.0.22"
	;----------------------------------------------*/
	
#define SERVER_HOST "localhost"

	/*----------------------------------------------
	; defines the interface that server will bind to.
	; If you want it to bind to all possible interfaces,
	; use "0.0.0.0".  Otherwise, use the name or IP
	; address (as a string) of the interface you want
	; the server to use.
	;----------------------------------------------*/
	
#define SERVER_BINDHOST	"localhost"

	/*------------------------------------------------
	; The port the server listens on.  It will use both
	; UDP and TCP for communicating with the various
	; modules.
	;--------------------------------------------------*/
	
#define SERVER_PORT 9990

	/*------------------------------------------------
	; The file the server will write its PID to.
	;------------------------------------------------*/
	
#define SERVER_PIDFILE "/var/run/gld.pid"

	/*----------------------------------------------
	; The directory the server will use to save its
	; state (and reload if restarted).  The SENDMAIL
	; client will also use this directory for its
	; pipe to sendmail.
	;----------------------------------------------*/
	
#define SERVER_STATEDIR "/var/state/gld"

	/*-----------------------------------------------
	; Information used when logging to syslog.  Any messages
	; at a lower priority than SERVER_LOG_LEVEL are not
	; logged at all.
	;-------------------------------------------------*/
	
#define SERVER_LOG_ID		"gld"
#define SERVER_LOG_FACILITY	LOG_MAIL
#define SERVER_LOG_LEVEL	LOG_INFO

	/*----------------------------------------------
	; The maximum number of tuples to keep in memory.
	; This is allocated *once* and doesn't change for
	; the lifetime of the server.  Each tuple takes 256
	; bytes of memory, so plan accordingly.
	;-----------------------------------------------*/
	
#define SERVER_MAX_TUPLES 65536uL

	/*----------------------------------------------
	; The tuple list will be scanned every SERVER_CLEANUP
	; seconds to clean up expired tuple records.
	;-----------------------------------------------*/
	
#define SERVER_CLEANUP			(60 * 5)

	/*---------------------------------------------------
	; The server will save its state every SERVER_SAVESTATE
	; seconds.
	;-----------------------------------------------------*/
	
#define SERVER_SAVESTATE		(3600.0)

	/*----------------------------------------------------
	; The minimum amount of time (in seconds) that the server
	; will return "TRY AGAIN LATER" to foreign SMTP servers
	; for a new tuple.
	;---------------------------------------------------------*/
	
#define SERVER_TIMEOUT_EMBARGO		(60.0 * 25.0)

	/*----------------------------------------------------------
	; The maximum amount of time (in seconds) that the server
	; will keep a tuple in the system that hasn't been flagged
	; for the whitelist.
	;----------------------------------------------------------*/
	
#define SERVER_TIMEOUT_GREYLIST		(3600.0 * 6.0)

	/*------------------------------------------------------------
	; The maximum amount of time (in seconds) that the server
	; will keep whitelisted tuples.
	;------------------------------------------------------------*/
	
#define SERVER_TIMEOUT_WHITELIST	(3600.0 * 24.0 * 36.0)

/**************************************************************************
*
* Configuration paramters for the GLD-MCP program (Master Control Program)
*
**************************************************************************/

	/*-------------------------------------------
	; Various help files are stored here
	;------------------------------------------*/
	
#define MCP_HELPDIR	"/usr/local/share/gld"

	/*----------------------------------------
	; Default pager program.  Can be overriden
	; with the PAGER environment variable.
	;-----------------------------------------*/
	
#define MCP_PAGER	"/bin/more"

	/*-------------------------------------------
	; Communications with GLD will timeout after
	; this number of seconds.
	;---------------------------------------------*/
	
#define MCP_TIMEOUT	5

	/*-------------------------------------------------------
	; Settings for syslog() (see notes for SERVER_LOG_LEVEL
	; for more information)
	;---------------------------------------------------------*/

#define MCP_LOG_FACILITY	LOG_MAIL
#define MCP_LOG_LEVEL		LOG_ERR
#define MCP_LOG_ID		"gld-mcp"

	/* END OF LINE */

/************************************************************************
*
* Configuration parameters for PFC (Postfix Client)
*
***********************************************************************/

	/*----------------------------------------------------------
	; the local interface PFC will bind to.  Set to "0.0.0.0"
	; to use any interface on the system.  If you do set this
	; to a specific interface, make sure that interface can
	; communicate with the greylist daemon.
	;---------------------------------------------------------*/
	
#define POSTFIX_HOST	"0.0.0.0"

	/*-------------------------------------------------------
	; The number of seconds PFC will wait for a response from
	; GLD before sending a default answer
	;--------------------------------------------------------*/
	
#define POSTFIX_TIMEOUT	5

	/*-------------------------------------------------------
	; Settings for syslog() (see notes for SERVER_LOG_LEVEL
	; for more information)
	;---------------------------------------------------------*/
	
#define POSTFIX_LOG_FACILITY	LOG_MAIL
#define POSTFIX_LOG_LEVEL	LOG_INFO
#define POSTFIX_LOG_ID		"gld-pfc"

/***********************************************************************
*
* Configuration parameters for SMC (the SENDMAIL CLIENT)
*
***********************************************************************/

	/*-------------------------------------------------------
	; Since SMC is an actual daemon, it records it PID in
	; a file.
	;-----------------------------------------------------*/
	
#define SENDMAIL_PIDFILE	"/var/run/smc.pid"

	/*--------------------------------------------------------
	; the pipe SMC uses to communicate to sendmail.
	;---------------------------------------------------------*/
	
#define SENDMAIL_FILTERCHANNEL	"unix:" SERVER_STATEDIR "/milter"

	/*----------------------------------------------------------
	; The local interface that SMC will communicate with.  To
	; use any interface, define as "0.0.0.0".  If you define
	; a specific interface, make sure that interface can 
	; talk to GLD.
	;---------------------------------------------------------*/
	
#define SENDMAIL_HOST		"0.0.0.0"

	/*--------------------------------------------------------
	; If SMC doesn't get a reply from GLD within this many seconds,
	; SMC will send back a default reply to sendmail.
	;------------------------------------------------------------*/
	
#define SENDMAIL_TIMEOUT	5

	/*-------------------------------------------------------
	; Settings for syslog() (see notes for SERVER_LOG_LEVEL
	; for more information)
	;---------------------------------------------------------*/

#define SENDMAIL_LOG_FACILITY	LOG_MAIL
#define SENDMAIL_LOG_LEVEL	LOG_INFO
#define SENDMAIL_LOG_ID		"gld-smc"

/************************************************************************
*
* Configuration parameters used by all programs.
*
*************************************************************************/

	/*---------------------------------------------------
	; The "secret" string, used to (laughingly) provide
	; some authentication to the communications protocol.
	; It's not meant to be very robust, just to keep random
	; crap from confusing the server and keeping the
	; really pathetic script kiddies from mucking things up.
	;------------------------------------------------------*/
	
#define SECRET		"decafbad"
#define SECRETSIZE	8

#endif

