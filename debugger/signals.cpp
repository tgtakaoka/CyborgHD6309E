#include "signals.h"
#include "debugger.h"
#include "digital_fast.h"
#include "string_util.h"

uint8_t Signals::_put = 0;
uint8_t Signals::_get = 0;
uint8_t Signals::_cycles = 0;
Signals Signals::_signals[MAX_CYCLES];

void Signals::get() {
    _pins = busRead(SIGNALS);
    data = busRead(DB);
}

void Signals::printCycles() {
    for (auto i = 0; i < _cycles; i++) {
        const auto idx = (_get + i) % MAX_CYCLES;
        _signals[idx].print();
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

void Signals::print() const {
    static char buffer[] = {
            'V',                  // ba/bs=0
            'A',                  // avma=1
            'B',                  // busy=2
            'L',                  // lic=3
            'W',                  // rw=4
            ' ', 'D', '=', 0, 0,  // _dbus=8
            0,                    // EOS
    };
    auto p = buffer;
    if (ba() == 0) {
        *p++ = bs() == 0 ? ' ' : 'V';
    } else {
        *p++ = bs() == 0 ? 'S' : 'H';
    }
    *p++ = avma() == 0 ? ' ' : 'A';
    *p++ = busy() == 0 ? ' ' : 'B';
    *p++ = lic() == 0 ? ' ' : 'L';
    *p++ = rw() == 0 ? 'W' : 'R';
    outHex8(buffer + 8, data);
    cli.println(buffer);
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
