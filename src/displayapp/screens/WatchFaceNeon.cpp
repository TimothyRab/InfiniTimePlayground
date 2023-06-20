#include "displayapp/screens/WatchFaceNeon.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
using namespace Pinetime::Applications::Screens;

WatchFaceNeon::WatchFaceNeon(Controllers::DateTime& dateTimeController,
                                                   const Controllers::Battery& batteryController,
                                                   const Controllers::Ble& bleController,
                                                   Controllers::NotificationManager& notificationManager,
                                                   Controllers::Settings& settingsController,
                                                   Controllers::HeartRateController& heartRateController,
                                                   Controllers::MotionController& motionController,
                                                   Controllers::FS& filesystem)
  : currentDateTime {{}},
    batteryIcon(false),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

  lfs_file f = {};
  if (filesystem.FileOpen(&f, "/fonts/bebas_110.bin", LFS_O_RDONLY) >= 0) {
    filesystem.FileClose(&f);
    font_bebas_110 = lv_font_load("F:/fonts/bebas_110.bin");
  }
  if (filesystem.FileOpen(&f, "/fonts/bebas_40.bin", LFS_O_RDONLY) >= 0) {
    filesystem.FileClose(&f);
    font_bebas_40 = lv_font_load("F:/fonts/bebas_40.bin");
  }


  background = lv_cont_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(background, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCF2AAD));
  lv_obj_set_style_local_radius(background, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 30);
  lv_obj_set_size(background, LV_HOR_RES, LV_VER_RES);

  sun = lv_img_create(lv_scr_act(), nullptr);
  lv_img_set_src(sun, "F:/images/neon_sun_small.bin");
  lv_obj_set_pos(sun, 0, 0);


  timeContainer = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_opa(timeContainer, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_size(timeContainer, 175, 175);
  lv_obj_align(timeContainer, lv_scr_act(), LV_ALIGN_CENTER, 60, -24);

  labelHour = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelHour, "01");
  lv_obj_set_style_local_text_font(labelHour, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_bebas_110);
  lv_obj_align(labelHour, timeContainer, LV_ALIGN_IN_TOP_MID, 0, 0);

  labelMinutes = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelMinutes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_bebas_110);
  lv_label_set_text_static(labelMinutes, "00");
  lv_obj_align(labelMinutes, timeContainer, LV_ALIGN_IN_BOTTOM_MID, 0, 0);


  dateContainer = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_opa(dateContainer, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_size(dateContainer, 60, 30);
  lv_obj_align(dateContainer, lv_scr_act(), LV_ALIGN_CENTER, 55, 90);

  labelDate = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelDate, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_obj_set_style_local_text_font(labelDate, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_bebas_40);
  lv_obj_align(labelDate, dateContainer, LV_ALIGN_IN_TOP_MID, 0, 0);
  lv_label_set_text_static(labelDate, "MON 01");



  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceNeon::~WatchFaceNeon() {
  lv_task_del(taskRefresh);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceNeon::Refresh() {
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
      } else if (hour > 12) {
        hour = hour - 12;
      }
    }

    lv_label_set_text_fmt(labelHour, "%02d", hour);
    lv_label_set_text_fmt(labelMinutes, "%02d", minute);

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(labelDate, "%s %02d", dateTimeController.DayOfWeekShortToString(), day);
      lv_obj_realign(labelDate);
    }
  }
}

bool WatchFaceNeon::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  lfs_file file = {};

  if (filesystem.FileOpen(&file, "/fonts/bebas_110.bin", LFS_O_RDONLY) < 0) {
    return false;
  }

  filesystem.FileClose(&file);
  if (filesystem.FileOpen(&file, "/fonts/bebas_40.bin", LFS_O_RDONLY) < 0) {
    return false;
  }

  filesystem.FileClose(&file);

  if (filesystem.FileOpen(&file, "/images/neon_sun_small.bin", LFS_O_RDONLY) < 0) {
    return false;
  }

  filesystem.FileClose(&file);
  return true;
}
