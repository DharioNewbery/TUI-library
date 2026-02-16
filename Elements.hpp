#include "Screen.hpp"

enum class Arrangement { HORIZONTAL, VERTICAL, NONE };

struct pair {
    size_t x, y;
    pair operator+ (const pair& other) const {
        return {x + other.x, y + other.y};
    }
    pair operator+= (const pair& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    pair operator- (const pair& other) const {
        return {x - other.x, y - other.y};
    }
    pair operator= (const pair& other) {
        x = other.x;
        y = other.y;
        return *this;
    }
    bool operator>= (const pair& other) const {
        return x >= other.x || y >= other.y;
    }
    bool operator== (const pair& other) const {
        return x == other.x && y == other.y;
    }
};

class Element {
protected:
    Screen* m_screen = nullptr;

    std::vector<Element*> m_children = {};
    Element* parent = nullptr;
    
    pair m_size = {0, 0}; // The size that the element wants to be (before constraints)
    pair m_constrain = {0, 0}; // The maximum size the element can be (inherited from parent)
    pair m_actualSize = {0, 0}; // The size that the element will actually be (after constraints)
    pair m_offset = {0, 0}; // The offset of the element from the top-left corner of the parent element
    pair m_arrangementOffset = {0, 0}; // The offset used for arranging itself on parent
    pair m_childrenCurrentOffset = {0, 0}; // The current offset for arranging children (accumulates as children are added)

    bool m_fillMaxWidth = false; // Whether the element should fill the maximum width available to it (ignoring its own size)
    bool m_fillMaxHeight = false; // Whether the element should fill the maximum height available to it (ignoring its own size)

    Arrangement m_arrangement = Arrangement::NONE;
protected:

    void cascadeScreenToChildren() {
        for (Element* child : m_children) {
            child->m_screen = m_screen;
            child->cascadeScreenToChildren();
        }
    }

    void updateChildren() {
        m_childrenCurrentOffset = {0, 0}; // reset before arranging children
        
        for (Element* child : m_children) {
            child->setArrangementOffset(m_childrenCurrentOffset);
            if (child->getOffset() >= m_offset + m_actualSize) {
                return;
            }
            child->update();
            
            m_childrenCurrentOffset += child->getSize();
            if (m_arrangement == Arrangement::HORIZONTAL) {
                m_childrenCurrentOffset.y = 0; // reset y offset for horizontal arrangement
            } else if (m_arrangement == Arrangement::VERTICAL) {
                m_childrenCurrentOffset.x = 0; // reset x offset for vertical arrangement
            } else {
                m_childrenCurrentOffset = {0, 0}; // reset both offsets for NONE arrangement
            }
        }
    }

    void updateActualSize() {
        

        m_actualSize.x = std::min(m_size.x, m_constrain.x);
        m_actualSize.y = std::min(m_size.y, m_constrain.y);

        if (m_fillMaxWidth) m_actualSize.x = m_constrain.x;
        if (m_fillMaxHeight) m_actualSize.y = m_constrain.y;

    }

    virtual void updateConstrains() {
        if (!parent) return;
        m_constrain = parent->m_actualSize + parent->m_offset - m_offset - pair{1, 1}; // -2 for the border of the parent box
    }

    void updateOffsets() {
        if (!parent) return;
        m_offset = parent->m_offset + m_arrangementOffset + pair{1, 1}; // +1 for the border of the parent box
    }

    void setArrangementOffset(pair offset) {
        m_arrangementOffset = offset;
        updateOffsets();
    }

    virtual void drawGraphics() {};

public:

    Element(size_t w, size_t h) {
        m_size = {w, h};
    }

    Element(Screen* s, std::initializer_list<Element*> list): m_screen(s) {
        m_size = {0, 0};
        for (auto child : list) {
            child->m_screen = s; // Ensure child elements have the same screen reference
            this->addChild(child);
        }
    }


    Element(std::initializer_list<Element*> list) {
        m_size = {0, 0};
        for (auto child : list) {
            this->addChild(child);
        }
    }
    
    virtual void update() {
        updateOffsets();
        updateConstrains();
        updateActualSize();
        drawGraphics();
        updateChildren();
    };

    void addChild(Element* newChild) {
        newChild->parent = this; 
        m_children.push_back(newChild);
    }

    Element* size(size_t w, size_t h) { m_size = {w, h}; return this; }
    Element* width(size_t w) { m_size.x = w; return this; }
    Element* height(size_t h) { m_size.y = h; return this; }
    Element* fillMaxWidth() { m_fillMaxWidth = true; return this; }
    Element* fillMaxHeight() { m_fillMaxHeight = true; return this; }
    Element* fillMaxSize() { m_fillMaxWidth = true; m_fillMaxHeight = true; return this; }



    pair getSize() { return m_actualSize; }
    pair getOffset() { return m_offset; }
    Arrangement getArrangement() { return m_arrangement; }
};

struct Terminal : public Element {
    Terminal(Screen* s, std::initializer_list<Element*> list = {}): Element(s, list) {
        m_size = {m_screen->width, m_screen->height};
        m_arrangement = Arrangement::NONE;
        cascadeScreenToChildren(); // Ensure all children have the screen reference
    }

    void updateConstrains() override {
        pair newConstrain = {m_screen->width, m_screen->height};
        m_constrain = newConstrain;
    }
};

struct Canvas : public Element {
    Canvas(size_t w = 0, size_t h = 0, std::initializer_list<Element*> list = {}): Element(list) {
        m_size = {w, h};
        m_arrangement = Arrangement::NONE;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Canvas");
        m_screen->putBox(m_offset.x, m_offset.y, box);
    }

    void setArrangement(Arrangement a) {
        m_arrangement = a;
    }
};

struct Column : public Element {
    Column(size_t w, size_t h, std::initializer_list<Element*> list = {}): Element(list) {
        m_size = {w, h};
        m_arrangement = Arrangement::VERTICAL;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Column");
        m_screen->putBox(m_offset.x, m_offset.y, box);
    }
};

struct Row : public Element {
    
    Row(size_t w, size_t h, std::initializer_list<Element*> list = {}): Element(list) {
        m_size = {w, h};
        m_arrangement = Arrangement::HORIZONTAL;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Row");
        m_screen->putBox(m_offset.x, m_offset.y, box);
    }
};

struct Text : public Element {
    std::string m_text;
    Text(size_t w, size_t h, std::string text): Element(w, h), m_text(text) {
        m_arrangement = Arrangement::NONE;
    }

    void drawGraphics () override {
        
        std::string displayText = std::to_string(m_constrain.y) + " " + m_text;
        int maxWidth = m_constrain.x;
        int lineOffset = 0;

        int cut = 0;

        while (cut < displayText.size()) {
            if (cut >= maxWidth) {
                if (lineOffset + 1 >= m_constrain.y) {
                    m_screen->putText(m_offset.x, m_offset.y + lineOffset, displayText.substr(0, cut - 3) + "...");
                    return; // No more vertical space to render text
                }
                m_screen->putText(m_offset.x, m_offset.y + lineOffset, displayText.substr(0, cut));
                displayText = displayText.substr(cut);
                lineOffset++;

                cut = 0;
            }
            cut++;
        }

        m_screen->putText(m_offset.x, m_offset.y + lineOffset, displayText);


    }
};

struct Spacer : public Element {
    Spacer(size_t w, size_t h): Element(w, h) {
        m_arrangement = Arrangement::NONE;
    }

    void drawGraphics () override {
        // Empty space :P
    }
};