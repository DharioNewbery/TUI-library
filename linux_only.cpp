#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sstream>
#include <math.h>

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

struct termios orig_termios;

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
    if (abs(accX) > delta) { accX -= (accX > 0 ? delta : -delta); } else { accX = 0; }
    if (abs(accY) > delta) { accY -= (accY > 0 ? delta : -delta); } else { accY = 0; }

    if (abs(velX) > delta) { velX -= (velX > 0 ? delta : -delta); } else { velX = 0; }
    if (abs(velY) > delta) { velY -= (velY > 0 ? delta : -delta); } else { velY = 0; }

    velX += accX;
    velY += accY;
    
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

void disableRawMode() {
    std::cout << "\033[?1003l\033[?1006l\033[?25h";
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0; // Non-blocking read
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // \033[?1006h = Enable SGR Mouse Mode
    // \033[?1002h = mouse cell motion tracking
    // \033[?25l = Hide Cursor
    std::cout << "\033[?1002h\033[?1006h\033[?25l" << std::flush;
}

struct Screen {
    int width, height;
    std::vector<std::vector<std::string>> buffer;

    Screen() {
        updateSize();
    }

    void updateSize() {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        width = ws.ws_col;
        height = ws.ws_row;
        
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

    // Render everything to one string then print
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
        write(STDOUT_FILENO, frame.c_str(), frame.length());
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
    while (true) {
        // READ INPUT
        

        int nread = read(STDIN_FILENO, &buf, sizeof(buf));
        screen.putChar(myChar.posX, myChar.posY, ' ');
        myChar.update();
        screen.putChar(myChar.posX, myChar.posY, '@');
        screen.render();
        
        if (nread > 0) {
            for (int i = 0; i < nread; i++) {
                if (buf[i] == 'q') return 0;
                
                if (buf[i] == 'w') {
                    myChar.up();
                }
                if (buf[i] == 's') {
                    myChar.down();
                }
                if (buf[i] == 'a') {
                    myChar.left();
                }
                if (buf[i] == 'd') {
                    myChar.right();
                }

                
                // ANSI Parsing
                if (buf[i] == '\033' && nread - i > 5) { // Simple safety check
                     // Look for Mouse Sequence: \033[<...
                     if (buf[i+1] == '[' && buf[i+2] == '<') {
                        int btn, x, y;
                        char type;
                        
                        // Find the end of the sequence 'M' or 'm'
                        int j = i + 3;
                        while (j < nread && buf[j] != 'M' && buf[j] != 'm') j++;

                        // Parse numbers manually or via sscanf
                        // (Pointer arithmetic simply skips "\033[<")
                        if (sscanf(&buf[i+3], "%d;%d;%d%c", &btn, &x, &y, &type) == 4) {
                            screen.putChar(3, 3, type); // Draw
                            if (type == 'M') { // Mouse Press or Drag
                                screen.putChar(x, y, '#'); // Draw
                                screen.render(); // Re-render frame
                            }
                        }
                        i = j; // Advance main loop past this sequence"
                     }
                } else {
                    for(size_t i=0; i<nread; i++) screen.putChar(i+2, 4, buf[i]);
                }
            }
            buf[0] = '\0'; // Clear buffer.
        }
        
        // Optional: Add a small sleep (e.g., 10ms) to reduce CPU usage 
        // if you aren't waiting on blocking input.
        usleep(1000); 

    }
    return 0;
}