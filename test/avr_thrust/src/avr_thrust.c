#include <ctype.h>

#include "m5.h"
#include "io_m5.h"
#include "io.h"
#include "dbg.h"


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
	(void)argc, (void)argv;
	io_init();
	io_stdin_init();

	io_m5_init(NULL);

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
