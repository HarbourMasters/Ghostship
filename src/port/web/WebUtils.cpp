#ifdef __EMSCRIPTEN__
#include "WebUtils.h"

#include <emscripten.h>
#include <emscripten/emscripten.h>
#include <string>
#include <cstdlib>

EM_JS(void, js_idbfs_mount, (const char* cpath), {
    var path = UTF8ToString(cpath);
    try {
        FS.mkdir(path);
    } catch (e) {}
    FS.mount(IDBFS, {}, path);
});

// Sync from IndexedDB → virtual FS (populate = true)
EM_ASYNC_JS(void, js_idbfs_load, (), {
    return new Promise(function(resolve, reject) {
        FS.syncfs(
            true, function(err) {
                if (err) {
                    console.error('[WebCache] load error:', err);
                }
                resolve();
            });
    });
});

// Sync from virtual FS → IndexedDB (populate = false)
EM_ASYNC_JS(void, js_idbfs_save, (), {
    return new Promise(function(resolve, reject) {
        FS.syncfs(
            false, function(err) {
                if (err) {
                    console.error('[WebCache] save error:', err);
                }
                resolve();
            });
    });
});

void WebCache_Mount(const char* path) {
    static bool mounted = false;
    if (mounted)
        return;
    js_idbfs_mount(path);
    mounted = true;
}

void WebCache_Load() {
    js_idbfs_load();
}

void WebCache_Save() {
    js_idbfs_save();
}

EM_ASYNC_JS(char*, js_pick_rom, (), {
    return new Promise(function(resolve) {
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = '.z64,.n64,.v64';

        input.addEventListener(
            'change', function(evt) {
                var file = evt.target.files[0];
                if (!file) {
                    resolve(0);
                    return;
                }

                var reader = new FileReader();
                reader.onload = function(re) {
                    var data = new Uint8Array(re.target.result);
                    var vpath = '/tmp/rom.z64';
                    FS.writeFile(vpath, data);
                    // Return the path as a malloc'd C string; caller must free().
                    var len = lengthBytesUTF8(vpath) + 1;
                    var ptr = _malloc(len);
                    stringToUTF8(vpath, ptr, len);
                    resolve(ptr);
                };
                reader.onerror = function() {
                    resolve(0);
                };
                reader.readAsArrayBuffer(file);
            });

        // If the dialog is closed without a selection the Promise never fires
        // otherwise, so also listen on the body focus-return as a heuristic.
        input.addEventListener('cancel', function() { resolve(0); });

        input.style.display = 'none';
        document.body.appendChild(input);
        input.click();
        // Remove from DOM immediately; the file dialog stays open.
        setTimeout(
            function() {
                if (input.parentNode)
                    input.parentNode.removeChild(input);
            },
            500);
    });
});

std::string WebFilePicker_PickROM() {
    char* ptr = js_pick_rom();
    if (!ptr)
        return "";
    std::string path(ptr);
    free(ptr);
    return path;
}

EM_ASYNC_JS(int, js_pick_into, (const char* caccept, const char* cdest), {
    var accept = UTF8ToString(caccept);
    var dest = UTF8ToString(cdest);
    return new Promise(function(resolve) {
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = accept;

        input.addEventListener(
            'change', function(evt) {
                var file = evt.target.files[0];
                if (!file) {
                    resolve(0);
                    return;
                }
                var reader = new FileReader();
                reader.onload = function(re) {
                    var data = new Uint8Array(re.target.result);
                    try {
                        FS.writeFile(dest, data);
                        resolve(1);
                    } catch (e) {
                        console.error('[WebFilePicker] write failed:', e);
                        resolve(0);
                    }
                };
                reader.onerror = function() {
                    resolve(0);
                };
                reader.readAsArrayBuffer(file);
            });
        input.addEventListener('cancel', function() { resolve(0); });

        input.style.display = 'none';
        document.body.appendChild(input);
        input.click();
        setTimeout(
            function() {
                if (input.parentNode)
                    input.parentNode.removeChild(input);
            },
            500);
    });
});

bool WebFilePicker_PickInto(const char* accept, const char* destPath) {
    return js_pick_into(accept, destPath) != 0;
}

#endif // __EMSCRIPTEN__
