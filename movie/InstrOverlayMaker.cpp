#include <iostream>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include "InstrOverlayMaker.h"

void InfoCell::setSize(int width) {
    int fontPointSize;

    QFont font = QFont(FONT_FAMILY_VALUE);

    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.tightBoundingRect("----");
        if(rect.width() > width){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_valueFont = QFont(FONT_FAMILY_VALUE, fontPointSize);
    QFontMetrics fm(m_valueFont);
    m_height = fm.ascent();
    m_labelFont = QFont(FONT_FAMILY_LABEL, fontPointSize / 2);
    QFontMetrics fl(m_labelFont);
    m_height += fl.ascent() + fl.descent() + fl.leading();
    m_width = width;
}

void InfoCell::draw(QPainter &painter, int x, int y, const QString &label, const QString &value) {
    QFontMetrics fl(m_labelFont);
    QRect labelRect = fl.tightBoundingRect(label);
    QFontMetrics fv(m_valueFont);
    QRect valueRect = fv.tightBoundingRect(label);

    int labelXoffset = (m_width - labelRect.width())   / 2;
    int labelYoffset = labelRect.height();
    int valueXoffset = (m_width - valueRect.width())   / 2;
    int valueYoffset = 10 + valueRect.height() + labelYoffset;

    painter.setFont(m_labelFont);
    painter.setPen(m_labelPen);
    painter.drawText(x + labelXoffset, y + labelYoffset, label);

    painter.setFont(m_valueFont);
    painter.setPen(m_valuePen);
    painter.drawText(x + valueXoffset, y + valueYoffset, value);
}


InstrOverlayMaker::InstrOverlayMaker(std::filesystem::path &workDir, int width, int height, bool ignoreCache)
: m_workDir(workDir), m_width(width), m_height(height), m_ignoreCache(ignoreCache)
{
    std::filesystem::create_directories(m_workDir);

    m_rectWidth = width;
    int cellWidth = m_rectWidth / m_numCells;
    m_cellStep = m_rectWidth / m_numCells;
    cellWidth = cellWidth * 3 / 4;  // Make it a bit smaller
    m_infoCell.setSize(cellWidth);
    chooseTimeStampFont(m_infoCell.getHeight() / 6);

    m_rectHeight = m_infoCell.getHeight();
    m_rectYoffset = height - m_rectHeight;
    m_rectXoffset = 0;
}

void InstrOverlayMaker::chooseTimeStampFont(int timeStampHeight) {
    QFont font = QFont(FONT_FAMILY_TIMESTAMP);

    int fontPointSize;
    for(int fs=10; fs <100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect("----");
        if(rect.height() > timeStampHeight){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_timeStampFont = QFont(FONT_FAMILY_TIMESTAMP, fontPointSize);
}

std::string InstrOverlayMaker::addEpoch(const std::string &fileName, InstrumentInput &instrData) {
    std::filesystem::path pngName = std::filesystem::path(m_workDir) / fileName;

    if ( std::filesystem::is_regular_file(pngName) && !m_ignoreCache){
        return pngName.string();
    }

    QImage image(m_width, m_height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));
    QPainter painter(&image);

    painter.setPen(QColor(0x32, 0x32, 0x32, 0x80));
    painter.setBrush(QColor(0x32, 0x32, 0x32, 0x80));
    painter.drawRect(0, 0, m_width, m_height);

    int x = m_cellStep;
    std::string txt = instrData.sow.toString(instrData.utc.getUnixTimeMs());
    m_infoCell.draw(painter, x, m_rectYoffset, "SPD", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = instrData.sog.toString(instrData.utc.getUnixTimeMs());
    m_infoCell.draw(painter, x, m_rectYoffset, "SOW", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = instrData.tws.toString(instrData.utc.getUnixTimeMs());
    m_infoCell.draw(painter, x, m_rectYoffset, "TWS", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = instrData.twa.toString(instrData.utc.getUnixTimeMs());
    m_infoCell.draw(painter, x, m_rectYoffset, "TWA", QString::fromStdString(txt));

    std::cout << "Created " << pngName << std::endl;
    image.save(QString::fromStdString(pngName.string()), "PNG");

    return pngName.string();
}

