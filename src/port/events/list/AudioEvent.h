#pragma once

#include "include/types.h"
#include "audio/external.h"

// ────────────────────────────────────────────────────────────────────────────
// Audio — BGM / music events — GAME THREAD
// Cancellable: set event->Cancelled = true to block the operation.
// Mutable fields (pointers): write through them to change the value the
// engine uses before the sequence command is dispatched.
// ────────────────────────────────────────────────────────────────────────────

// Fires inside play_music before the sequence is queued. Cancellable.
// Write *seqArgs to redirect playback. Cancel to suppress entirely.
//   player    — sequence player index (SEQ_PLAYER_LEVEL, SEQ_PLAYER_ENV, …)
//   seqArgs   — mutable pointer to (priority<<8 | seqId); write to remap
//   fadeTimer — mutable pointer to fade-in duration in audio frames
DEFINE_EVENT(PlayMusicEvent,
    u8    player;
    u16*  seqArgs;
    u16*  fadeTimer;
);

// Fires inside stop_background_music before the sequence is removed. Cancellable.
// Cancel to keep the sequence running.
//   seqId — mutable pointer to the sequence ID being stopped
DEFINE_EVENT(StopMusicEvent,
    u16* seqId;
);

// Fires inside fadeout_background_music before the fade is applied. Cancellable.
// Cancel to suppress the fadeout.
//   seqId   — mutable pointer to the sequence ID being faded out
//   fadeOut — mutable pointer to the fade duration in audio frames
DEFINE_EVENT(FadeoutMusicEvent,
    u16* seqId;
    u16* fadeOut;
);

// Fires inside play_secondary_music before the ENV player is started. Cancellable.
// Cancel to suppress secondary music playback.
//   seqId          — mutable pointer to the sequence ID
//   bgMusicVolume  — mutable pointer to the BGM duck volume
//   volume         — mutable pointer to the ENV player target volume
//   fadeTimer      — mutable pointer to the fade duration
DEFINE_EVENT(PlaySecondaryMusicEvent,
    u8*  seqId;
    u8*  bgMusicVolume;
    u8*  volume;
    u16* fadeTimer;
);

// ────────────────────────────────────────────────────────────────────────────
// Audio — SFX event — GAME THREAD — Cancellable
// ────────────────────────────────────────────────────────────────────────────

// Fires inside play_sound before the request is enqueued. Cancellable.
// Write *soundBits to remap to a different sound effect.
// Cancel to suppress the sound entirely.
//   soundBits — mutable sound bits (bank | soundId | priority | flags)
//   pos       — world-space source position (3 floats)
DEFINE_EVENT(PlaySfxEvent,
    s32* soundBits;
    f32* pos;
);

// ────────────────────────────────────────────────────────────────────────────
// Audio — dialog sound event — GAME THREAD — Cancellable
// ────────────────────────────────────────────────────────────────────────────

// Fires inside play_dialog_sound before the dialog SFX is played. Cancellable.
//   dialogID — mutable pointer to the dialog ID
DEFINE_EVENT(PlayDialogSoundEvent,
    u8* dialogID;
);

// ────────────────────────────────────────────────────────────────────────────
// Audio — per-frame tick — GAME THREAD
// ────────────────────────────────────────────────────────────────────────────

// Fires at the end of audio_signal_game_loop_tick every frame, after all
// sound requests and sequence commands have been processed.
// Use for per-frame audio mod work that must run in lockstep with the game.
// Not cancellable.
DEFINE_EVENT(AudioUpdateEvent);

// ────────────────────────────────────────────────────────────────────────────
// Audio — note-level hooks — AUDIO THREAD
// These fire deep inside the sequence player on the audio thread.
// Do not call any game-thread APIs or take game-thread locks here.
// ────────────────────────────────────────────────────────────────────────────

// Fires just before a note is allocated for a sequence layer.
// Used to swap layer->sound for a custom PCM buffer and adjust
// layer->noteFreqMod for sample-rate matching.
//   layer   — the sequence layer about to trigger a note
//   channel — the channel that owns this layer
DEFINE_EVENT(SeqLayerPreNoteEvent,
    struct SequenceChannelLayer*  layer;
    struct SequenceChannel*       channel;
);

// Fires immediately after a note has been allocated for a sequence layer.
// Used to pin delay/gateDelay so the engine does not cut a custom sample short.
//   layer — the layer whose note was just allocated
DEFINE_EVENT(SeqLayerPostNoteEvent,
    struct SequenceChannelLayer* layer;
);
