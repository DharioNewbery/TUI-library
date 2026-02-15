#include "main.hpp"

int main() {
    enableRawMode();
    Screen screen;

    screen.updateSize();
    Terminal terminal(screen);
    Canvas canva1(screen.width - 4, screen.height - 4, screen);
    canva1.setArrangement(Arrangement::VERTICAL);
    
    terminal.addChild(&canva1);

    Canvas *c;
    for (int i = 0; i < 4; i++) {
        Row *row = new Row(0, 10, screen);

        for (int j = 1; j <= i + 1; j++) {
            c = new Canvas(5 + i, 5 + i, screen);
            row->addChild(c);
        }

        canva1.addChild(row);
        row->fillMaxWidth();
        Text *t = new Text(0, 0, screen, "something something...");
        Spacer *s = new Spacer(1, 0, screen);
        row->addChild(s);
        row->addChild(t);
    }

    char buf[1024];

    while (true) {

        size_t deltaW, deltaH;
        getWindowSize(deltaW, deltaH);
        if (deltaW != screen.width || deltaH != screen.height) {
            screen.updateSize();
        }

        terminal.update();

        // READ INPUT
        int nread = readInput(buf, sizeof(buf));
        
        if (nread > 0) {
            // Quit on 'q'
            for(int i=0; i<nread; i++) {
                if (buf[i] == 'q') exit(0);
            }
        }
        screen.render();
        sleepMs(33); // Sleep 10ms (approx 100 FPS cap)
    }
    return 0;
}