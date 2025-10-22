#include "PilotClipOverlayMaker.h"
PilotClipOverlayMaker::PilotClipOverlayMaker(int width, int height, int x, int y)
  :OverlayElement(width, height, x, y){

}

void PilotClipOverlayMaker::addEpoch(QPainter &painter, const InstrumentInput &epoch) {
    // Determine tack based on TWA
    QColor backgroundColor;
    if (epoch.twa.isValid(epoch.utc.getUnixTimeMs())) {
        double twaValue = epoch.twa.getDegrees();

        // Port tack: TWA is negative (wind from port side)
        // Starboard tack: TWA is positive (wind from starboard side)
        if (twaValue < 0) {
            backgroundColor = QColor(255, 0, 0, 128); // Red with transparency for port tack
        } else {
            backgroundColor = QColor(0, 255, 0, 128); // Green with transparency for starboard tack
        }
    } else {
        backgroundColor = QColor(128, 128, 128, 128); // Gray if TWA is invalid
    }

    // Set the background color
    painter.setPen(backgroundColor);
    painter.setBrush(backgroundColor);
    painter.drawRect(0, 0, m_width, m_height);

    // Display epoch time in local timezone in the top half of the screen
    QDateTime epochTime = QDateTime::fromMSecsSinceEpoch(epoch.utc.getUnixTimeMs());
    epochTime = epochTime.toLocalTime();
    QString timeString = epochTime.toString("hh:mm:ss");

    // Set up text properties for the time display - much larger font
    QFont timeFont("Arial", m_height / 3, QFont::Bold);
    painter.setFont(timeFont);
    painter.setPen(QColor(255, 255, 255)); // White text

    // Calculate position for centering text in top half
    QFontMetrics fm(timeFont);
    int textWidth = fm.horizontalAdvance(timeString);
    int textHeight = fm.height();

    int x = (m_width - textWidth) / 2;
    int y = (m_height / 4) + (textHeight / 2); // Center in top half (baseline)

    // Draw the time
    painter.drawText(x, y, timeString);

    // Draw the date just below the time, 4x smaller font
    QString dateString = epochTime.toString("yyyy-MM-dd");
    int dateFontSize = std::max(1, m_height / 12); // 4x smaller than time font (m_height/3)
    QFont dateFont("Arial", dateFontSize, QFont::Normal);
    painter.setFont(dateFont);
    QFontMetrics dfm(dateFont);
    int dateTextWidth = dfm.horizontalAdvance(dateString);
    int dateX = (m_width - dateTextWidth) / 2;
    // place one small gap below the time baseline
    int dateY = y + fm.descent() + 2 + dfm.ascent();
    painter.drawText(dateX, dateY, dateString);

    // Display SOW and TWS in bottom half - bigger font
    QFont dataFont("Arial", m_height / 4, QFont::Bold);
    painter.setFont(dataFont);
    QFontMetrics dataFm(dataFont);

    // SOW on the left
    QString sowString = "---";
    if (epoch.sow.isValid(epoch.utc.getUnixTimeMs())) {
        sowString = QString::number(epoch.sow.getKnots(), 'f', 1);
    }
    int sowX = m_width / 4 - dataFm.horizontalAdvance(sowString) / 2;
    int sowY = (3 * m_height / 4) + (dataFm.height() / 2);
    painter.drawText(sowX, sowY, sowString);

    // TWS on the right
    QString twsString = "---";
    if (epoch.tws.isValid(epoch.utc.getUnixTimeMs())) {
        twsString = QString::number(epoch.tws.getKnots(), 'f', 1);
    }
    int twsX = (3 * m_width / 4) - dataFm.horizontalAdvance(twsString) / 2;
    int twsY = (3 * m_height / 4) + (dataFm.height() / 2);
    painter.drawText(twsX, twsY, twsString);

    // Add small labels underneath the numbers
    QFont labelFont("Arial", m_height / 20, QFont::Normal);
    painter.setFont(labelFont);
    QFontMetrics labelFm(labelFont);

    // SOW label
    QString sowLabel = "SOW";
    int sowLabelX = m_width / 4 - labelFm.horizontalAdvance(sowLabel) / 2;
    int sowLabelY = sowY + dataFm.descent() + labelFm.height();
    painter.drawText(sowLabelX, sowLabelY, sowLabel);

    // TWS label
    QString twsLabel = "TWS";
    int twsLabelX = (3 * m_width / 4) - labelFm.horizontalAdvance(twsLabel) / 2;
    int twsLabelY = twsY + dataFm.descent() + labelFm.height();
    painter.drawText(twsLabelX, twsLabelY, twsLabel);
}