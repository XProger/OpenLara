#ifndef H_CMDLINE
#define H_CMDLINE

#include <string>
#include <stdlib.h>

int argLevelName(int argc, char** argv) {
    std::string arg;

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];

        if ((arg.size() >= 2) && arg[0] == '-' && arg[1] == '-') continue;
        return i;
    }

    return 0;
}

bool argWindowSize(int argc, char** argv, unsigned int* w, unsigned int* h) {
    std::string arg = "";
    std::string sub = "";

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];

        if (arg.size() > 14 && arg.substr(0, 14) == "--window-size=") {
            sub = arg.substr(arg.find('=') + 1, arg.size());
            break;
        }
    }

    if (sub.empty()) return false;

    size_t pos;
    pos = sub.find("x");
    if (pos == sub.npos) return false;
    if (pos + 1 >= sub.size()) return false;

    std::string w_str = sub.substr(0, pos);
    std::string h_str = sub.substr(pos + 1, sub.size());

    unsigned int w_tmp = atoi(w_str.c_str());
    unsigned int h_tmp = atoi(h_str.c_str());
    if (w_tmp == 0 || h_tmp == 0) return false;

    if (w != 0) *w = w_tmp;
    if (h != 0) *h = h_tmp;

    return true;
}

bool argFullscreen(int argc, char** argv) {
    std::string arg;

    for (int i = 1; i < argc; ++i) {
        arg = argv[i];

        if (arg == "--fullscreen") return true;
    }

    return false;
}

#endif
