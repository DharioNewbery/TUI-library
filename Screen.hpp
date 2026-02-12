#ifndef SCREEN_H
#define SCREEN_H

#include "ConsoleSetup.hpp"
#include "Styles.hpp"
#include <sstream>
#include <string>
#include <vector>
#include "box.hpp"
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

#endif