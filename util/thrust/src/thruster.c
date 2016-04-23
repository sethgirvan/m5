#include <ctype.h>

#include "m5.h"
#include "dbg.h"
#include "io_m5.h"
#include "macrodef.h"


static void setpowers(float vals[NUM_THRUSTERS])
{
	for (enum thruster t = NUM_THRUSTERS; t--;)
	{
		// zero powers
		m5_power(t, vals[t]);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Please pass the name of the file acting as the m5's on the command line.\n");
		return -1;
	}

	io_m5_init(argv[1]);
	if (!io_m5)
	{
		DEBUG("Failed opening %s\n", argv[1]);
	}

	// zero powers
	setpowers((float [NUM_THRUSTERS]){0.f});
	m5_power_offer();

	io_m5_trans_start(m5_power_trans);
	for (float powers[NUM_THRUSTERS] = {0.f};;)
	{
		unsigned int t;
		float power;
		int ret = scanf("%u: %f", &t, &power);
		if (ret == EOF)
		{
			return 0;
		}
		else if (ret == 2)
		{
			if (t < NUM_THRUSTERS)
			{
				powers[t] = power;
				setpowers(powers);
				m5_power_offer();
			}
			else
			{
				DEBUG("Thruster number out of range");
			}
		}
		else
		{
			DEBUG("Input formatted incorrectly");
			int c;
			do
			{
				c = getchar();
			} while (!isdigit(c));
			ungetc(c, stdin);
		}
	}
	return 0;
}
