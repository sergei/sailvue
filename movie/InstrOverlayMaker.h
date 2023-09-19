#ifndef SAILVUE_INSTROVERLAYMAKER_H
#define SAILVUE_INSTROVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"

static const char *const FONT_FAMILY_TIMESTAMP = "Courier";
static const char *const FONT_FAMILY_VALUE = "Courier";
static const char *const FONT_FAMILY_LABEL = "Courier";

class InfoCell {
public:
    void setSize(int width);
    void draw(QPainter &painter, int x, int y, const QString &label, const QString &value);
    [[nodiscard]] int getHeight() const {return m_height;};
private:
    QFont m_valueFont;
    QFont m_labelFont;
    int m_height;
public:

private:
    int m_width;

    QPen m_labelPen = QPen(QColor(255, 255, 255, 200));
    QPen m_valuePen = QPen(QColor(255, 255, 255, 255));
};


class InstrOverlayMaker {
public:
    InstrOverlayMaker(std::filesystem::path &workDir, int width, int height, bool ignoreCache);
    std::string addEpoch(const std::string &fileName, InstrumentInput &instrData);
private:
    void chooseTimeStampFont(int timeStampHeight);
    static std::string formatSpeed(const Speed& speed, const UtcTime& utc);
    static std::string formatAngle(const Angle& angle, const UtcTime& utc);
private:
    InfoCell m_infoCell;
    std::filesystem::path m_workDir;
    int m_width;
    int m_height;
    bool m_ignoreCache;

    const int m_numCells = 5;
    int m_rectWidth;
    int m_rectHeight;
    int m_cellStep;

    int m_rectYoffset = 0;
    int m_rectXoffset = 0;

    QFont m_timeStampFont;
    QPen m_timeStampPen = QPen(QColor(255, 255, 255, 255));

};


#endif //SAILVUE_INSTROVERLAYMAKER_H
