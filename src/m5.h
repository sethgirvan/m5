#ifndef M5_H
#define M5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Each value should equal the named thruster's Motor ID (For easy compatibility with
 * PROPULSION_COMMAND).
 */
enum thruster
{
	// No motor ID 0, because with the firmwaree 0.9.8 that our thrusters came
	// with, setting motor ID to 0 seemed to really mess up communications,
	// probably due to a firmware bug.
	//
	// Since PROPULSION_COMMAND needs to output power values consecutively
	// starting from motor ID 0, we will just be outputting unspecified power
	// values for motor ID 0.
	VERT_FR = 1,
	VERT_FL = 2,
	VERT_BR = 3,
	VERT_BL = 4,
	SURGE_FR = 5,
	SURGE_FL = 6,
	SURGE_BR = 7,
	SURGE_BL = 8,
	NUM_THRUSTERS
};

/**
 * writes the next byte of the propulsion command packet to the m5's
 *
 * returns EOF on failure writing byte, 1 on packet completion, and zero on non
 * packet completion.
 */
int m5_power_trans();

/**
 * Sets thruster t to be set to power.
 * 
 * power must range [-1, 1]
 *
 * The new value will not actually be transmitted until m5_power_offer is
 * called.
 */
void m5_power(enum thruster t, float power);

/**
 * Sets the most recent power value set for each thruster with m5_power to be
 * transmitted as soon as the next set of thrust values begin transmission.
 */
void m5_power_offer();

#ifdef __cplusplus
}
#endif

#endif
