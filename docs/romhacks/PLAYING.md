# Playing a romhack

This guide walks through playing a romhack in Ghostship, from downloading it to switching back to the base game.

## Before you start

You need:

1. **Ghostship already set up with the base game.** The first time you started Ghostship, it asked for your Super Mario 64 ROM and generated `sm64.o2r`. A romhack loads on top of those files, so this has to be done first.
2. **The Super Mario 64 ROM the hack was made for** (`.z64`), usually the US version.
3. **A supported romhack.** Open **Settings → Romhacks** in Ghostship. Every romhack under **Supported Romhacks** has a link button that opens its download page. Only the listed version of each hack is supported.

   A romhack Ghostship doesn't ship with can come as a **recipe package**, a small `.o2r` holding the instructions for extracting it (no game data). Put it in the `romhacks/` folder inside the Ghostship folder and it shows up in the list.

## The quick way: Generate from your Super Mario 64 ROM

Some romhacks under **Supported Romhacks** have a **Generate** button: their package carries the hack's patch, so Ghostship patches your own ROM for you.

1. Go to **Settings → Romhacks** and click **Generate** next to the romhack.
2. Pick your **Super Mario 64 ROM** (the same one you set Ghostship up with). Ghostship extends it if the hack needs that, applies the patch, checks the result is the supported version, and extracts the romhack. This only has to be done once.
3. Continue from step 5 of [Step 2](#step-2-add-the-romhack-to-ghostship).

For a romhack without a **Generate** button, patch it yourself:

## Step 1: Patch the romhack

Romhacks are downloaded as a patch file (`.bps`, `.ips`, `.ppf` or `.xdelta`). You apply the patch to your own ROM to get the romhack ROM.

1. Download the romhack from its page.
2. If the hack's instructions say to extend the ROM first (with M64 ROM Extender or sm64extend), do that with the default settings. Older hacks made with ROM Manager need this.
3. Open a ROM patching tool, for example [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/).
4. Pick your **Super Mario 64 ROM** (the extended one, if step 2 applied) as the ROM file and the romhack's **patch file** as the patch.
5. Apply the patch and save the result, e.g. `My Hack.z64`.

## Step 2: Add the romhack to Ghostship

1. Start Ghostship and open the menu.
2. Go to **Settings → Romhacks**.
3. Click **Generate Romhack from ROM** and pick the patched ROM from step 1.
4. Wait for the progress bar to finish. This only has to be done once per romhack.
5. When **Romhack Ready** shows up, click **Play** and quit Ghostship, or **Later** to keep playing what you're playing now.
6. Start Ghostship again. It loads the romhack.

While a romhack is active, the menu header shows **Romhack active: NAME**.

## Switching romhacks, or going back to the base game

Only one romhack can be active at a time. To change it:

1. Go to **Settings → Romhacks → Installed Romhacks**.
2. Click **Edit**.
3. Use the arrow buttons to move a romhack into **Enabled Romhack**, or out of it.
   - To play the base game again, move the active romhack to **Disabled Romhacks**, or click **Clear List**.
4. Click **Apply**, then restart Ghostship.

To play a romhack for a single launch without changing the saved choice, start Ghostship with `-hack <name>`, where `<name>` is the romhack's file name without `.o2r`.

## Saves

Each romhack keeps its own save files, apart from the base game and from other romhacks, in `saves/~romhacks/<name>/`. Switching between them never overwrites another game's saves.

## Mods and texture packs

Mods still work while a romhack is active. They load on top of the romhack, so a mod that changes something the romhack also changed wins. A mod made for the base game may not fit a romhack that changed the same levels.

A romhack or mod can turn off Ghostship options that would break it, or pin them to a value. Those options are greyed out, and hovering one says which romhack or mod did it; your own setting comes back when you play without it. The randomizer is turned off while any romhack is active. A mod that says it only works with another base (the base game or a different romhack) isn't loaded, and **Mods › General** says why.

## Common errors

| Message | What it means | What to do |
|---|---|---|
| **Wrong ROM** | You used **Generate** with a ROM that isn't the one the hack's patch was made for. | Pick the original, unmodified Super Mario 64 ROM (usually the US version), or the hack's already patched `.z64`. |
| **Not a Supported Romhack** | Ghostship has no recipe for the ROM you picked. | Check that the romhack is in the **Supported Romhacks** list, that you patched the right base ROM, and that you picked the patched `.z64` and not the original. Different versions of the same romhack give different ROMs. |
| **Base Game Required** | You picked a romhack ROM when Ghostship asked for the base game. | Pick your original Super Mario 64 ROM first. Add the romhack afterwards through **Settings → Romhacks**. |
| **Romhack Extraction Failed** | Something went wrong while generating the romhack files. | Try again. If it keeps failing, report it with the message shown. |

## Where the files are

All paths are inside the Ghostship folder.

| What | Where |
|---|---|
| Romhack files | `mods/~romhacks/<name>.o2r` |
| Recipe packages | `romhacks/<name>.o2r` (unpacked into `cache/romhacks/<name>/` while extracting) |
| Romhack saves | `saves/~romhacks/<name>/` |

Folders starting with `~` are managed by Ghostship. Deleting a romhack's `.o2r` removes the romhack; its saves stay in place and come back if you add the romhack again.
