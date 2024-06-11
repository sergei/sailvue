#ifndef SAILVUE_INSTA360_H
#define SAILVUE_INSTA360_H

#include <string>
#include "../InstrDataReader.h"
#include "navcomputer/IProgressListener.h"
#include "cameras/CameraBase.h"


class Insta360 : public CameraBase{
public:
    Insta360(InstrDataReader& rInstrDataReader, IProgressListener& rProgressListener);
private:
    std::tuple<int , int >  readClipFile(const std::string &clipFileName, std::list<InstrumentInput> &listInputs) override;
};


#endif //SAILVUE_INSTA360_H
