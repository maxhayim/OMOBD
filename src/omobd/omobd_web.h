/*
 * omobd_web.h - OMOBD Wi-Fi web dashboard (ESP32-S3)
 *
 * Serves the browser dashboard and a small JSON API from the device itself,
 * so a phone, tablet or laptop can watch the engine with no internet and no
 * app install. Monitoring only: nothing here can drive a vehicle output.
 *
 * Compiled only when the board defines OMOBD_WEB_DASHBOARD (see
 * boards/esp32s3_omobd/board.h). main.cpp calls the two functions below.
 */
#ifndef OMOBD_WEB_H
#define OMOBD_WEB_H

#include <stdint.h>

#if defined(OMOBD_WEB_DASHBOARD) && OMOBD_WEB_DASHBOARD

// Call once from setup(), after the input manager and rules are loaded.
void omobdWebInit();
// Call every loop(). Non-blocking: serves at most one pending HTTP request,
// runs the calculated values, samples history and supervises Wi-Fi.
void omobdWebUpdate(uint32_t now);

#else
inline void omobdWebInit() {}
inline void omobdWebUpdate(uint32_t) {}
#endif

#endif // OMOBD_WEB_H
