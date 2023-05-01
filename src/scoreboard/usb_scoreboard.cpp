/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2022 DirtBagXon, mcspaeth
 *
 * This file is part of HYPSEUS, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "usb_scoreboard.h"
#include <string.h>
#include "../io/serialib.h"
#include "../game/lair_util.h"
#include "../hypseus.h"
#include <plog/Log.h>
#include <cstdio>

serialib g_usb_serial;
bool g_serial_rts = false, g_serial_saeboot = false;

void USBScoreboard::DeleteInstance() { delete this; }

IScoreboard *USBScoreboard::GetInstance() {
  USBScoreboard *uRes = 0;

  uRes = new USBScoreboard();
  
  // Try to init, if this fails, we fail
  if (!uRes->USBInit() || !uRes->Init()) {
    uRes->DeleteInstance();
    uRes = 0;
  }

  return uRes;
}

bool USBScoreboard::USBInit() {

  char device[20] = {0};
  unsigned char port = get_usb_port();
  unsigned int baud = get_usb_baud();

  if (g_game_sae())
  {
      if (g_game_fastboot()) return false;
      g_serial_saeboot = true;
  }

  if (g_usb_serial.SupportedBaud(baud)) {
     LOGI << "Setting BAUD rate: " << baud;
  } else {
     LOGE << "Aborting: Unsupported BAUD rate: " << baud;
     return false;
  }

#ifdef WIN32

  snprintf(device, sizeof(device), "COM%d", port);

#elif defined(__linux__)

  unsigned char type = get_usb_impl();

  switch(type)
  {
      case 1:
          snprintf(device, sizeof(device), "/dev/ttyUSB%d", port);
          break;
      case 2:
          snprintf(device, sizeof(device), "/dev/ttyACM%d", port);
          break;
      default:
          LOGE << "Aborting: no USB device setup was specified";
          return false;
  }
#endif

  int usb = g_usb_serial.openDevice(device, baud);

  struct timespec delta = {0, 10000000};
  nanosleep(&delta, &delta);

  if (usb != 1)
  {
      LOGE << "Failed to open USB device: " << device;
      return false;
  }

  g_usb_serial.flushReceiver();
  LOGI << "Opened: " << device;

  g_usb_serial.DTR(false);
  g_usb_serial.RTS(true);
  g_serial_rts = true;

  nanosleep(&delta, &delta);
  g_usb_serial.flushReceiver();

  return true;
}

void USBScoreboard::USBShutdown() {

  if (g_serial_rts) {
      g_serial_rts = false;
      LOGI << "Shutting down serial";
      g_usb_serial.flushReceiver();
  }
  g_usb_serial.closeDevice();
}

USBScoreboard::USBScoreboard() { }

USBScoreboard::~USBScoreboard() { USBShutdown(); }

void USBScoreboard::Invalidate() { }

bool USBScoreboard::RepaintIfNeeded() { return false; }

bool USBScoreboard::set_digit(unsigned int uValue, WhichDigit which) {

  USBUtil serial;
  ds.unit = SCOREBOARD;
  ds.digit = which;
  static bool bseq, trip;
  static uint32_t buf = 0;

  if (g_serial_saeboot) // SAE is a serial killer
  {
      if (get_usb_baud() < LOWBAUD) {
          if (buf < BOOTBYPASS) {
              buf++;
              return true;
          }
      }

      if (buf < BOOTCYCLE) {
          if ((uValue == 0x5) && (which == PLAYER2_5)) {

              bseq = true;
              return true;

           } else if (which == PLAYER2_1 && ((uValue == 0xc)
                      || (uValue == 0xe)))
              uValue = s_asc_vla;
      }

      if (bseq && (which == PLAYER2_5) && (uValue == 0x0))
          bseq = false;

      if (buf > BOOTCYCLE) {
          if ((uValue > 0xb) && (which == PLAYER2_5)) {

              g_serial_saeboot = false;
              DigitStruct clr;
              clr.value = s_asc_spc;
              clr.unit = SCOREBOARD;

              for (char u = PLAYER2_0; u <= PLAYER2_5; u++) {
                   clr.digit = (WhichDigit)u;
                   serial.write_usb(clr);
              }
          }
      }
      if (bseq) return true;
  }

  switch(uValue) {
  case 0xa:
      ds.value = s_asc_dsh;
      break;
  case 0xb:
      ds.value = s_asc_vle;
      break;
  case 0xc:
      ds.value = s_asc_vlh;
      break;
  case 0xd:
      ds.value = s_asc_vll;
      break;
  case 0xe:
      ds.value = s_asc_vlp;
      break;
  case 0xf:
      ds.value = s_asc_spc;
      buf++;
      break;
  case s_asc_vla:
      ds.value = uValue;
      break;
  default:
      ds.value = ASCI(uValue);
      buf++;
      break;
  }

  if (!trip) {
      if (buf < STARTDELAY) return true;
      else trip = true;
  }

  serial.write_usb(ds);
  return true;
}
 
bool USBScoreboard::is_repaint_needed() { return false; }

bool USBScoreboard::get_digit(unsigned int &uValue, WhichDigit which) {
	uValue = m_DigitValues[which];
	return true;
}

bool USBScoreboard::ChangeVisibility(bool bDontCare) { return false; }

bool USBUtil::usb_connected() { return g_serial_rts; }

void USBUtil::write_usb(DigitStruct ds) {
	g_usb_serial.writeBytes((uint8_t *)&ds, sizeof(ds));
}
