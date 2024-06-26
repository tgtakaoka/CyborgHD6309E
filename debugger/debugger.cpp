/*
  The Controller can accept commands represented by one letter.

  R - reset MPU.
  d - dump memory. addr [length]
  m - write memory. addr byte...
  s - step one instruction.
  S - step one instruction with printing bus cycles.
  r - print MPU registers.
  = - set MPU register. register value
  G - Go, continuously run MPU.
  D - disassemble
  A - assemble
  U - upload S19 file
  F - list files in SD card
  L - load S-record file.
  ? - print version.
*/

#include "debugger.h"
#include <SD.h>
#include <string.h>
#include "config.h"
#include "pins.h"
#include "regs.h"

using State = libcli::Cli::State;
libcli::Cli cli;

#if ENABLE_ASM == 1
#define USAGE                                                      \
    F("R:eset r:egs =:setReg d:ump D:is m:emory A:sm s/S:tep G:o " \
      "U:pload F:iles L:oad")
#else
#define USAGE                                                 \
    F("R:eset r:egs =:setReg d:ump D:is m:emory s/S:tep G:o " \
      "U:pload F:iles L:oad")
#endif

struct Debugger Debugger;

static void commandHandler(char c, uintptr_t extra) {
    (void)extra;
    Debugger.exec(c);
}

static void printPrompt() {
    cli.print(F("> "));
    cli.readLetter(commandHandler, 0);
}

static uint16_t last_addr;
#define DUMP_ADDR 0
#define DUMP_LENGTH 1
#define ASM_ADDR 0
#define DIS_ADDR 0
#define DIS_LENGTH 1
#define INST_DATA(i) ((uintptr_t)i)

static uint8_t mem_buffer[16];
#define MEMORY_ADDR static_cast<uintptr_t>(sizeof(mem_buffer) + 10)
#define MEMORY_DATA(i) ((uintptr_t)(i))

static char str_buffer[40];

static void memoryDump(uint16_t addr, uint16_t len) {
    Regs.save();
    const auto start = addr;
    const auto end = addr + len;
    for (addr &= ~0xF; addr < end; addr += 16) {
        cli.printHex(addr, 4);
        cli.print(':');
        for (auto i = 0; i < 16; i++) {
            const auto a = addr + i;
            if (a < start || a >= end) {
                cli.print(F("   "));
            } else {
                const auto data = Regs.read(a);
                cli.print(' ');
                cli.printHex(data, 2);
            }
        }
        cli.print(' ');
        for (auto i = 0; i < 16; i++) {
            const auto a = addr + i;
            if (a < start || a >= end) {
                cli.print(' ');
            } else {
                const char data = Regs.read(a);
                if (isprint(data)) {
                    cli.print(data);
                } else {
                    cli.print('.');
                }
            }
        }
        cli.println();
    }
    Regs.restore();
}

static void handleDump(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DUMP_LENGTH) {
            cli.backspace();
            cli.readHex(handleDump, DUMP_ADDR, UINT16_MAX, last_addr);
        }
        return;
    }
    if (extra == DUMP_ADDR) {
        last_addr = value;
        if (state == State::CLI_SPACE) {
            cli.readHex(handleDump, DUMP_LENGTH, UINT16_MAX);
            return;
        }
        value = 16;
    }
    cli.println();
    memoryDump(last_addr, value);
    last_addr += value;
cancel:
    printPrompt();
}

static void handleDisassemble(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DIS_LENGTH) {
            cli.backspace();
            cli.readHex(handleDisassemble, DIS_ADDR, UINT16_MAX, last_addr);
        }
        return;
    }
    if (extra == DIS_ADDR) {
        last_addr = value;
        if (state == State::CLI_SPACE) {
            cli.readDec(handleDisassemble, DIS_LENGTH, UINT16_MAX);
            return;
        }
        value = 16;
    }
    cli.println();
    last_addr = Regs.disassemble(last_addr, value);
cancel:
    printPrompt();
}

