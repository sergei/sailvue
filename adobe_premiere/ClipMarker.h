#ifndef SAILVUE_CLIPMARKER_H
#define SAILVUE_CLIPMARKER_H

#include <string>
#include "cameras/CameraBase.h"


class ClipMarker {
public:
    void setClipStartUtc(u_int64_t clipStartUtcMs){
        m_UtcMsIn = clipStartUtcMs + m_inTimeMillisecond;
        m_UtcMsOut = clipStartUtcMs + m_outTimeMillisecond;
    }
    void setName(const std::string &makerName) { m_name = makerName; }
    void setInTimeMilliSecond(u_int64_t inTimeMillisecond) { m_inTimeMillisecond = inTimeMillisecond; }
    void setOutTimeMilliSecond(u_int64_t outTimeMillisecond) { m_outTimeMillisecond = outTimeMillisecond; }
    void setClipInfo(CameraClipInfo *mpClipInfo) { m_pClipInfo = mpClipInfo; }
    void setOverlayName(const std::string &mOverlayName) { m_OverlayName = mOverlayName; }

    [[nodiscard]] const std::string &getName() const { return m_name; }
    [[nodiscard]] u_int64_t getUtcMsIn() const { return m_UtcMsIn; }
    [[nodiscard]] u_int64_t getUtcMsOut() const { return m_UtcMsOut; }
    [[nodiscard]] const std::string &getOverlayName() const { return m_OverlayName; }
    [[nodiscard]] CameraClipInfo *getClipInfo() const { return m_pClipInfo; }

private:
    std::string m_name="Untitled Marker";
    u_int64_t m_inTimeMillisecond=0;
    u_int64_t m_outTimeMillisecond=0;
    u_int64_t m_UtcMsIn=0;
    u_int64_t m_UtcMsOut=0;
    CameraClipInfo *m_pClipInfo = nullptr;
    std::string m_OverlayName = "";
};


#endif //SAILVUE_CLIPMARKER_H
