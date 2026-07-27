# ddb_jumpin — "Jump In" plugin for DeaDBeeF

Advances playback to the next track and immediately seeks to a
user-configured, optionally randomized time offset — e.g. jumping into a
track at 12 seconds instead of the beginning. Designed to work alongside a
crossfader/DSP setup: the fade lands on the offset position, not the raw
start of the file.

This is a Linux/DeaDBeeF port of the foobar2000 component `foo_jumpin`.

## Behaviour

Triggering the action (menu, hotkey, or CLI — see below):

1. Does nothing if playback is stopped (only acts while playing/paused).
2. Computes a jump offset:
   - If **Randomize offset** is on, picks a uniform random value between
     **Minimum seconds** and **Maximum seconds**.
   - If off, always uses **Minimum seconds**.
3. Advances to the next track (`DB_EV_NEXT`).
4. When the new track starts, seeks to the computed offset — unless:
   - The track is tagged `JUMPIN=0`, which opts it out of the seek entirely
     (useful for intros, short clips, etc.).
   - **Clamp to 80% of track length** is on and the offset would land past
     80% of the track's duration, in which case it's clamped down to 80%.
   - The resulting position is ≤ 1 second, in which case no seek happens
     (not worth it).

## Configuration

Available via DeaDBeeF's plugin configuration dialog (**Edit → Preferences →
Plugins → Jump In**):

| Setting | Config key | Default |
|---|---|---|
| Randomize offset | `jumpin.randomize` | on |
| Minimum seconds | `jumpin.min_seconds` | 8 |
| Maximum seconds | `jumpin.max_seconds` | 20 |
| Clamp to 80% of track length | `jumpin.clamp_to_pct` | on |

For best results with a crossfader/DSP, set the minimum offset ≥ your fade
duration, so the fade never runs into silence from before the seek point.

## Triggering it

The action is registered as `Playback/Jump In (Next Track)` and shows up:

- In the DeaDBeeF **Playback** menu.
- Via any DeaDBeeF-internal mechanism that can invoke a named plugin action
  (e.g. the Hotkeys plugin's *local*, in-window bindings).

### Global hotkey (X11 / XFCE)

DeaDBeeF 1.10.1+ removed global hotkey support from its Hotkeys plugin (see
[upstream issue #3306](https://github.com/DeaDBeeF-Player/deadbeef/issues/3306)).
To trigger Jump In from a system-wide shortcut regardless of window focus,
this plugin implements `exec_cmdline`, so it can be invoked straight from the
command line and forwarded to the already-running DeaDBeeF instance:

```sh
deadbeef --plugin=jumpin
```

Bind that command to a key combo in **Settings → Keyboard → Application
Shortcuts** in XFCE (or the equivalent in any other X11 WM/DE). No arguments
are parsed — any invocation just triggers the action.

## Building

Requires the DeaDBeeF SDK headers (checked out as a sibling `../deadbeef`
directory — see `Makefile`'s `-I../deadbeef/include`).

```sh
make            # builds ddb_jumpin.so
make install    # copies it to ~/.local/lib/deadbeef/
make clean
```

Restart DeaDBeeF after installing/updating so it picks up the new build.

## Files

- `ddb_jumpin.c` — the entire plugin (single file).
- `Makefile` — build/install/clean targets.
