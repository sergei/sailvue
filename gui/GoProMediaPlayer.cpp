#include <iostream>
#include <QtCore>
#include  <QtDebug>
#include "GoProMediaPlayer.h"

GoProMediaPlayer::GoProMediaPlayer(QObject *parent)
:QMediaPlayer(parent) {
    connect(this, &QMediaPlayer::playbackStateChanged, this, &GoProMediaPlayer::processStateChanged);
    connect(this, &QMediaPlayer::mediaStatusChanged, this, &GoProMediaPlayer::processMediaStatusChanged);
    connect(this, &QMediaPlayer::positionChanged, this, &GoProMediaPlayer::processPositionChanged);
    connect(this, &QMediaPlayer::sourceChanged, this, &GoProMediaPlayer::processSourceChanged);
    connect(this, &QMediaPlayer::bufferProgressChanged, this, &GoProMediaPlayer::processBufferProgressChanged);
    connect(this, &QMediaPlayer::playingChanged, this, &GoProMediaPlayer::processPlayingChanged);
    connect(this, &QMediaPlayer::errorOccurred, this, &GoProMediaPlayer::processErrorOccurred);
}

void GoProMediaPlayer::updateRaceIdx(qint64 clipPositionMs) {
    if( m_pCurrentClipInfo == nullptr )
        return;
    uint64_t ulUtcMs = m_pCurrentClipInfo->getClipStartUtcMs() + clipPositionMs;
    qint64 idx = m_pRaceTreeModel->getIdxForUtc(ulUtcMs);
    emit racePathIdxChanged(idx);
}

void GoProMediaPlayer::moveByMs(qint64 ms) {
    qDebug() << "GoProMediaPlayer::moveByMs: " << ms << " " << ToString(m_goProState) ;
    if(m_pRaceTreeModel == nullptr )
        return;

    if( m_pCurrentClipInfo == nullptr )
        return;

    uint64_t ulUtcMs = m_pCurrentClipInfo->getClipStartUtcMs() + position() + ms;
    qint64 idx = m_pRaceTreeModel->getIdxForUtc(ulUtcMs);
    seekTo(idx);
}

void GoProMediaPlayer::seekTo(qint64 idx) {
    qDebug() << "GoProMediaPlayer::seekTo: " << idx << " " << ToString(m_goProState) ;

    if(m_pRaceTreeModel == nullptr )
        return;

    uint64_t currentUtcMs = m_pRaceTreeModel->getUtcForIdx(idx);

    // TODO check if UTC is within current clip to save some time

    // Find clip corresponding to idx
    for( const auto &clip : *m_pRaceTreeModel->getClipList()) {
        if (clip.getClipStartUtcMs() <= currentUtcMs && currentUtcMs <= clip.getClipEndUtcMs() ){
            m_pCurrentClipInfo = &clip;
            std::string mp4 = std::filesystem::path(clip.getFileName());
            QUrl clipUrl = QUrl::fromLocalFile(mp4.c_str());
            auto position = qint64(currentUtcMs - clip.getClipStartUtcMs());
            setClip(clipUrl, position);
            return;
        }
    }

    std::cerr << "Clip not found for utc ms " << currentUtcMs << std::endl;
}

void GoProMediaPlayer::playNextClip() {
    assert(m_pRaceTreeModel);

    std::list<GoProClipInfo> *clips = m_pRaceTreeModel->getClipList();
    // Find clip after the current one and play it
    for( auto it = clips->begin(); it != clips->end(); it++ ){
        if( &(*it) == m_pCurrentClipInfo ){
            it++;
            if( it != clips->end() ){
                m_pCurrentClipInfo = &(*it);
                std::string mp4 = std::filesystem::path(m_pCurrentClipInfo->getFileName());
                QUrl clipUrl = QUrl::fromLocalFile(mp4.c_str());
                auto position = 0;
                setClip(clipUrl, position);
                play();
                return;
            }
        }
    }
}


