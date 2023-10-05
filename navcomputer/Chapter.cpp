#include "Chapter.h"

Chapter::Chapter(u_int64_t startIdx, u_int64_t endIdx)
:m_startIdx(startIdx), m_endIdx(endIdx), m_gunIdx( (startIdx + endIdx) / 2)
{
    m_uuid = QUuid::createUuid();
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

