#ifndef __SIGNALS_H__
#define __SIGNALS_H__

#include <stdint.h>
#include "config.h"

namespace {}  // namespace

struct Signals {
    void get();
    void print() const;
    uint8_t rw() const { return _pins & _bv(RW_PIN); }
    uint8_t ba() const { return _pins & _bv(BA_PIN); }
    uint8_t bs() const { return _pins & _bv(BS_PIN); }
    uint8_t lic() const { return _pins & _bv(LIC_PIN); }
    uint8_t avma() const { return _pins & _bv(AVMA_PIN); }
    uint8_t busy() const { return _pins & _bv(BUSY_PIN); }
    bool run() const { return ba() == 0 && bs() == 0; }
    bool vector() const { return ba() == 0 && bs(); }
    bool halt() const { return ba() && bs(); }
    bool validRead(const Signals *prev) const {
        return prev && prev->avma() && rw();
    }
    bool validWrite(const Signals *prev) const {
        return prev && prev->avma() && rw() == 0;
    }

    uint8_t data;

    static void printCycles();
    static Signals &currCycle();
    static void resetCycles();
    static void nextCycle();

private:
    uint8_t _pins;

    static constexpr uint8_t _bv(uint8_t pin) { return 1 << pin; }

    static constexpr auto MAX_CYCLES = 32;
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
