#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

// A definition passed by a mod, copied at the size the mod was built with. Fields added to the end of the struct
// in later API versions read as zero.
template <typename T> class SizedDef {
  public:
    SizedDef(const T* def, uint32_t size) : mValid(def != nullptr) {
        if (mValid) {
            memcpy(&mDef, def, std::min<size_t>(size, sizeof(T)));
        }
    }

    const T* get() const {
        return mValid ? &mDef : nullptr;
    }

  private:
    T mDef{};
    bool mValid;
};
