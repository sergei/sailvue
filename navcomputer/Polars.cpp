#include <iostream>
#include <fstream>
#include "Polars.h"

void Polars::loadPolar(const std::string &path) {
    std::ifstream f(path, std::ios::in);
    std::string line;

    std::cout << "Reading polars file " << path << std::endl;

    std::vector<double> twaVector;
    std::vector<double> twsVector;
    std::vector<double> spdVector;

    while (std::getline(f, line)) {
        // If starts with #, skip
        if (line[0] == '#') {
            continue;
        }
        bool isHeader = false;
        // If starts with twa/tws, it's a header line
        if (line[0] == 't') {
            isHeader = true;
        }
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        bool firstColumn = true;
        while (std::getline(iss, token, ';')) {
            if( isHeader ){
                if ( !firstColumn)
                    twsVector.push_back(std::stod(token));
            }else{
                if ( firstColumn)
                    twaVector.push_back(std::stod(token));
                else
                    spdVector.push_back(std::stod(token));
            }
            firstColumn = false;
        }
   }

    // Make x and y vectors the same size as spd to be used by interpolator later
    std::vector<double> y;
    std::vector<double> x;
    for(double twa_value : twaVector){
        for(double tws_value : twsVector){
            y.push_back(tws_value);
            x.push_back(twa_value);
        }
    }
    // Create speed interpolator
    m_speedInterp.setData(x, y, spdVector);

    // Create vmg interpolator
    std::vector<double> vmgVector;
    for( int i = 0; i < spdVector.size(); i++)
    {
        vmgVector.push_back(spdVector[i] * cos(x[i] * M_PI / 180.0));
//        std::cout << "twa: " << x[i] << " tws: " << y[i] << " spd: " << spdVector[i] << " vmg: " << vmgVector[i] << std::endl;
    }
    m_vmgInterp.setData(x, y, vmgVector);

    // Get rails
    m_minTwa = twaVector.front();
    m_maxTwa = twaVector.back();
    m_minTws = twsVector.front();
    m_maxTws = twsVector.back();

    std::cout << "Done reading polars file " << path << std::endl;
}

double Polars::getSpeed(double twa, double tws) {
    twa = abs(twa);
    twa = std::min(std::max(twa, m_minTwa), m_maxTwa);
    tws = std::min(std::max(tws, m_minTws), m_maxTws);

    double val = m_speedInterp(twa, tws);

    return val;
}

double Polars::getVmg(double twa, double tws) {
    twa = abs(twa);
    twa = std::min(std::max(twa, m_minTwa), m_maxTwa);
    tws = std::min(std::max(tws, m_minTws), m_maxTws);

    double val = m_vmgInterp(twa, tws);

    return val;
}

std::pair<double, double>  Polars::getTargets(double tws, bool isUpwind){
    if ( isUpwind ){
        double maxVmg = 0;
        double targetTwa = 0;
        for( int twa = int(m_minTwa); twa < 90; twa += 2){
            double vmg = getVmg(twa, tws);
//            std::cout << "twa: " << twa << " vmg: " << vmg << std::endl;
            if ( vmg > maxVmg ){
                maxVmg = vmg;
                targetTwa = twa;
            }
        }
        return std::make_pair(targetTwa, maxVmg);

    } else{
        double minVmg = 0;
        double targetTwa = 0;
        for( int twa = int(m_maxTwa); twa > 90; twa -= 1){
            double vmg = getVmg(twa, tws);
            if (vmg < minVmg ){
                minVmg = vmg;
                targetTwa = twa;
            }
        }
        return std::make_pair(targetTwa, minVmg);
    }
}

