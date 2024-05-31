#include "mc6850.h"
#include <libcli.h>
#include "config.h"
#include "pins.h"

void Mc6850::reset() {
    negateRxIntr();
    negateTxIntr();
    _control = CDS_DIV1_gc;
    _status = TDRE_bm;
}

void Mc6850::enable(bool enabled, uint16_t baseAddr) {
    _enabled = enabled;
    _baseAddr = baseAddr & ~1;
    reset();
}

void Mc6850::negateRxIntr() {
    if ((_irq &= ~rxIrq) == 0)
        negateIntreq();
}

void Mc6850::negateTxIntr() {
    if ((_irq &= ~txIrq) == 0)
        negateIntreq();
}

void Mc6850::assertRxIntr() {
    _irq |= rxIrq;
    assertIntreq();
}

void Mc6850::assertTxIntr() {
    _irq |= txIrq;
    assertIntreq();
}

void Mc6850::loop() {
    if (!_enabled)
        return;
    if (Console.available() > 0) {
        _rxData = Console.read();
        if (rxReady())
            _status |= OVRN_bm;
        _status |= RDRF_bm;
        if (rxIntEnabled())
            assertRxIntr();
    }
    // TODO: Implement flow control
    if (Console.availableForWrite() > 0 && !txReady()) {
        Console.write(_txData);
        _status |= TDRE_bm;
        if (txIntEnabled())
            assertTxIntr();
    }
}

void Mc6850::assertIntreq() {
    _status |= IRQF_bm;
    Pins.assertIrq();
}

void Mc6850::negateIntreq() {
    _status &= ~IRQF_bm;
    Pins.negateIrq();
}

void Mc6850::write(uint8_t data, uint16_t addr) {
    if (addr == _baseAddr) {
        _control = data;
        if (masterReset()) {
            negateRxIntr();
            negateTxIntr();
            _status = TDRE_bm;
        }
        // TODO: flow control
        if (txReady() && txIntEnabled()) {
            assertTxIntr();
        } else {
            negateTxIntr();
        }
        if (rxReady() && rxIntEnabled()) {
            assertRxIntr();
        } else {
            negateRxIntr();
        }
    } else {
        _txData = data;
        _status &= ~TDRE_bm;
        negateTxIntr();
    }
}

uint8_t Mc6850::read(uint16_t addr) {
    if (addr == _baseAddr)
        return _status;
    _status &= ~(RDRF_bm | OVRN_bm);
    negateRxIntr();
    return _rxData;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
