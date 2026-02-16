#include "main.hpp"

Column* drawColumn(std::initializer_list<Element*> list = {}) {
    return new Column(0, 0, list);
}

Row* drawRow(std::initializer_list<Element*> list = {}) {
    return new Row(0, 0, list);
}

Canvas* drawCanvas(std::initializer_list<Element*> list = {}) {
    return new Canvas(0, 0, list);
}


int main() {
    enableRawMode();
    Screen screen = Screen();

    Terminal terminal(&screen, {
        drawColumn({
            drawColumn({
                drawCanvas()->size(10, 5),
                drawCanvas()->size(10, 5),
                drawRow()->fillMaxWidth()->height(5)
            })->fillMaxSize()
        })->fillMaxSize()
    });


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