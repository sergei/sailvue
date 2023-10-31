#ifndef SAILVUE_OVERLAYELEMENT_H
#define SAILVUE_OVERLAYELEMENT_H

#include <QImage>
#include <QPainter>
#include "navcomputer/Chapter.h"

class OverlayElement {
public:
    OverlayElement(int width, int height, int x, int y) : m_width(width), m_height(height), m_x(x), m_y(y){}
    virtual void addEpoch(QPainter &painter, int epochIdx) = 0;
    virtual void setChapter(Chapter &chapter) {} ;
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    [[nodiscard]] int getX() const { return m_x; }
    [[nodiscard]] int getY() const { return m_y; }
protected:
    int m_width;
    int m_height;
private:
    int m_x;
    int m_y;
};


#endif //SAILVUE_OVERLAYELEMENT_H
