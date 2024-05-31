#ifndef __SIGNALS_H__
#define __SIGNALS_H__

#include <stdint.h>
#include "config.h"

#ifndef _BV
#define _BV(n) (1 << (n))
#endif

struct Signals {
    void get();
    void print(const Signals *prev = nullptr) const;
    uint8_t dbus() const { return _dbus; }
    bool running() const { return babs() == 0; }
    bool fetchingVector() const { return babs() == bs; }
    bool halting() const { return babs() == (ba | bs); }
    bool lastInstructionCycle() const { return _pins & lic; }
    bool advancedValidMemoryAddress() const { return _pins & avma; }
    bool readCycle(const Signals *prev) const {
        return prev && prev->advancedValidMemoryAddress() && (_pins & rw);
    }
    bool writeCycle(const Signals *prev) const {
        return prev && prev->advancedValidMemoryAddress() && (_pins & rw) == 0;
    }

    static void printCycles();
    static Signals &currCycle();
    static void resetCycles();
    static void nextCycle();

private:

    static constexpr uint8_t bs = _BV(BS_PIN);
    static constexpr uint8_t ba = _BV(BA_PIN);
    static constexpr uint8_t reset = _BV(RESET_PIN);
    static constexpr uint8_t halt = _BV(HALT_PIN);
    static constexpr uint8_t lic = _BV(LIC_PIN);
    static constexpr uint8_t avma = _BV(AVMA_PIN);
    static constexpr uint8_t rw = _BV(RW_PIN);
    static constexpr uint8_t busy = _BV(BUSY_PIN);
    uint8_t babs() const { return _pins & (ba | bs); }

    uint8_t _pins;
    uint8_t _dbus;

    static constexpr auto MAX_CYCLES = 64;
    static uint8_t _put;
    static uint8_t _get;
    static uint8_t _cycles;
    static Signals _signals[MAX_CYCLES];
};

#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
