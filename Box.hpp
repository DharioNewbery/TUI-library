#ifndef BOX_HPP
#define BOX_HPP

#include <string>

struct Box {
    int width, height;
    std::string title;

    Box(int w, int h, std::string t = "") 
        : width(w), height(h), title(t) {}
};

#endif