static void handleMemory(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_ADDR)
            return;
        cli.backspace();
        uint16_t index = extra;
        if (index == 0) {
            cli.readHex(handleMemory, MEMORY_ADDR, UINT16_MAX, last_addr);
        } else {
            index--;
            cli.readHex(handleMemory, MEMORY_DATA(index), UINT8_MAX, mem_buffer[index]);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (extra == MEMORY_ADDR) {
            last_addr = value;
            cli.readHex(handleMemory, MEMORY_DATA(0), UINT8_MAX);
            return;
        }
        uint16_t index = extra;
        mem_buffer[index++] = value;
        if (state == State::CLI_SPACE) {
            if (index < sizeof(mem_buffer)) {
                cli.readHex(handleMemory, MEMORY_DATA(index), UINT8_MAX);
                return;
            }
        }
        cli.println();
        Regs.write(last_addr, mem_buffer, index);
        memoryDump(last_addr, index);
        last_addr += index;
    }
    printPrompt();
}

#if ENABLE_ASM == 1
static void handleAssembleLine(char *line, uintptr_t extra, State state) {
    (void)extra;
    if (state == State::CLI_CANCEL || *line == 0) {
        cli.println(F("end"));
        printPrompt();
        return;
    }
    cli.println();
    last_addr = Regs.assemble(last_addr, line);
    cli.printHex(last_addr, 4);
    cli.print('?');
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}

static void handleAssembler(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    if (state == State::CLI_CANCEL) {
        printPrompt();
        return;
    }
    last_addr = value;
    cli.println();
    cli.printHex(last_addr, 4);
    cli.print('?');
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}
#endif

static void listDirectory(File dir, const char *parent = nullptr) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        if (entry.isDirectory()) {
            listDirectory(entry, entry.name());
        } else {
            if (parent) {
                cli.print(parent);
                cli.print('/');
            }
            cli.printStr(entry.name(), -20);
            cli.printlnDec(entry.size(), 6);
        }
        entry.close();
    }
}

static void handleFileListing() {
    SD.begin(Pins.sdCardChipSelectPin());
    File root = SD.open("/");
    listDirectory(root);
    root.close();
}

static uint8_t toInt(const char c) {
    return isDigit(c) ? c - '0' : c - 'A' + 10;
}

static uint8_t toInt8Hex(const char *text) {
    return (toInt(text[0]) << 4) | toInt(text[1]);
}

static uint16_t toInt16Hex(const char *text) {
    return ((uint16_t)toInt8Hex(text) << 8) | toInt8Hex(text + 2);
}

static int loadS19Record(const char *line) {
    const int num = toInt8Hex(line + 2) - 3;
    uint16_t addr;
    switch (line[1]) {
    case '1':
        addr = toInt16Hex(line + 4);
        line += 8;
        break;
    default:
        return 0;
    }
    uint8_t buffer[num];
    for (int i = 0; i < num; i++) {
        buffer[i] = toInt8Hex(line + i * 2);
    }
    Regs.write(addr, buffer, num);
    cli.printHex(addr, 4);
    cli.print(':');
    cli.printHex(num, 2);
    cli.print(' ');
    return num;
}

static void handleLoadFile(char *line, uintptr_t extra, State state) {
    (void)extra;
    if (state != State::CLI_CANCEL && *line) {
        cli.println();
        SD.begin(Pins.sdCardChipSelectPin());
        File file = SD.open(line);
        if (!file) {
            cli.print(line);
            cli.println(F(" not found"));
        } else {
            uint16_t size = 0;
            char buffer[80];
            char *p = buffer;
            while (file.available() > 0) {
                const char c = file.read();
                if (c == '\n') {
                    *p = 0;
                    if (*buffer == 'S') {
                        size += loadS19Record(buffer);
                    }
                    p = buffer;
                } else if (c != '\r' && p < buffer + sizeof(buffer) - 1) {
                    *p++ = c;
                }
            }
            file.close();
            cli.println();
            cli.print(size);
            cli.println(F(" bytes loaded"));
        }
    }
    printPrompt();
}

struct UploadContext {
    uint32_t size;
    char buffer[80];
    uintptr_t extra() { return reinterpret_cast<uintptr_t>(this); }
    static UploadContext *context(uintptr_t extra) {
        return reinterpret_cast<UploadContext *>(extra);
    }
} upload_context;

