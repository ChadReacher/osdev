#ifndef TERMIOS_H
#define TERMIOS_H

#include "types.h"

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;

#define NCCS 11
struct termios {
	tcflag_t c_iflag; /* input modes */
	tcflag_t c_oflag; /* output modes */
	tcflag_t c_cflag; /* control modes */
	tcflag_t c_lflag; /* local modes */
	cc_t c_cc[NCCS];
};

/* c_iflag values */
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IXON	0001000
#define IXOFF	0002000

/* c_oflag values */
#define OPOST	0000001
#define ONLCR	0000002

/* c_cflag values */
#define CBAUD	0000017
#define  B0		0000000
#define  B50	0000001
#define  B75	0000002
#define  B110	0000003
#define  B134	0000004
#define  B150	0000005
#define  B200	0000006
#define  B300	0000007
#define  B600	0000010
#define  B1200	0000011
#define  B1800	0000012
#define  B2400	0000013
#define  B4800	0000014
#define  B9600	0000015
#define  B19200 0000016
#define	 B38400	0000017
#define CSIZE	0000060
#define	  CS5	0000000
#define   CS6	0000020
#define   CS7	0000040
#define   CS8	0000060
#define CLOCAL	0000100
#define CREAD	0000200
#define CSTOPB	0000400
#define PARENB	0001000
#define PARODD	0002000
#define HUPCL	0004000

/* c_lflag values */
#define ISIG	0000001
#define ICANON	0000002
#define ECHO	0000004
#define ECHOE	0000010
#define ECHOK	0000020
#define ECHONL	0000040
#define NOFLSH	0000100
#define TOSTOP	0000200
#define IEXTEN	0000400

/* c_cc values */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSTART 7
#define VSUSP 8
#define VSTOP 9
#define VEOL 10

#endif
