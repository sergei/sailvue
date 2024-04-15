//
// Created by Sergei on 8/16/23.
//

#ifndef SAILVUE_INSTRDATAREADER_H
#define SAILVUE_INSTRDATAREADER_H



#include <ctime>
#include <string>
#include <map>
#include <set>
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>
#include <list>

#include "navcomputer/InstrumentInput.h"

class InstrDataReader {
public:
    virtual void read(uint64_t ulStartUtcMs, uint64_t ulEndUtcMs, std::list<InstrumentInput> &listInputs) = 0;
    virtual ~InstrDataReader() {} ;
};


#endif //SAILVUE_INSTRDATAREADER_H
