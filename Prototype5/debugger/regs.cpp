#include "regs.h"

#include "pins.h"
#include "string_util.h"

extern libcli::Cli &cli;

struct Regs Regs;
struct Memory Memory;

/**
 * How to determine MC6809 variants.
 *
 *   CLRB
 *   FCB  $10, $43
 *        ; NOP  ($10) on MC6809
 *        ; COMA ($43) on MC6809
 *        ; COMD ($10,$43) on HD6309
 *   B=$00: MC6809
 *   B=$FF: HD6309
 */

static const char MC6809[] = "MC6809";
static const char HD6309[] = "HD6309";

void Regs::setCpuType() {
    static const uint8_t CLRB[] = {0x5F};
    static const uint8_t COMD[] = {0x10, 0x43};
    static const uint8_t STB[] = {0xD7, 0x00};

    Pins.execInst(CLRB, sizeof(CLRB));
    Pins.execInst(COMD, sizeof(COMD));
    uint8_t b;
    Pins.captureWrites(STB, sizeof(STB), &b, 1);
    _cpuType = b ? HD6309 : MC6809;
}

const char *Regs::cpu() const {
    return _cpuType;
}

void Regs::reset() {
    _cpuType = nullptr;
}

bool Regs::is6309() const {
    return _cpuType == HD6309;
}

static char bit1(uint8_t v, char name) {
    return v ? name : '_';
}

void Regs::print() const {
    // clang-format off
    static char buffer[] = {
        'D', 'P', '=', 0, 0, ' ',  // dp=3
        'P', 'C', '=', 0, 0, 0, 0, // pc=9
        ' ', 'S', '=', 0, 0, 0, 0, // s=16
        ' ', 'U', '=', 0, 0, 0, 0, // u=23
        ' ', 'Y', '=', 0, 0, 0, 0, // y=30
        ' ', 'X', '=', 0, 0, 0, 0, // x=37
        ' ', 'A', '=', 0, 0,       // A=44
        ' ', 'B', '=', 0, 0,       // B=49
        ' ',                       // 6309=51
        'W', '=', 0, 0, 0, 0,      // w=54
        ' ', 'V', '=', 0, 0, 0, 0, // v=61
        0,                         // EOS
    };
    // clang-format on
    outHex8(buffer + 3, dp);
    outHex16(buffer + 9, pc);
    outHex16(buffer + 16, s);
    outHex16(buffer + 23, u);
    outHex16(buffer + 30, y);
    outHex16(buffer + 37, x);
    outHex8(buffer + 44, a);
    outHex8(buffer + 49, b);
    if (is6309()) {
        buffer[51] = ' ';
        outHex16(buffer + 54, getW());
        outHex16(buffer + 61, v);
    } else {
        buffer[51] = 0;
    }
    // clang-format off
    static char cc_bits[] = {
        ' ', 'C', 'C', '=',
        0, 0, 0, 0, 0, 0, 0, 0,
        0,
    };
    // clang-format on
    char *p = cc_bits + 4;
    *p++ = bit1(cc & 0x80, 'E');
    *p++ = bit1(cc & 0x40, 'F');
    *p++ = bit1(cc & 0x20, 'H');
    *p++ = bit1(cc & 0x10, 'I');
    *p++ = bit1(cc & 0x08, 'N');
    *p++ = bit1(cc & 0x04, 'Z');
    *p++ = bit1(cc & 0x02, 'V');
    *p = bit1(cc & 0x01, 'C');
    cli.print(buffer);
    cli.println(cc_bits);
}

void Regs::get(bool show) {
    save();
    restore();
    if (show)
        print();
}

static constexpr uint16_t uint16(const uint8_t hi, const uint8_t lo) {
    return (static_cast<uint16_t>(hi) << 8) | lo;
}
static constexpr uint16_t le16(const uint8_t *p) {
    return uint16(p[1], p[0]);
}
static constexpr uint16_t be16(const uint8_t *p) {
    return uint16(p[0], p[1]);
}
static constexpr uint8_t hi(const uint16_t v) {
    return static_cast<uint8_t>(v >> 8);
}
static constexpr uint8_t lo(const uint16_t v) {
    return static_cast<uint8_t>(v);
}

void Regs::save() {
    static const uint8_t PSHS_ALL[] = {0x34, 0xFF};  // PSHS PC,U,Y,X,DP,B,A,CC
    static const uint8_t PSHU_S[] = {0x36, 0x40};    // PSHU S
    static uint8_t buffer[14];

    Pins.captureWrites(PSHS_ALL, sizeof(PSHS_ALL), buffer, 12);
    if (_cpuType == nullptr)
        setCpuType();
    Pins.captureWrites(PSHU_S, sizeof(PSHU_S), buffer + 12, 2);
    if (is6309())
        save6309();
    // capture writes to stack in reverse order.
    pc = le16(buffer + 0) - 2;  // offset PSHS instruction.
    u = le16(buffer + 2);
    y = le16(buffer + 4);
    x = le16(buffer + 6);
    dp = buffer[8];
    b = buffer[9];
    a = buffer[10];
    cc = buffer[11];
    s = le16(buffer + 12) + 12;  // offset PSHS stack frame.
}

