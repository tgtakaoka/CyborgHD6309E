/**
 *  CyborgHD6309E controller
 *
 * This sketch is designed to controll and debug MC6809E/HD6309E MPU.
 */

#include "commands.h"
#include "pins.h"

void setup() {
    Console.begin(CONSOLE_BAUD);
    cli.begin(Console);
    Pins.begin();
    Commands.begin();
    interrupts();
}

void loop() {
    Commands.loop();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
