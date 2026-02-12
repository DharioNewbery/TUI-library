#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <math.h>

class Character {
    int maxX, maxY;
    const double delta = 0.001;
    double velX = 0, velY = 0;

public:
    double posX = 0, posY = 0;

    void setConstrains (int x, int y) { maxX = x; maxY = y; }
    void setPos(int x, int y) {
        posX = x; posY = y;
    }

    void up() {
        velY = -2;
    }

    void down() {
        velY = 2;
    }

    void left() {
        velX = -2;
    }

    void right() {
        velX = 2;
    }

    void update() {
        

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
        if(posX + velX > maxX || posX + velX < 1) {
            velX *= -1;
        } else { posX += velX; }
        
        
        if(posY + velY > maxY || posY + velY < 1) {
            velY *= -1;
        } else { posY += velY; }
    }
};

#endif
