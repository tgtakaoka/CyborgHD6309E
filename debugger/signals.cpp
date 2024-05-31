#include "signals.h"
#include "commands.h"
#include "digital_fast.h"
#include "string_util.h"

uint8_t Signals::_put = 0;
uint8_t Signals::_get = 0;
uint8_t Signals::_cycles = 0;
Signals Signals::_signals[MAX_CYCLES];

void Signals::printCycles() {
    const Signals *prev = nullptr;
    for (auto i = 0; i < _cycles; i++) {
        const auto idx = (_get + i) % MAX_CYCLES;
        _signals[idx].print(prev);
        prev = &_signals[idx];
    }
}

Signals &Signals::currCycle() {
    return _signals[_put];
}

void Signals::resetCycles() {
    _cycles = 0;
    _signals[_get = _put];
}

void Signals::nextCycle() {
    _put++;
    _put %= MAX_CYCLES;
    if (_cycles < MAX_CYCLES) {
        _cycles++;
    } else {
        _get++;
        _get %= MAX_CYCLES;
    }
}

void Signals::get() {
    _pins = busRead(SIGNALS);
    _dbus = busRead(DB);
}

void Signals::print(const Signals *prev) const {
    static char buffer[] = {
            'V',                  // ba/bs=0
            'A',                  // avma=1
            'B',                  // busy=2
            'L',                  // lic=3
            'W',                  // rw=4
            ' ', 'D', '=', 0, 0,  // _dbus=8
            ' ',                  // status?=10
            'S', 0                // status=11
    };
    auto p = buffer;
    if ((_pins & ba) == 0) {
        *p++ = (_pins & bs) == 0 ? ' ' : 'V';
    } else {
        *p++ = (_pins & bs) == 0 ? 'S' : 'H';
    }
    *p++ = (_pins & avma) == 0 ? ' ' : 'A';
    *p++ = (_pins & busy) == 0 ? ' ' : 'B';
    *p++ = (_pins & lic) == 0 ? ' ' : 'L';
    *p++ = (_pins & rw) == 0 ? 'W' : 'R';
    outHex8(buffer + 8, _dbus);
    if (prev) {
        char status;
        if (fetchingVector()) {
            status = 'V';
        } else if (running()) {
            if (writeCycle(prev)) {
                status = 'W';
            } else if (readCycle(prev)) {
                status = 'R';
            } else {
                status = '-';
            }
        } else if (halting()) {
            status = 'H';
        } else {
            status = 'S';
        }
        buffer[10] = ' ';
        buffer[11] = status;
    } else {
        buffer[10] = 0;
    }
    cli.println(buffer);
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
