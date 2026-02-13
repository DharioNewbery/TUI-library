#include "main.hpp"

int main() {
    enableRawMode();
    Screen screen;

    Character myChar;
    
    Box outline(screen.width - 1, screen.height - 1);
    myChar.setConstrains(screen.width - 1, screen.height - 1);
    myChar.setPos(screen.width / 2, screen.height / 2);
    
    char buf[1024];

    while (true) {
        
        // check if size has changed before expensive updateSize method so dynamic elements can be rendered
        if (false)
            screen.updateSize();

        outline.width = screen.width - 1;
        outline.height = screen.height - 1;
        screen.putBox(1, 1, outline);

        screen.putChar(myChar.posX, myChar.posY, ' ');
        myChar.update();
        screen.putChar(myChar.posX, myChar.posY, '@');

        // READ INPUT
        int nread = readInput(buf, sizeof(buf));
        
        if (nread > 0) {
            // Quit on 'q'
            for(int i=0; i<nread; i++) {
                if (buf[i] == 'q') exit(0);
                
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
                            if (type == 'M') { // Mouse Press or Drag
                                screen.render(); // Re-render frame
                            }
                        }
                        i = j; // Advance main loop past this sequence
                    }
                }
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