void handleUploadFile(char *line, uintptr_t extra, State state) {
    UploadContext *context = UploadContext::context(extra);
    if (state == State::CLI_CANCEL) {
        cli.print(context->size);
        cli.println(F(" bytes uploaded"));
        printPrompt();
        return;
    }
    const auto c = context->buffer[0];
    if (c == 'S') {
        context->size += loadS19Record(context->buffer);
        cli.println();
    }
    cli.readLine(handleUploadFile, context->extra(), context->buffer, sizeof(context->buffer));
}

static void handleRegisterValue(uint32_t, uintptr_t, State);

static void handleSetRegister(char *word, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    char reg;
    if ((reg = Regs.validUint8Reg(word)) != 0) {
        cli.print('?');
        cli.readHex(handleRegisterValue, (uintptr_t)reg, UINT8_MAX);
        return;
    }
    if ((reg = Regs.validUint16Reg(word)) != 0) {
        cli.print('?');
        cli.readHex(handleRegisterValue, (uintptr_t)reg, UINT16_MAX);
        return;
    }
    if ((reg = Regs.validUint32Reg(word)) != 0) {
        cli.print('?');
        cli.readHex(handleRegisterValue, (uintptr_t)reg, UINT32_MAX);
        return;
    }
    Regs.printRegList();
    printPrompt();
}

static void handleRegisterValue(uint32_t value, uintptr_t reg, State state) {
    if (state == State::CLI_DELETE) {
        cli.backspace(2);  // ' ', '?'
        cli.readWord(handleSetRegister, 0, str_buffer, sizeof(str_buffer), true);
        return;
    }
    if (state != State::CLI_CANCEL) {
        Regs.setRegValue(reg, value);
        cli.println();
        Regs.print();
    }
    printPrompt();
}

void Debugger::exec(char c) {
    switch (c) {
    case 'R':
        cli.println(F("Reset"));
        Pins.reset(true);
        printStatus();
        break;
    case 'd':
        cli.print(F("Dump? "));
        cli.readHex(handleDump, DUMP_ADDR, UINT16_MAX);
        return;
    case 'D':
        cli.print(F("Disassemble? "));
        cli.readHex(handleDisassemble, DIS_ADDR, UINT16_MAX);
        return;
#if ENABLE_ASM == 1
    case 'A':
        cli.print(F("Assemble? "));
        cli.readHex(handleAssembler, ASM_ADDR, UINT16_MAX);
        return;
#endif
    case 'm':
        cli.print(F("Memory? "));
        cli.readHex(handleMemory, MEMORY_ADDR, UINT16_MAX);
        return;
    case 'r':
        cli.println(F("Registers"));
        printStatus();
        break;
    case '=':
        cli.print(F("Set register? "));
        cli.readWord(handleSetRegister, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'S':
    case 's':
        cli.println(F("Step"));
        Pins.step(c == 'S');
        printStatus();
        break;
    case 'G':
        cli.println(F("Go"));
        Pins.run();
        return;
    case 'U':
        cli.println(F("Upload waiting..."));
        upload_context.size = 0;
        cli.readLine(handleUploadFile, upload_context.extra(), upload_context.buffer,
                sizeof(upload_context.buffer));
        return;
    case 'F':
        cli.println(F("Files"));
        handleFileListing();
        break;
    case 'L':
        cli.print(F("Load? "));
        cli.readLine(handleLoadFile, 0, str_buffer, sizeof(str_buffer));
        return;
    case '?':
        usage();
        break;
    case '\r':
        cli.println();
    case '\n':
        break;
    default:
        return;
    }
    printPrompt();
}

void Debugger::halt(bool show) {
    Pins.halt(show);
    printStatus();
    printPrompt();
}

void Debugger::usage() const {
    cli.print(F("* Cyborg"));
    cli.print(Regs.cpuName());
    cli.println(F(" * " VERSION_TEXT));
    cli.println(USAGE);
}

void Debugger::printStatus() {
    Regs.get(true);
    Regs.disassemble(Regs.nextIp(), 1);
}

void Debugger::begin() {
    Pins.begin();
    usage();
    printStatus();
    printPrompt();
}

void Debugger::loop() {
    cli.loop();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
