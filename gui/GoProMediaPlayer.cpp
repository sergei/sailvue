#include <iostream>
#include <QtCore>
#include  <QtDebug>
#include "GoProMediaPlayer.h"

GoProMediaPlayer::GoProMediaPlayer(QObject *parent)
:QMediaPlayer(parent) {
    connect(this, &QMediaPlayer::playbackStateChanged, this, &GoProMediaPlayer::processStateChanged);
    connect(this, &QMediaPlayer::mediaStatusChanged, this, &GoProMediaPlayer::processMediaStatusChanged);
    connect(this, &QMediaPlayer::positionChanged, this, &GoProMediaPlayer::processPositionChanged);
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
                return;
            }
        }
    }
}


void GoProMediaPlayer::setClip(const QUrl &clip, qint64 position) {
    qDebug() << position << " " << ToString(m_goProState) << "GoProMediaPlayer::setClip: " << clip.toString().toStdString();
    bool newClipNeeded = m_newClip != clip;
    m_newClip = clip;
    m_NewPosition = position;

    if( newClipNeeded ){
        QString clipBaseName = clip.fileName();
        emit clipNameChanged(clipBaseName);
    }


    switch (m_goProState) {
        case GoProStoppedState:
            m_goProState = GoProWaitingToLoadState;
            setSource(clip);
            break;
        case GoProWaitingForNewClipState:
            setSource(m_newClip);
            setPosition(m_NewPosition);
            play();
            m_goProState = GoProPlayingState;
            break;
        case GoProPlayingState:
            if( newClipNeeded ) {
                m_goProState = GoProWaitingToStopAndLoadState;
                qDebug() << "MediaPlayer::stop()" ;
                stop();
            }else{
                m_goProState = GoProWaitingToPauseSeekAndPlayState;
                qDebug() << "MediaPlayer::pause()" ;
                pause();
            }
            break;
        case GoProPausedState:
            if( newClipNeeded ) {
                qDebug() << "MediaPlayer::setSource(" << m_newClip.toString().toStdString() << ")" ;
                setSource(m_newClip);
                setPosition(m_NewPosition);
                play();
//                QThread::msleep(500);
                pause();
            }else{
                qDebug() << "MediaPlayer::setPosition(" << position << ")" ;
                setPosition(m_NewPosition);
            }
            break;
        case GoProWaitingToLoadState:
        case GoProWaitingToStopAndLoadState:
        case GoProWaitingToPauseSeekAndPlayState:
            break;
    }
}

void GoProMediaPlayer::processStateChanged(QMediaPlayer::PlaybackState playBackState){
    qDebug() << "GoProMediaPlayer::processStateChanged: " << ToString(playBackState) << " " << ToString(m_goProState) ;
    switch (m_goProState) {
        case GoProStoppedState:
        case GoProPlayingState:
        case GoProPausedState:
        case GoProWaitingToLoadState:
        case GoProWaitingForNewClipState:
            break;
        case GoProWaitingToStopAndLoadState:
            if(playBackState == QMediaPlayer::StoppedState ){
                m_goProState = GoProWaitingToLoadState;
                qDebug() << "MediaPlayer::setSource(" << m_newClip.toString().toStdString() << ")" ;
                setSource(m_newClip);
            }
            break;
        case GoProWaitingToPauseSeekAndPlayState:
            if(playBackState == QMediaPlayer::PausedState ){
                setPosition(m_NewPosition);
                qDebug() << "MediaPlayer::play()" ;
                play();
                m_goProState = GoProPlayingState;
            }
            break;
    }
}

void GoProMediaPlayer::processMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    qDebug() << "GoProMediaPlayer::processMediaStatusChanged: " << ToString(status) << " " << ToString(m_goProState) ;
    switch (m_goProState) {
        case GoProStoppedState:
            break;
        case GoProPlayingState:
            if( status == QMediaPlayer::EndOfMedia ){
                m_goProState = GoProWaitingForNewClipState;
                playNextClip();
            }
            break;
        case GoProWaitingForNewClipState:
        case GoProPausedState:
            break;
        case GoProWaitingToLoadState:
            if ( status == QMediaPlayer::LoadedMedia ){
                qDebug() << "MediaPlayer::play()" ;
                play();
            }
            break;
        case GoProWaitingToStopAndLoadState:
        case GoProWaitingToPauseSeekAndPlayState:
            break;
    }
}

void GoProMediaPlayer::processPositionChanged(qint64 position) {
//    qDebug() << "GoProMediaPlayer::ProcessPositionChanged: " << position << " " << ToString(m_goProState) ;
    updateRaceIdx(position);

    switch (m_goProState) {
        case GoProStoppedState:
        case GoProPlayingState:
        case GoProPausedState:
        case GoProWaitingForNewClipState:
            break;
        case GoProWaitingToLoadState:
            qDebug() << "QThread::msleep(500);" ;
            QThread::msleep(500);
            qDebug() << "MediaPlayer::pause()" ;
            pause();
            m_goProState = GoProPausedState;
            break;
        case GoProWaitingToStopAndLoadState:
        case GoProWaitingToPauseSeekAndPlayState:
            break;
    }
}

void GoProMediaPlayer::playClip() {
    m_goProState = GoProPlayingState;
    qDebug() << "MediaPlayer::play()" ;
    play();
}

void GoProMediaPlayer::pauseClip() {
    m_goProState = GoProPausedState;
    qDebug() << "MediaPlayer::pause()" ;
    pause();
}


std::string GoProMediaPlayer::ToString(GoProMediaPlayer::GoProState state) {
    switch (state) {
        case GoProStoppedState: return "GoProStoppedState";
        case GoProWaitingToLoadState: return "GoProWaitingToLoadState";
        case GoProWaitingToStopAndLoadState: return "GoProWaitingToStopAndLoadState";
        case GoProWaitingToPauseSeekAndPlayState: return "GoProWaitingToPauseAndSeekState";
        case GoProPlayingState: return "GoProPlayingState";
        case GoProPausedState: return "GoProPausedState";
        case GoProWaitingForNewClipState: return "GoProWaitingForNewClipState";
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


