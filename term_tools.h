//
// Created by gregorio on 4/10/25.
//

#ifndef TERM_TOOLS_H
#define TERM_TOOLS_H

#include <stddef.h>
#include <assert.h>

#ifdef __linux__
#include <sys/ioctl.h>
#elif __WIN32__
#include <windows.h>
#endif

// Terminal Colors
#define TERM_RED "\x1b[31m"
#define TERM_GREEN "\x1b[32m"
#define TERM_YELLOW "\x1b[33m"
#define TERM_BLUE "\x1b[34m"
#define TERM_MAGENTA "\x1b[35m"
#define TERM_CYAN "\x1b[36m"
#define TERM_WHITE "\x1b[37m"

// Terminal Bold Colors
#define TERM_RED_BOLD "\x1b[1;31m"
#define TERM_GREEN_BOLD "\x1b[1;32m"
#define TERM_YELLOW_BOLD "\x1b[1;33m"
#define TERM_BLUE_BOLD "\x1b[1;34m"
#define TERM_MAGENTA_BOLD "\x1b[1;35m"
#define TERM_CYAN_BOLD "\x1b[1;36m"
#define TERM_WHITE_BOLD "\x1b[1;37m"

// Terminal Italic Colors
#define TERM_ITALIC "\x1b[3m"
#define TERM_RED_ITALIC "\x1b[3;31m"
#define TERM_GREEN_ITALIC "\x1b[3;32m"
#define TERM_YELLOW_ITALIC "\x1b[3;33m"
#define TERM_BLUE_ITALIC "\x1b[3;34m"
#define TERM_MAGENTA_ITALIC "\x1b[3;35m"
#define TERM_CYAN_ITALIC "\x1b[3;36m"
#define TERM_WHITE_ITALIC "\x1b[3;37m"

// Terminal Reset
#define TERM_RESET "\x1b[0m"


static inline size_t getTerminalRows() {
    #ifdef __linux__
        struct winsize _;
        ioctl(0, TIOCGWINSZ, &_);
        return _.ws_row;
    #elif defined(_WIN32)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return rows;
    #endif
}

static inline size_t getTerminalCols() {
    #ifdef __linux__
        struct winsize _;
        ioctl(0, TIOCGWINSZ, &_);
        return _.ws_col;
    #elif defined(_WIN32)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        return cols;
    #endif
}

#endif //TERM_TOOLS_H
