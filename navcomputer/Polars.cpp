#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "Polars.h"

void Polars::loadPolar(const std::string &path) {
    // Check the extension of the file. If it's .csv load CVS file otherwise load POL file
    std::string ext = boost::algorithm::to_lower_copy(path.substr(path.find_last_of('.') + 1));
    if ( ext == "csv"){
        loadPolarCsv(path);
    }else if ( ext == "pol"){
        loadPolarPol(path);
    }else{
        std::cout << "Unknown polar file extension: " << ext << std::endl;
    }
}

void Polars::loadPolarCsv(const std::string &path) {
    std::ifstream f(path, std::ios::in);
    std::string line;

    std::cout << "Reading CSV formatted polars file " << path << std::endl;

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

void Polars::loadPolarPol(const std::string &path) {
    std::ifstream f(path, std::ios::in);
    std::string line;

    std::cout << "Reading POL formatted polars file " << path << std::endl;

    std::vector<double> tws;    // 1, 1, 1, 2, 2, 2, 3, 3, 3
    std::vector<double> twa;    // 1, 2, 3, 1, 2, 3, 1, 2, 3
    std::vector<double> spd;    // boat speed

    // Since TWA is not uniformly spaced let's make it y and TWS x
    m_isTransposed = true;

    while (std::getline(f, line)) {
        // If starts with #, skip
        if (line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        bool firstColumn = true;
        double tws_value; // TWS
        while (std::getline(iss, token, '\t')) {
            if ( firstColumn) {
                tws_value = std::stod(token);
            }else {
                double twa_value = std::stod(token);
                std::getline(iss, token, '\t');
                double speed_value = std::stod(token);
                tws.push_back(tws_value);
                twa.push_back(twa_value);
                spd.push_back(speed_value);
            }
            firstColumn = false;
        }
   }

    // Create speed interpolator
    m_speedInterp.setData(tws, twa, spd);

    // Create vmg interpolator
    std::vector<double> vmgVector;
    for( int i = 0; i < spd.size(); i++)
    {
        vmgVector.push_back(spd[i] * cos(twa[i] * M_PI / 180.0));
//        std::cout << "twa: " << twa[i] << " tws: " << tws[i] << " spd: " << spd[i] << " vmg: " << vmgVector[i] << std::endl;
    }
    m_vmgInterp.setData(tws, twa, vmgVector);

    // Get rails
    m_minTwa = twa.front();
    m_maxTwa = twa.back();
    m_minTws = tws.front();
    m_maxTws = tws.back();

    std::cout << "Done reading polars file " << path << std::endl;
}

double Polars::getSpeed(double twa, double tws) {
    twa = abs(twa);
    twa = std::min(std::max(twa, m_minTwa), m_maxTwa);
    tws = std::min(std::max(tws, m_minTws), m_maxTws);

    if ( m_isTransposed){
        return m_speedInterp(tws, twa);
    }else {
        return m_speedInterp(twa, tws);
    }
}

double Polars::getVmg(double twa, double tws) {
    twa = abs(twa);
    twa = std::min(std::max(twa, m_minTwa), m_maxTwa);
    tws = std::min(std::max(tws, m_minTws), m_maxTws);

    if ( m_isTransposed){
        return m_vmgInterp(tws, twa);
    }else {
        return m_vmgInterp(twa, tws);
    }
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


