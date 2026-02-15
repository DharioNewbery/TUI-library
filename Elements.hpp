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

    Screen& m_screen;

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

    Element(size_t w, size_t h, Screen& s): m_screen(s) {
        m_size = {w, h};
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

    pair getSize() { return m_actualSize; }
    pair getOffset() { return m_offset; }
    Arrangement getArrangement() { return m_arrangement; }
    void fillMaxWidth() { m_fillMaxWidth = true; }
    void fillMaxHeight() { m_fillMaxHeight = true; }
    void fillMaxSize() { m_fillMaxWidth = true; m_fillMaxHeight = true; }
};

struct Terminal : public Element {

    Terminal( Screen& s): Element(s.width, s.height, s) {
        m_arrangement = Arrangement::NONE;
    }

    void updateConstrains() override {
        pair newConstrain = {m_screen.width, m_screen.height};
        m_constrain = newConstrain;
    }
};

struct Canvas : public Element {
    Canvas(size_t x, size_t y, Screen& s): Element(x, y, s) {
        m_arrangement = Arrangement::NONE;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Canvas");
        m_screen.putBox(m_offset.x, m_offset.y, box);
    }

    void setArrangement(Arrangement a) {
        m_arrangement = a;
    }
};

struct Column : public Element {
    
    Column(size_t x, size_t y, Screen& s): Element(x, y, s) {
        m_arrangement = Arrangement::VERTICAL;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Column");
        m_screen.putBox(m_offset.x, m_offset.y, box);
    }
};

struct Row : public Element {
    Row(size_t x, size_t y, Screen& s): Element(x, y, s) {
        m_arrangement = Arrangement::HORIZONTAL;
    }

    void drawGraphics () override {
        Box box(m_actualSize.x, m_actualSize.y, "Row");
        m_screen.putBox(m_offset.x, m_offset.y, box);
    }
};

struct Text : public Element {
    std::string m_text;
    Text(size_t x, size_t y, Screen& s, std::string text): Element(x, y, s), m_text(text) {
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
                    m_screen.putText(m_offset.x, m_offset.y + lineOffset, displayText.substr(0, cut - 3) + "...");
                    return; // No more vertical space to render text
                }
                m_screen.putText(m_offset.x, m_offset.y + lineOffset, displayText.substr(0, cut));
                displayText = displayText.substr(cut);
                lineOffset++;

                cut = 0;
            }
            cut++;
        }

        m_screen.putText(m_offset.x, m_offset.y + lineOffset, displayText);


    }
};

struct Spacer : public Element {
    Spacer(size_t x, size_t y, Screen& s): Element(x, y, s) {
        m_arrangement = Arrangement::NONE;
    }

    void drawGraphics () override {
        // Empty space :P
    }
};