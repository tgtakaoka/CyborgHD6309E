#ifndef __REGS_H__
#define __REGS_H__

#include <stdint.h>

struct Regs {
    uint16_t s;
    uint16_t pc;
    uint16_t u;
    uint16_t y;
    uint16_t x;
    uint8_t dp;
    uint8_t a;
    uint8_t b;
    uint16_t getD() const { return (static_cast<uint16_t>(a) << 8) | b; }
    void setD(uint16_t d) {
        b = d;
        a = d >> 8;
    }
    uint8_t cc;
    uint8_t e;
    uint8_t f;
    uint16_t getW() const { return (static_cast<uint16_t>(e) << 8) | f; }
    void setW(uint16_t w) {
        f = w;
        e = w >> 8;
    }
    uint32_t getQ() const { return (static_cast<uint32_t>(getD()) << 16) | getW(); }
    void setQ(uint32_t q) {
        setW(q);
        setD(q >> 16);
    }
    uint16_t v;

    void print() const;
    void get(bool show = false);
    void save();
    void restore();
    void reset();

    const char *cpu() const;
    const char *cpuName() const;
    bool is6309() const;
    bool native6309() const { return _native6309; }

    uint16_t nextIp() const { return pc; }
    uint32_t maxAddr() const { return UINT16_MAX; }
    void printRegList() const;
    char validUint8Reg(const char *word) const;
    char validUint16Reg(const char *word) const;
    char validUint32Reg(const char *word) const;
    void setRegValue(char reg, uint32_t value);

    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data);
    void write(uint16_t addr, const uint8_t *data, uint8_t len);
#if ENABLE_ASM == 1
    uint16_t assemble(uint16_t addr, const char *line) const;
#endif
    uint16_t disassemble(uint16_t addr, uint16_t numInsn) const;

private:
    const char *_cpuType;
    bool _native6309;

    void setCpuType();
    void save6309();
    void restore6309();
};

extern struct Regs Regs;

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
