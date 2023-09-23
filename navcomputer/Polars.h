#ifndef SAILVUE_POLARS_H
#define SAILVUE_POLARS_H

#include <string>
#include <libInterpolate/Interpolate.hpp>

class Polars {
public:
    void loadPolar(const std::string &path);
    double getSpeed(double twa, double tws);
    // Returns optimal <twa,vmg> for given tws
    std::pair<double, double>  getTargets(double tws, bool isUpwind);
    [[nodiscard]] double getMinTwa() const { return m_minTwa; }
    [[nodiscard]] double getMaxTwa() const { return m_maxTwa; }

private:
    void loadPolarCsv(const std::string &path);
    void loadPolarPol(const std::string &path);
    double getVmg(double twa, double tws);
private:
    _2D::BilinearInterpolator<double> m_speedInterp;
    _2D::BilinearInterpolator<double> m_vmgInterp;
    double m_minTwa;
    double m_maxTwa;
    double m_minTws;
    double m_maxTws;
    bool m_isTransposed = false;
};


#endif //SAILVUE_POLARS_H
