#ifndef SAILVUE_CLIPMARKER_H
#define SAILVUE_CLIPMARKER_H

#include <string>
#include "cameras/CameraBase.h"


class ClipMarker {
public:

    void setClipStartUtc(u_int64_t clipStartUtcMs){
        m_UtcMsIn = clipStartUtcMs + m_inTimeSec * 1000;
        m_UtcMsOut = clipStartUtcMs + m_outTimeSec * 1000;
    }

    [[nodiscard]] const std::string &getName() const { return m_name; }
    [[nodiscard]] u_int64_t getUtcMsIn() const { return m_UtcMsIn; }
    [[nodiscard]] u_int64_t getUtcMsOut() const { return m_UtcMsOut; }
    void setName(const std::string &makerName) { m_name = makerName; }
    void setInTimeSec(u_int64_t mInTimeSec) { m_inTimeSec = mInTimeSec; }
    void setOutTimeSec(u_int64_t mOutTimeSec) { m_outTimeSec = mOutTimeSec; }
    CameraClipInfo *getClipInfo() const { return m_pClipInfo; }
    void setClipInfo(CameraClipInfo *mPClipInfo) { m_pClipInfo = mPClipInfo; }

private:
    std::string m_name="Untitled Marker";
    u_int64_t m_inTimeSec=0;
    u_int64_t m_outTimeSec=0;
    u_int64_t m_UtcMsIn=0;
    u_int64_t m_UtcMsOut=0;
    CameraClipInfo *m_pClipInfo = nullptr;
};


#endif //SAILVUE_CLIPMARKER_H