void Regs::restore() {
    static uint8_t LDS[] = {0x10, 0xCE, 0, 0};      // LDS #s-12
    static uint8_t PULS_ALL[2 + 12] = {0x35, 0xFF}; // PULS CC,A,B,DP,X,Y,U,PC

    const uint16_t sp = s - 12;
    LDS[2] = hi(sp);
    LDS[3] = lo(sp);
    PULS_ALL[2] = cc;
    PULS_ALL[3] = a;
    PULS_ALL[4] = b;
    PULS_ALL[5] = dp;
    PULS_ALL[6] = hi(x);
    PULS_ALL[7] = lo(x);
    PULS_ALL[8] = hi(y);
    PULS_ALL[9] = lo(y);
    PULS_ALL[10] = hi(u);
    PULS_ALL[11] = lo(u);
    PULS_ALL[12] = hi(pc);
    PULS_ALL[13] = lo(pc);

    if (is6309())
        restore6309();
    Pins.execInst(LDS, sizeof(LDS));
    Pins.execInst(PULS_ALL, sizeof(PULS_ALL));
}

void Regs::save6309() {
    static const uint8_t TFR_WY[] = {0x1F, 0x62};    // TFR W,Y
    static const uint8_t TFR_VX[] = {0x1F, 0x71};    // TFR V,X
    static const uint8_t PSHU_YX[] = {0x36, 0x30};   // PSHU Y,X
    static uint8_t buffer[4];

    Pins.execInst(TFR_WY, sizeof(TFR_WY));
    Pins.execInst(TFR_VX, sizeof(TFR_VX));
    Pins.captureWrites(PSHU_YX, sizeof(PSHU_YX), buffer, 4);
    f = buffer[0];
    e = buffer[1];
    v = le16(buffer + 2);
}

void Regs::restore6309() {
    static uint8_t LDD[] = {0xCC, 0, 0};          // LDD #v
    static const uint8_t TFR_DV[] = {0x1F, 0x07}; // TFR D,V
    static uint8_t LDW[] = {0x10, 0x86, 0, 0};    // LDW #w

    LDD[1] = hi(v);
    LDD[2] = lo(v);
    LDW[2] = e;
    LDW[3] = f;
    Pins.execInst(LDD, sizeof(LDD));
    Pins.execInst(TFR_DV, sizeof(TFR_DV));
    Pins.execInst(LDW, sizeof(LDW));
}

void Regs::printRegList() const {
    cli.print(F("?Reg: pc s u x y a b d"));
    if (is6309())
        cli.print(F(" w e f v"));
    cli.println(F(" DP cc"));
}

bool Regs::validUint8Reg(char reg) const {
    if (reg == 'D' || reg == 'a' || reg == 'b' || reg == 'c' ||
            (is6309() && (reg == 'e' || reg == 'f'))) {
        cli.print(reg);
        if (reg == 'D')
            cli.print('P');
        if (reg == 'c')
            cli.print('c');
        return true;
    }
    return false;
}

bool Regs::validUint16Reg(char reg) const {
    if (reg == 'p' || reg == 's' || reg == 'u' || reg == 'y' || reg == 'x' ||
            reg == 'd' || (is6309() && (reg == 'w' || reg == 'v'))) {
        cli.print(reg);
        if (reg == 'p')
            cli.print('c');
        return true;
    }
    return false;
}

bool Regs::setRegValue(char reg, uint32_t value, State state) {
    if (state == State::CLI_CANCEL)
        return true;
    if (state == State::CLI_DELETE) {
        cli.backspace(reg == 'p' || reg == 'D' || reg == 'c' ? 3 : 2);
        return false;
    }
    switch (reg) {
    case 'p':
        pc = value;
        break;
    case 's':
        s = value;
        break;
    case 'u':
        u = value;
        break;
    case 'y':
        y = value;
        break;
    case 'x':
        x = value;
        break;
    case 'd':
        setD(value);
        break;
    case 'w':
        setW(value);
        break;
    case 'v':
        v = value;
        break;
    case 'a':
        a = value;
        break;
    case 'b':
        b = value;
        break;
    case 'e':
        e = value;
        break;
    case 'f':
        f = value;
        break;
    case 'D':
        dp = value;
        break;
    case 'c':
        cc = value;
        break;
    }
    restore();
    print();
    return true;
}

uint8_t Memory::read(uint16_t addr) const {
    static uint8_t LDA[] = {0xB6, 0, 0}; // LDA $addr
    LDA[1] = hi(addr);
    LDA[2] = lo(addr);
    uint8_t data;
    Pins.captureReads(LDA, sizeof(LDA), &data, 1);
    return data;
}

void Memory::write(uint16_t addr, uint8_t data) {
    static uint8_t LDA[] = {0x86, 0};    // LDA #data
    static uint8_t STA[] = {0xB7, 0, 0}; // STA $addr
    LDA[1] = data;
    STA[1] = hi(addr);
    STA[2] = lo(addr);
    Pins.execInst(LDA, sizeof(LDA));
    Pins.execInst(STA, sizeof(STA));
}

uint8_t Memory::nextByte() {
    Regs.save();
    const uint8_t data = read(address());
    Regs.restore();
    return data;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
