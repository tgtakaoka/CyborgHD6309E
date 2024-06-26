/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; -*- */
#ifndef __PINS_H__
#define __PINS_H__

#include <stdint.h>

#include "config.h"
#include "signals.h"

struct Pins {
    void begin();

    void reset(bool show = false);
    void halt(bool show = false);
    void step(bool show = false);
    void run();
    void stopRunning();
    void execInst(const uint8_t *inst, uint8_t len);
    void captureReads(const uint8_t *inst, uint8_t len, uint8_t *capBuf, uint8_t max);
    void captureWrites(const uint8_t *inst, uint8_t len, uint8_t *capBuf, uint8_t max);

    void acknowledgeIoRequest();
    uint16_t ioRequestAddress() const;
    bool ioRequestWrite() const;
    uint8_t ioGetData();
    void ioSetData(uint8_t data);

    static constexpr uint16_t ioBaseAddress() { return IO_BASE_ADDR; }

    void assertIrq() const;
    void negateIrq() const;

    int sdCardChipSelectPin() const;

private:
    class Dbus {
    public:
        void begin();
        void set(uint8_t data);
        void output();
        void input();
        bool valid() const { return _valid; }
        void capture(bool enable);

    private:
        void setDbus(uint8_t dir, uint8_t data);
        uint8_t _data;
        bool _valid;
        bool _capture;
    };

    void loop();
    Signals &cycle(const Signals *prev);
    const Signals *unhalt();
    void setData(uint8_t data);
    void execute(
            const uint8_t *inst, uint8_t len, uint8_t *capWrite, uint8_t max, uint8_t *capRead);

    Dbus _dbus;
};

extern struct Pins Pins;

#endif /* __PINS_H__ */
