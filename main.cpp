#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>

// --- Platform Specific Includes and Definitions ---
#ifdef _WIN32
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


namespace style {
    namespace defaultBox {
        const std::string LEFT_UPPER_CORNER = "┌";
        const std::string RIGHT_UPPER_CORNER = "┐"; 
        const std::string LEFT_DOWN_CORNER = "└";
        const std::string RIGHT_DOWN_CORNER = "┘";
        const std::string HORIZONTAL_BORDER = "─";
        const std::string VERTICAL_BORDER = "│";
    }
}

class Character {
    const double delta = 0.00001;
    double velX = 0, velY = 0;
    double accX = 0, accY = 0;

public:
    double posX = 0, posY = 0;

    void setPos(int x, int y) {
        posX = x; posY = y;
    }

    void up() {
        accY -= 0.0005;
    }

    void down() {
        accY += 0.0005;
    }

    void left() {
        accX -= 0.0005;
    }

    void right() {
        accX += 0.0005;
    }

    void update() {
        // 1. Friction-style logic for Acceleration
        // Check X Acceleration
        if (std::abs(accX) <= delta) {
            accX = 0;
        } else {
            accX -= (accX > 0 ? delta : -delta);
        }

        // Check Y Acceleration
        if (std::abs(accY) <= delta) {
            accY = 0;
        } else {
            accY -= (accY > 0 ? delta : -delta);
        }

        // 2. Apply Acceleration to Velocity
        velX += accX;
        velY += accY;

        // 3. Friction-style logic for Velocity
        if (std::abs(velX) <= delta) {
            velX = 0;
        } else {
            velX -= (velX > 0 ? delta : -delta);
        }

        if (std::abs(velY) <= delta) {
            velY = 0;
        } else {
            velY -= (velY > 0 ? delta : -delta);
        }

        // 4. Update Position
        posX += velX;
        posY += velY;
    }
};

class Box {
public:
    int width, height;
    std::string title;

    Box(int w, int h, std::string t = "") 
        : width(w), height(h), title(t) {}
};

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

void getWindowSize(int &width, int &height) {
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

// --- Application Logic ---

struct Screen {
    int width, height;
    std::vector<std::vector<std::string>> buffer;

    Screen() {
        updateSize();
    }

    void updateSize() {
        getWindowSize(width, height);
        // Ensure strictly positive dimensions
        if (width <= 0) width = 80;
        if (height <= 0) height = 24;
        
        buffer.assign(height, std::vector<std::string>(width, " "));
    }

    void putChar(int x, int y, char c) {
        // Transform to 0-indexed vectors (terminal is 1-indexed)
        int vecX = x - 1;
        int vecY = y - 1;

        if (vecY >= 0 && vecY < height && vecX >= 0 && vecX < width) {
            buffer[vecY][vecX] = c;
        }
    }

    // Overload putChar to accept a string (for UTF-8 characters)
    void putChar(int x, int y, std::string s) {
        // convert 1-coords to 0-coords
        int vX = x - 1;
        int vY = y - 1;

        if (vY >= 0 && vY < height && vX >= 0 && vX < width) {
            buffer[vY][vX] = s;
        }
    }

    void putText(int x0, int y0, const std::string& text) {
        for(size_t i=0; i<text.length(); i++)
        putChar(x0 + i, y0, text[i]);
    }

    void putList(int x0, int y0, const std::vector<std::string>& list) {
        for (std::string line : list) {
            putText(x0, y0, line);
            y0++;
        }
    }

    void putBox(int x0, int y0, Box box) {
        int width = box.width;
        int height = box.height;
        std::string title = box.title;

        if (width < 2 || height < 2) return;

        // 1. Draw Corners
        putChar(x0, y0, style::defaultBox::LEFT_UPPER_CORNER);
        putChar(x0 + width - 1, y0, style::defaultBox::RIGHT_UPPER_CORNER);
        putChar(x0, y0 + height - 1, style::defaultBox::LEFT_DOWN_CORNER); 
        putChar(x0 + width - 1, y0 + height - 1, style::defaultBox::RIGHT_DOWN_CORNER);

        // 2. Draw Horizontal Lines
        for (int i = 1; i < width - 1; ++i) {
            putChar(x0 + i, y0, style::defaultBox::HORIZONTAL_BORDER);                // Top
            putChar(x0 + i, y0 + height - 1, style::defaultBox::HORIZONTAL_BORDER);   // Bottom
        }

        // 3. Draw Vertical Lines
        for (int i = 1; i < height - 1; ++i) {
            putChar(x0, y0+ i, style::defaultBox::VERTICAL_BORDER);                // Left
            putChar(x0 + width - 1, y0 + i, style::defaultBox::VERTICAL_BORDER);    // Right
        }

        // 4. Draw Title (if any)
        if (!title.empty()) {
            int titlePos = x0 + (width / 2) - (title.length() / 2);
            putText(titlePos, y0, title);
        }
    }

    void render() {
        std::stringstream ss;
        ss << "\033[H"; 
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                ss << buffer[y][x];
            }
            if (y < height - 1) ss << "\r\n";
        }
        std::string frame = ss.str();
        writeBuffer(frame);
    }
};

int main() {
    enableRawMode();
    Screen screen;

    // Draw initial instructions
    std::string msg = "CLICK/DRAG TO DRAW 'X'. PRESS 'q' TO QUIT.";
    Box myBox(30, 10, "Opcoes");
    const std::vector<std::string> options = {"Criar", "Remover", "Editar"};
    int currentSelection = 0;
    Character myChar;

    screen.putText(2, 3, msg);
    screen.putBox(3, 3, myBox);
    screen.putList(4, 4, options);

    myChar.setPos(screen.width / 2, screen.height / 2);
    screen.render();

    char buf[1024];
    
    // Example: Add a character to prove it renders
    if (screen.height > 5 && screen.width > 5) {
        screen.buffer[5][5] = "#"; 
    }

    while (true) {

        screen.putChar(myChar.posX, myChar.posY, ' ');
        myChar.update();
        screen.putChar(myChar.posX, myChar.posY, '@');

        // READ INPUT
        int nread = readInput(buf, sizeof(buf));
        
        if (nread > 0) {
            // Quit on 'q'
            for(int i=0; i<nread; i++) {
                if (buf[i] == 'q') exit(0);
            }

            if (buf[0] == 'w') myChar.up();
            if (buf[0] == 's') myChar.down();
            if (buf[0] == 'a') myChar.left();
            if (buf[0] == 'd') myChar.right();

            
            // Example: Print input at top left for debug
            screen.buffer[10][10] = buf[0];
        }

        screen.render();
        
        sleepMs(10); // Sleep 10ms (approx 100 FPS cap)
    }
    return 0;
}