#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <libcli.h>

struct Debugger {
    void begin();
    void loop();
    void exec(char c);
    void halt(bool show = false);
    void usage() const;
    void printStatus();
};

extern struct Debugger Debugger;
extern libcli::Cli cli;

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
