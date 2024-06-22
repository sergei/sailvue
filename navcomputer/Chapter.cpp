#include "Chapter.h"

Chapter::Chapter(u_int64_t startIdx, u_int64_t endIdx)
:m_startIdx(startIdx), m_endIdx(endIdx), m_gunIdx( (startIdx + endIdx) / 2)
{
    m_uuid = QUuid::createUuid();
}

Chapter::Chapter(QUuid uuid,u_int64_t startIdx, u_int64_t endIdx)
:m_uuid(uuid),m_startIdx(startIdx), m_endIdx(endIdx), m_gunIdx( (startIdx + endIdx) / 2)
{
}

void Chapter::SetName(std::string s) {
    name = s;
}

bool Chapter::isFetch() const {
    return m_isFetch;
}

void Chapter::setFetch(bool isFetch) {
    m_isFetch = isFetch;
}

const std::string &Chapter::getChapterClipFileName() const {
    return m_chapterClipFileName;
}

void Chapter::setChapterClipFileName(const std::string &chapterClipFileName) {
    m_chapterClipFileName = chapterClipFileName;
}

