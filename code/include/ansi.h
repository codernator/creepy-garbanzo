#ifndef _ANSI_
#define _ANSI_
/* ANSI control sequences, to be include */
#define ESCAPE      ""
#define ANSI_NORMAL "[0m"
#define ANSI_BOLD   "[0;1m"
#define ANSI_UNDERL "[0;4m"
#define ANSI_BLINK  "[0;5m"
#define ANSI_INVERS "[0;7m"

#define ANSI_BLACK  "[0;30m"
#define ANSI_RED    "[0;31m"
#define ANSI_GREEN  "[0;32m"
#define ANSI_YELLOW "[0;33m"
#define ANSI_BLUE   "[0;34m"
#define ANSI_PURPLE "[0;35m"
#define ANSI_CYAN   "[0;36m"
#define ANSI_WHITE  "[0;37m"
#define ANSI_BOLD_RED    "[0;1;31m"
#define ANSI_BOLD_GREEN  "[0;1;32m"
#define ANSI_BOLD_YELLOW "[0;1;33m"
#define ANSI_BOLD_BLUE   "[0;1;34m"
#define ANSI_BOLD_PURPLE "[0;1;35m"
#define ANSI_BOLD_CYAN   "[0;1;36m"
#define ANSI_BOLD_WHITE  "[0;1;37m"

#define ANSI_CLS    "[2J"
#define ANSI_HOME   "[1;1H"
#endif

char *color_table[] =
{
	"[0;31m",              /* 0x0 */
	"[0;32m",
	"[0;33m",
	"[0;34m",
	"[0;35m",
	"[0;36m",              /*0x5 */
	"[0;37m",              /*0x6 white */
	"[0;1;30m",
	"[0;1;31m",
	"[0;1;32m",
	"[0;1;33m",
	"[0;1;34m",            /*0xb */
	"[0;1;35m",
	"[0;1;36m",
	"[0;1;37m",            /*0xe */
	"[0;37m",              /*0x6 white */
	0
};
