/**
 * Allocates a pseudoterminal, prints the slave file name to STDERR, and writes
 * data read from STDIN to the master end and writes data read from the master
 * end to STDOUT.
 *
 * Purpose: The VideoRay utility programs like 'vr_refresh' and 'vr_csr_rw'
 * expect to be communicating with the thrusters over a com port (eg
 * /dev/ttyUSB0), which would be something like a USB<-> RS-485 converter. It
 * can be useful to see what these programs are communicating (however, you can
 * get vr_csr_rw to dump its packets in hex with --dump arg), so by allocating
 * a pseudoterminal, we can emulate the com port these programs expect.
 */
#define _XOPEN_SOURCE 600

#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdnoreturn.h>


int set_term_raw(int fd, int baud, int parity)
{
	struct termios tty;
	if (tcgetattr(fd, &tty))
	{
		// error from tcgetattr
		return -1;
	}

	cfsetospeed(&tty, baud);
	cfsetispeed(&tty, baud);

	tty.c_iflag &= ~(
			IGNBRK | // BREAK condition on input
			BRKINT | // SIGINT controlling process on BREAK condition
			PARMRK | // mark bytes with parity or framing errors
			ISTRIP | // strip off the eight bit
			INLCR | // translate NL to CR on input
			IGNCR | // ignore CR on input
			IXON ); // enable XON/XOFF flow control on output

	tty.c_lflag &= ~(
			ECHO | // echo input characters
			ECHONL | // echo newline character
			ICANON | // canonical mode
			ISIG | // generate signals for control character
			IEXTEN); // implementation-defined input processing

	tty.c_oflag &= ~OPOST; // implementation-defined output processing
	tty.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	tty.c_cflag |= CS8 | parity; // 8 data bits

	tty.c_cc[VMIN] = 1; // read blocks until at least one char
	tty.c_cc[VTIME] = 0; // no read timeout

	if (tcsetattr(fd, TCSANOW, &tty))
	{
		// error from tcsetattr
		return -1;
	}
	return 0;
}

void stdin_to_ptm(int fd)
{
	for (char c; fread(&c, 1, 1, stdin) == 1;)
	{
		if (write(fd, &c, 1) != 1)
		{
			return;
		}
	}
	fprintf(stderr, "Closing stdin_to_ptm.\n");
}

noreturn void *ptm_to_stdout(void *arg)
{
	fprintf(stderr, "Starting ptm_to_stdout\n");
	for (char c;;)
	{
		if (read(*(int *)arg, &c, 1) == 1)
		{
			fwrite(&c, 1, 1, stdout);
			fflush(stdout);
		}
	}
}

int main(int argc, char *argv[])
{
	(void)argc, (void)argv;
	int masterfd = posix_openpt(O_RDWR | O_NOCTTY);
	grantpt(masterfd);
	unlockpt(masterfd);

	if (set_term_raw(masterfd, B115200, 0))
	{
		return -1;
	}
	fprintf(stderr, "%s\n", ptsname(masterfd));

	pthread_t thread_ptm_to_stdout;
	pthread_create(&thread_ptm_to_stdout, NULL, ptm_to_stdout, &masterfd);

	stdin_to_ptm(masterfd);

	return 0;
}
