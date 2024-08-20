#ifndef SAILVUE_COLOR_PALETTE_H
#define SAILVUE_COLOR_PALETTE_H

#include <QPainter>
#include <QPen>
#include <QColor>

// Common colors
static const QColor GRID_COLOR = QColor::fromString("#7F706C62");

// Polar overlay colors and pens

static const QColor POLAR_ARM_COLOR = QColor::fromString("#F2C356");
static const QColor POLAR_CURVE_COLOR = QColor::fromString("#0D98F2");
static const QColor POLAR_GRID_COLOR = GRID_COLOR;
static const QColor POLAR_PILOT_TWA_COLOR = QColor::fromString("#726E63");
static const QColor POLAR_HISTORY_COLOR = QColor::fromString("#F16704");

static const QPen POLAR_GRID_PEN = QPen(POLAR_GRID_COLOR, 2);
static const QPen POLAR_ARM_PEN = QPen(POLAR_ARM_COLOR, 4);
static const QPen POLAR_CURVE_PEN = QPen(POLAR_CURVE_COLOR, 6);
static const QPen POLAR_PILOT_TWA_PEN =  QPen(POLAR_PILOT_TWA_COLOR, 4);

// Rudder overlay colors and pens

static const QColor RUDDER_GRID_COLOR = GRID_COLOR;
static const QColor RUDDER_RUDDER_COLOR = POLAR_ARM_COLOR;
static const QColor RUDDER_RUDDER_FONT_COLOR = QColor::fromString("#F2D1A5");
static const QColor RUDDER_AUTO_COLOR = POLAR_PILOT_TWA_COLOR;
static const QColor RUDDER_AUTO_FONT_COLOR = POLAR_PILOT_TWA_COLOR;

static const QPen RUDDER_GRID_PEN = QPen(RUDDER_GRID_COLOR, 2);
static const QPen RUDDER_PEN = QPen(RUDDER_RUDDER_COLOR, 15, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
static const QPen RUDDER_FONT_PEN = QPen(RUDDER_RUDDER_FONT_COLOR);
static const QPen AUTO_PEN = QPen(RUDDER_AUTO_COLOR, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
static const QPen AUTO_FONT_PEN = QPen(RUDDER_AUTO_FONT_COLOR);

// Start overlay colors and pens
static const QColor BEFORE_START_COLOR = QColor::fromString("#F16704");
static const QColor AFTER_START_COLOR = QColor::fromString("#eef8fe");
static const QPen BEFORE_START_PEN = QPen(BEFORE_START_COLOR);
static const QPen AFTER_START_PEN = QPen(AFTER_START_COLOR);

static const QColor PERFORMANCE_SLOW_COLOR = QColor::fromString("#FFF16704");
static const QColor PERFORMANCE_FAST_COLOR = QColor::fromString("#DC726E63");

static const QPen FAST_TIME_PEN = QPen(PERFORMANCE_FAST_COLOR);
static const QPen SLOW_TIME_PEN = QPen(PERFORMANCE_SLOW_COLOR);
static const QPen PERF_LABEL_PEN = QPen(GRID_COLOR);

static const char *const FONT_FAMILY = "Consolas";
static const char *const FONT_FAMILY_TIMESTAMP = FONT_FAMILY;
static const char *const FONT_FAMILY_TIME = FONT_FAMILY;
static const char *const FONT_FAMILY_VALUE = FONT_FAMILY;
static const char *const FONT_FAMILY_LABEL = FONT_FAMILY;

static const int RUDDER_DOT_RADIUS = 6;
static const int HIST_DISPLAY_LEN_MS = 1000 * 30;

#endif //SAILVUE_COLOR_PALETTE_H
