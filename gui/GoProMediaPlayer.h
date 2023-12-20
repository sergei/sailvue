#ifndef SAILVUE_GOPROMEDIAPLAYER_H
#define SAILVUE_GOPROMEDIAPLAYER_H

#include <QMediaPlayer>
#include <QtQmlIntegration>

#include "RaceTreeModel.h"

class GoProMediaPlayer : public QMediaPlayer {
Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)
    Q_PROPERTY(RaceTreeModel *model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit GoProMediaPlayer(QObject *parent = nullptr);

    Q_INVOKABLE void setClip(const QUrl &clip, qint64 position);
    Q_INVOKABLE void seekTo(qint64 idx);
    Q_INVOKABLE void moveByMs(qint64 ms);
    [[nodiscard]] RaceTreeModel *model() const { return m_pRaceTreeModel; }
    void setModel(RaceTreeModel *model) { m_pRaceTreeModel = model; }

public Q_SLOTS:
    void playClip();
    void pauseClip();

private:
    RaceTreeModel *m_pRaceTreeModel= nullptr;
    enum GoProState
    {
        GoProStoppedState,
        GoProPlayingState,
        GoProPausedState
    };

    static std::string ToString(GoProState state);
    static std::string ToString(QMediaPlayer::PlaybackState state);
    static std::string ToString(QMediaPlayer::MediaStatus state);

    GoProState m_goProState = GoProStoppedState;
    QUrl m_newClip = QUrl();
    qint64 m_NewPosition = -1;
    const GoProClipInfo *m_pCurrentClipInfo = nullptr;

    void playNextClip();
    void updateRaceIdx(qint64 position);

signals:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NotImplementedFunctions"
    void clipNameChanged(const QString &clipBaseName);
    void racePathIdxChanged(uint64_t racePathIdx);
    void modelChanged();
#pragma clang diagnostic pop

private slots:
    void processStateChanged(QMediaPlayer::PlaybackState playBackState);
    void processMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void processPositionChanged(qint64 position);

    static void processSourceChanged(const QUrl &media);
    static void processBufferProgressChanged(float progress);
    static void processPlayingChanged(bool playing);
    static void processErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
};


#endif //SAILVUE_GOPROMEDIAPLAYER_H
