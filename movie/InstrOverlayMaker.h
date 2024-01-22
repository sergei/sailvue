#ifndef SAILVUE_INSTROVERLAYMAKER_H
#define SAILVUE_INSTROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "OverlayElement.h"

static const char *const FONT_FAMILY_TIMESTAMP = "Courier";
static const char *const FONT_FAMILY_VALUE = "Courier";
static const char *const FONT_FAMILY_LABEL = "Courier";

static const char *const COPYRIGHT_SAILVUE = "(c) github.com/sergei/sailvue " GIT_HASH;

class InfoCell {
public:
    void setSize(int width);
    void draw(QPainter &painter, int x, int y, const QString &label, const QString &value);
    [[nodiscard]] int getHeight() const {return m_height;};
    [[nodiscard]] int geWidth() const {return m_width;};

private:
    QFont m_valueFont;
    QFont m_labelFont;
    int m_height;
    int m_width;
    QPen m_labelPen = QPen(QColor(255, 255, 255, 200));
    QPen m_valuePen = QPen(QColor(255, 255, 255, 255));
};


class InstrOverlayMaker : public OverlayElement{
public:
    InstrOverlayMaker(std::vector<InstrumentInput> &instrDataVector, int width, int height, int x, int y);
    void addEpoch(QPainter &painter, const InstrumentInput &epoch);
private:
    void chooseTimeStampFont(int timeStampHeight);
    static std::string formatSpeed(const Speed& speed, const UtcTime& utc);
    static std::string formatAngle(const Angle& angle, const UtcTime& utc);
private:
    InfoCell m_infoCell;
    std::vector<InstrumentInput> &m_instrDataVector;

    const int m_numCells = 5;
    int m_rectWidth;
    int m_rectHeight;
    int m_cellStep;

    int m_rectYoffset = 0;
    int m_rectXoffset = 0;

    QFont m_timeStampFont;
    QPen m_timeStampPen = QPen(QColor(255, 255, 255, 128));

    QFont m_copyrightFont;
    QPen m_copyrightPen = QPen(QColor(255, 255, 255, 128));

};


#endif //SAILVUE_INSTROVERLAYMAKER_H
