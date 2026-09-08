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

// Shows an in-page prompt with a real button and opens the file dialog from that
// button's click handler. Safari only opens a file dialog from inside a user
// gesture, and the game loop is not one, so a programmatic click issued from a
// later frame is silently ignored and the Promise would never settle. The
// <input> stays in the document until the dialog reports a file or the user
// cancels, since some browsers drop the change event for a detached input.
// clang-format off
EM_ASYNC_JS(int, js_pick_into, (const char* ctitle, const char* caccept, const char* cdest), {
    var title = UTF8ToString(ctitle);
    var accept = UTF8ToString(caccept);
    var dest = UTF8ToString(cdest);
    return new Promise(function(resolve) {
        var overlay = document.createElement('div');
        overlay.style.cssText = 'position:fixed;inset:0;display:flex;align-items:center;justify-content:center;' +
                                'background:rgba(0,0,0,0.65);z-index:1000;font-family:system-ui,sans-serif;';
        var panel = document.createElement('div');
        panel.style.cssText = 'background:#1c1c24;color:#eee;padding:24px 28px;border-radius:12px;text-align:center;' +
                              'max-width:90vw;box-shadow:0 8px 32px rgba(0,0,0,0.5);';
        var label = document.createElement('p');
        label.textContent = title;
        label.style.cssText = 'margin:0 0 16px 0;font-size:1rem;';
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = accept;
        input.style.display = 'none';
        var choose = document.createElement('button');
        choose.textContent = 'Choose File';
        choose.style.cssText = 'font-size:1rem;padding:8px 18px;margin:0 6px;cursor:pointer;';
        var cancel = document.createElement('button');
        cancel.textContent = 'Cancel';
        cancel.style.cssText = choose.style.cssText;
        var settled = false;
        function finish(result) {
            if (settled) return;
            settled = true;
            if (overlay.parentNode) overlay.parentNode.removeChild(overlay);
            resolve(result);
        }
        input.addEventListener('change', function(evt) {
            var file = evt.target.files[0];
            if (!file) { finish(0); return; }
            var reader = new FileReader();
            reader.onload = function(re) {
                try {
                    FS.writeFile(dest, new Uint8Array(re.target.result));
                    finish(1);
                } catch (e) {
                    console.error('[WebFilePicker] write failed:', e);
                    finish(0);
                }
            };
            reader.onerror = function() { finish(0); };
            reader.readAsArrayBuffer(file);
        });
        input.addEventListener('cancel', function() { /* keep the prompt; the user can retry or cancel */ });
        choose.addEventListener('click', function() { input.click(); });
        cancel.addEventListener('click', function() { finish(0); });
        panel.appendChild(label);
        panel.appendChild(input);
        panel.appendChild(choose);
        panel.appendChild(cancel);
        overlay.appendChild(panel);
        document.body.appendChild(overlay);
    });
});
// clang-format on

std::string WebFilePicker_PickROM() {
    const char* vpath = "/tmp/rom.z64";
    if (!js_pick_into("Choose your Super Mario 64 ROM (.z64, .n64, or .v64)", ".z64,.n64,.v64", vpath)) {
        return "";
    }
    return vpath;
}

bool WebFilePicker_PickInto(const char* accept, const char* destPath) {
    return js_pick_into("Choose the file to load", accept, destPath) != 0;
}

#endif // __EMSCRIPTEN__
