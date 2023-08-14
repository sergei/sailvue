#ifndef SAILVUE_IPROGRESSLISTENER_H
#define SAILVUE_IPROGRESSLISTENER_H


#include <string>

class IProgressListener {
public:
    virtual void progress(const std::string& state, int progress) = 0;
    virtual bool stopRequested() = 0;
};


#endif //SAILVUE_IPROGRESSLISTENER_H
