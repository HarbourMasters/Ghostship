#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H
#include "port/features/Features.h"

#define DEFAULT_CAMERA_MODE Port_FeatureInt("camera.default_mode", 0)
#define ALTERNATE_CAMERA_MODE Port_FeatureInt("camera.alternate_mode", 0)
#define HORIZONTAL_ANALOG_CAMERA Port_FeatureInt("camera.horizontal_analog", 0)
#define VERTICAL_ANALOG_CAMERA Port_FeatureInt("camera.vertical_analog", 0)
#define IMPROVED_C_BUTTON_CAMERA Port_FeatureInt("camera.improved_c_buttons", 0)
#define CENTER_CAMERA_BUTTON Port_FeatureInt("camera.center_button", 0)
#define INVERTED_HORIZONTAL_CAMERA CVarGetInteger("gEnhancements.Camera.InvertedHorizontalCamera", 0)
#define INVERTED_VERTICAL_CAMERA CVarGetInteger("gEnhancements.Camera.InvertedVerticalCamera", 0)
#define CAMERA_SPEED CVarGetFloat("gEnhancements.Camera.CameraSpeed", 32.0)
#define CAMERA_DISTANCE CVarGetFloat("gEnhancements.Camera.CameraDistance", 100)
#define CAMERA_DISTANCE_ZOOMED_OUT CVarGetFloat("gEnhancements.Camera.CameraDistanceZoomedOut", 150)
#define ADDITIONAL_CAMERA_DISTANCE CVarGetFloat("gEnhancements.Camera.AdditionalCameraDistance", 0)
#define MANUAL_CAMERA_SOUNDS CVarGetInteger("gEnhancements.Camera.ManualCameraSounds", 1)

#define ANALOG_AMOUNT (12 * (1 - INVERTED_HORIZONTAL_CAMERA * 2))
#define ANALOG_AMOUNT_VERTICAL (12 * (1 - INVERTED_VERTICAL_CAMERA * 2))
#define LROTATE_SPEED 0x400

#define VERTICAL_MIN 6144.0f
#define VERTICAL_MAX 12288.0f
#define VERTICAL_MAX_LIMITED 8192.0f
#define VERTICAL_MAX_PITCH 9216.0f

#endif // CAMERA_SETTINGS_H
