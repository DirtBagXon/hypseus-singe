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

serialib s;
bool rts = false, saeboot = false;

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

  char device[20] = {};
  unsigned char port = get_usb_port();
  unsigned int baud = get_usb_baud();

  if (s.SupportedBaud(baud)) {
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
          return false;
  }
#endif

  int usb = s.openDevice(device, baud);

  if (usb != 1)
  {
      LOGE << "Failed to open: " << device;
      return false;
  }

  LOGI << "Opened: " << device;

  if (g_game_sae()) saeboot = true;

  s.DTR(false);
  s.RTS(true);
  rts = true;

  return true;
}

void USBScoreboard::USBShutdown() {

  if (rts) {
      rts = false;
      LOGI << "Shutting down serial";
      s.flushReceiver();
  }
  s.closeDevice();
}

USBScoreboard::USBScoreboard() { }

USBScoreboard::~USBScoreboard() { USBShutdown(); }

void USBScoreboard::Invalidate() { }

bool USBScoreboard::RepaintIfNeeded() { return false; }

bool USBScoreboard::set_digit(unsigned int uValue, WhichDigit which) {

  ds.unit = SCOREBOARD;
  ds.digit = which;
  static bool bseq, trip;
  static uint32_t buf = 0;

  if (saeboot) // SAE is a serial killer
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
              uValue = vla;
      }

      if (bseq && (which == PLAYER2_5) && (uValue == 0x0))
          bseq = false;

      if (buf > BOOTCYCLE) {
          if ((uValue > 0xb) && (which == PLAYER2_5)) {

              saeboot = false;
              DigitStruct clr;
              clr.value = spc;
              clr.unit = SCOREBOARD;

              for (char u = PLAYER2_0; u <= PLAYER2_5; u++) {
                   clr.digit = (WhichDigit)u;
                   s.writeBytes((uint8_t *)&clr, sizeof(clr));
              }
          }
      }
      if (bseq) return true;
  }

  switch(uValue) {
  case 0xa:
      ds.value = dsh;
      break;
  case 0xb:
      ds.value = vle;
      break;
  case 0xc:
      ds.value = vlh;
      break;
  case 0xd:
      ds.value = vll;
      break;
  case 0xe:
      ds.value = vlp;
      break;
  case 0xf:
      ds.value = spc;
      buf++;
      break;
  case vla:
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

  s.writeBytes((uint8_t *)&ds, sizeof(ds));
  return true;
}
 
bool USBScoreboard::is_repaint_needed() { return false; }

bool USBScoreboard::get_digit(unsigned int &uValue, WhichDigit which) {
	uValue = m_DigitValues[which];
	return true;
}

bool USBScoreboard::ChangeVisibility(bool bDontCare) { return false; }

bool g_usb_connected() { return rts; }

void send_usb_annunciator(DigitStruct ds) {
	s.writeBytes((uint8_t *)&ds, sizeof(ds));
}