void GoProMediaPlayer::setClip(const QUrl &clip, qint64 position) {
    qDebug() << "GoProMediaPlayer::setClip" << position << " " << ToString(m_goProState) << "clip" << clip.toString().toStdString();
    bool newClipNeeded = m_newClip != clip;
    m_newClip = clip;
    m_NewPosition = position;

    if( newClipNeeded ){
        QString clipBaseName = clip.fileName();
        emit clipNameChanged(clipBaseName);
        qDebug() << "MediaPlayer::setSource()" << m_newClip;
        setSource(m_newClip);
    }

    qDebug() << "MediaPlayer::setPosition()" << m_NewPosition;
    setPosition(m_NewPosition);
}

void GoProMediaPlayer::processStateChanged(QMediaPlayer::PlaybackState playBackState){
    qDebug() << "GoProMediaPlayer::processStateChanged: " << ToString(playBackState) << " " << ToString(m_goProState) ;
}

void GoProMediaPlayer::processMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    qDebug() << "GoProMediaPlayer::processMediaStatusChanged: " << ToString(status) << " " << ToString(m_goProState) ;

    switch (status) {
        case LoadedMedia:
            if ( m_goProState == GoProPausedState || m_goProState == GoProStoppedState){
                // Start playing clip so it will appear on a screen
                qDebug() << "Play a bit";
                play();
            }
            break;
        case EndOfMedia:
            if ( m_goProState == GoProPlayingState){
                // Load the next clip
                playNextClip();
            }
            break;
        case NoMedia:
        case LoadingMedia:
        case StalledMedia:
        case BufferingMedia:
        case BufferedMedia:
        case InvalidMedia:
            break;
    }

}

void GoProMediaPlayer::processPositionChanged(qint64 position) {
//    qDebug() << "GoProMediaPlayer::ProcessPositionChanged: " << position << " " << ToString(m_goProState) ;
    if ( m_goProState == GoProPausedState || m_goProState == GoProStoppedState){
        // Stop playing clip so it will appear on a screen
        qDebug() << "Stop,  since we are paused";
        pause();
    }
    updateRaceIdx(position);

}

void GoProMediaPlayer::playClip() {
    m_goProState = GoProPlayingState;
    qDebug() << "GoProMediaPlayer::play()" ;
    play();
}

void GoProMediaPlayer::pauseClip() {
    m_goProState = GoProPausedState;
    qDebug() << "GoProMediaPlayer::pause()" ;
    pause();
}


std::string GoProMediaPlayer::ToString(GoProMediaPlayer::GoProState state) {
    switch (state) {
        case GoProStoppedState: return "GoProStoppedState";
        case GoProPlayingState: return "GoProPlayingState";
        case GoProPausedState: return "GoProPausedState";
        default: return "GoProUnknownState";
    }
}

std::string GoProMediaPlayer::ToString(QMediaPlayer::MediaStatus state) {
    switch(state){
        case QMediaPlayer::NoMedia: return "NoMedia";
        case QMediaPlayer::LoadingMedia: return "LoadingMedia";
        case QMediaPlayer::LoadedMedia: return "LoadedMedia";
        case QMediaPlayer::StalledMedia: return "StalledMedia";
        case QMediaPlayer::BufferingMedia: return "BufferingMedia";
        case QMediaPlayer::BufferedMedia: return "BufferedMedia";
        case QMediaPlayer::EndOfMedia: return "EndOfMedia";
        case QMediaPlayer::InvalidMedia: return "InvalidMedia";
        default: return "UnknownMediaStatus";
    }
}

std::string GoProMediaPlayer::ToString(QMediaPlayer::PlaybackState state){
    switch(state){
        case QMediaPlayer::StoppedState: return "StoppedState";
        case QMediaPlayer::PlayingState: return "PlayingState";
        case QMediaPlayer::PausedState: return "PausedState";
        default: return "UnknownState";
    }
}

void GoProMediaPlayer::processSourceChanged(const QUrl &media) {
    qDebug() << "GoProMediaPlayer::processSourceChanged()" << media;
}

void GoProMediaPlayer::processBufferProgressChanged(float progress){
    qDebug() << "GoProMediaPlayer::processBufferProgressChanged()" << progress;
}
void GoProMediaPlayer::processPlayingChanged(bool playing){
    qDebug() << "GoProMediaPlayer::processPlayingChanged()" << playing;

}
void GoProMediaPlayer::processErrorOccurred(QMediaPlayer::Error error, const QString &errorString){
    qDebug() << "GoProMediaPlayer::processErrorOccurred()" << errorString << "(" << error << ")";
}

