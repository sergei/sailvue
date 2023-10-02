#ifndef SAILVUE_CHAPTERTYPES_H
#define SAILVUE_CHAPTERTYPES_H

#include <QtQmlIntegration>
#include <QObject>

class ChapterTypes : public QObject{
    Q_OBJECT       // Let the MOC know about this QObject
    QML_ELEMENT    // Make this object available to QML
public:
    enum ChapterType{
        TACK_GYBE,
        SPEED_PERFORMANCE,
        START,
        MARK_ROUNDING
    };
    Q_ENUM(ChapterType)
};


#endif //SAILVUE_CHAPTERTYPES_H
