#pragma once

#ifdef __ANDROID__

// Unpacks the APK's gamedata.zip (config.yml, assets/, ghostship.o2r) into app
// storage when the packaged gamedata.version stamp changes.
bool Android_SyncPackagedData();

#endif
