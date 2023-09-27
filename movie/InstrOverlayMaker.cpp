#include <iostream>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
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

    // Time stamp font must be no longer that cell wideth and no taller that 3/8 of cell height
    int maxHeight = m_infoCell.getHeight() * 3 / 8;
    int fontPointSize;
    for(int fs=1; fs < 100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect("2023-03-01 13:34:00 PDT");
        if(rect.height() > maxHeight || rect.width() > m_infoCell.geWidth()){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_timeStampFont = QFont(FONT_FAMILY_TIMESTAMP, fontPointSize);

    // The copyright font must be no wider that cell width and no taller then 1/4 of cell height
    maxHeight = m_infoCell.getHeight() * 2 / 8;
    for(int fs=1; fs < 100; fs ++){
        fontPointSize = fs;
        font.setPointSize(fs);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect(COPYRIGHT_SAILVUE);
        if( (rect.height() > maxHeight) || (rect.width() > m_infoCell.geWidth()) ){
            fontPointSize = fs - 1;
            break;
        }
    }

    m_copyrightFont = QFont(FONT_FAMILY_TIMESTAMP, fontPointSize);
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

    int x = 0;
    int y = 0;
    int pad = 5;
    // Time stamp
    QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(instrData.utc.getUnixTimeMs()));
    QString tz = time.timeZoneAbbreviation();
    std::string txt = (time.toString("yyyy-MM-dd hh:mm:ss ") + " " + tz).toStdString();
    painter.setFont(m_timeStampFont);
    painter.setPen(m_timeStampPen);
    painter.drawText(QRect(x + pad, y + pad, m_cellStep, m_infoCell.getHeight() * 3 / 8), QString::fromStdString(txt));

    // Coordinates
    y = m_infoCell.getHeight() * 3 / 8;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(5) << instrData.loc.getLat()
    << " " << std::fixed << std::setprecision(5) << instrData.loc.getLon();
    painter.drawText(QRect(x + pad, y + pad, m_cellStep, m_infoCell.getHeight() * 3 / 8), QString::fromStdString(oss.str()));

    // Copyright
    y = m_infoCell.getHeight() * 3 / 4;
    painter.setFont(m_copyrightFont);
    painter.setPen(m_copyrightPen);
    painter.drawText(QRect(x + pad , y + pad, m_cellStep, m_infoCell.getHeight() * 1 / 4), COPYRIGHT_SAILVUE);

    x+= m_cellStep;
    txt = formatSpeed(instrData.sow, instrData.utc);
    m_infoCell.draw(painter, x, m_rectYoffset, "SOW", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = formatSpeed(instrData.sog, instrData.utc);
    m_infoCell.draw(painter, x, m_rectYoffset, "SOG", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = formatSpeed(instrData.tws, instrData.utc);
    m_infoCell.draw(painter, x, m_rectYoffset, "TWS", QString::fromStdString(txt));

    x+= m_cellStep;
    txt = formatAngle(instrData.twa, instrData.utc);
    m_infoCell.draw(painter, x, m_rectYoffset, "TWA", QString::fromStdString(txt));

    image.save(QString::fromStdString(pngName.string()), "PNG");
    return pngName.string();
}

std::string InstrOverlayMaker::formatSpeed(const Speed& speed, const UtcTime& utc) {
    if( ! speed.isValid(utc.getUnixTimeMs()) )
        return "--.-";

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill(' ') << std::fixed << std::setprecision(1) << speed.getKnots();
    return oss.str();
}

std::string InstrOverlayMaker::formatAngle(const Angle& angle, const UtcTime& utc) {
    if( ! angle.isValid(utc.getUnixTimeMs()) )
        return "---°";

    std::ostringstream oss;
    oss << std::setw(3) << std::setfill(' ') << std::fixed << std::setprecision(0) << abs(angle.getDegrees()) << "°";
    return oss.str();
}

