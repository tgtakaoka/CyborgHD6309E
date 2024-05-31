#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <libcli.h>

struct Commands {
    void begin();
    void loop();
    void exec(char c);
    void halt(bool show = false);
};

extern Commands Commands;
extern libcli::Cli cli;

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
