#ifndef SAILVUE_PILOTCLIPOVERLAYMAKER_H
#define SAILVUE_PILOTCLIPOVERLAYMAKER_H

#include <filesystem>
#include <QPainter>
#include "navcomputer/InstrumentInput.h"
#include "navcomputer/Polars.h"
#include "OverlayElement.h"
#include "ColorPalette.h"

class PilotClipOverlayMaker : public OverlayElement {
public:
  PilotClipOverlayMaker(int width, int height, int x, int y);
  void addEpoch(QPainter &painter, const InstrumentInput &epoch) override;
private:
};


#endif //SAILVUE_PILOTCLIPOVERLAYMAKER_H
