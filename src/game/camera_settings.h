#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H

#define DEFAULT_CAMERA_MODE CVarGetInteger("gEnhancements.Camera.DefaultMode", 0)
#define ALTERNATE_CAMERA_MODE CVarGetInteger("gEnhancements.Camera.AlternateMode", 0)
#define HORIZONTAL_ANALOG_CAMERA CVarGetInteger("gEnhancements.Camera.HorizontalAnalog", 0)
#define VERTICAL_ANALOG_CAMERA CVarGetInteger("gEnhancements.Camera.VerticalAnalog", 0)
#define IMPROVED_C_BUTTON_CAMERA CVarGetInteger("gEnhancements.Camera.ImprovedCButtonCamera", 0)
#define CENTER_CAMERA_BUTTON CVarGetInteger("gEnhancements.Camera.CenterCameraButton", 0)
#define INVERTED_HORIZONTAL_CAMERA CVarGetInteger("gEnhancements.Camera.InvertedHorizontalCamera", 0)
#define INVERTED_VERTICAL_CAMERA CVarGetInteger("gEnhancements.Camera.InvertedVerticalCamera", 0)
#define CAMERA_SPEED CVarGetFloat("gEnhancements.Camera.CameraSpeed", 32.0)
#define CAMERA_DISTANCE CVarGetFloat("gEnhancements.Camera.CameraDistance", 100)
#define CAMERA_DISTANCE_ZOOMED_OUT CVarGetFloat("gEnhancements.Camera.CameraDistanceZoomedOut", 150)
#define ADDITIONAL_CAMERA_DISTANCE CVarGetFloat("gEnhancements.Camera.AdditionalCameraDistance", 0)

#define ANALOG_AMOUNT (12 * (1 - INVERTED_HORIZONTAL_CAMERA * 2))
#define ANALOG_AMOUNT_VERTICAL (12 * (1 - INVERTED_VERTICAL_CAMERA * 2))
#define LROTATE_SPEED 0x400

#define VERTICAL_MIN 6144.0f
#define VERTICAL_MAX 12288.0f
#define VERTICAL_MAX_LIMITED 8192.0f
#define VERTICAL_MAX_PITCH 9216.0f

#endif // CAMERA_SETTINGS_H
