/**
 *  CyborgHD6309E controller
 *
 * This sketch is designed to controll and debug MC6809E/HD6309E MPU.
 */

#include "config.h"
#include "debugger.h"

void setup() {
    Console.begin(CONSOLE_BAUD);
    while (!Console)
        yield();
    cli.begin(Console);
    Debugger.begin();
    interrupts();
}

void loop() {
    Debugger.loop();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
