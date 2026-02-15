
#ifndef CONSOLE_SETUP_HPP
#define CONSOLE_SETUP_HPP

#include <iostream>

#ifdef _WIN32
    #pragma execution_character_set( "utf-8" )
    
    #include <windows.h>
    #include <conio.h>
    #include <io.h>
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

// --- Global State for restoring terminal ---
#ifdef _WIN32
    HANDLE hStdin;
    DWORD originalInMode;
    HANDLE hStdout;
    DWORD originalOutMode;
#else
    struct termios orig_termios;
#endif


// --- Cross-Platform System Functions ---

void disableRawMode() {
    std::cout << "\033[?1003l\033[?1006l\033[?25h"; // Disable mouse tracking, show cursor
    
    #ifdef _WIN32
        SetConsoleMode(hStdin, originalInMode);
        SetConsoleMode(hStdout, originalOutMode);
    #else
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    #endif
}

void enableRawMode() {
    atexit(disableRawMode);

    #ifdef _WIN32
        hStdin = GetStdHandle(STD_INPUT_HANDLE);
        hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(hStdin, &originalInMode);
        GetConsoleMode(hStdout, &originalOutMode);
        SetConsoleOutputCP( 65001 ); // CP_UTF8
    
        DWORD newInMode = originalInMode;
        // Disable generic formatting
        newInMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        // Enable virtual terminal input (needed for mouse input on newer Windows)
        newInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT; 
        SetConsoleMode(hStdin, newInMode);

        DWORD newOutMode = originalOutMode;
        // ENABLE_VIRTUAL_TERMINAL_PROCESSING is required for ANSI escape codes (\033...)
        newOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; 
        SetConsoleMode(hStdout, newOutMode);
    #else
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0; 
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    #endif

    // Common ANSI setup: Mouse tracking and Hide Cursor
    std::cout << "\033[?1002h\033[?1006h\033[?25l" << std::flush;
}

void getWindowSize(size_t &width, size_t &height) {
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    #else
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        width = ws.ws_col;
        height = ws.ws_row;
    #endif
}

void sleepMs(int ms) {
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
}

// Wrapper to write buffer to stdout
void writeBuffer(const std::string& data) {
    #ifdef _WIN32
        _write(STDOUT_FILENO, data.c_str(), (unsigned int)data.length());
    #else
        write(STDOUT_FILENO, data.c_str(), data.length());
    #endif
}

// Wrapper for non-blocking read
// Returns number of bytes read
int readInput(char* buf, int max_size) {
    #ifdef _WIN32
        // Windows non-blocking check
        if (_kbhit()) {
            // Read one char (note: _read on stdin is usually blocking on Windows)
            // For a robust game engine, you'd use ReadConsoleInput, 
            // but for compatibility with this snippet structure:
            int i = 0;
            while (_kbhit() && i < max_size) {
                 buf[i] = _getch();
                 i++;
            }
            return i;
        }
        return 0;
    #else
        return read(STDIN_FILENO, buf, max_size);
    #endif
}

#endif