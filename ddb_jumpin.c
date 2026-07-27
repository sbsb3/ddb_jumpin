// ddb_jumpin — "Jump In" plugin for DeaDBeeF
//
// Port of the foobar2000 component foo_jumpin: advances to the next track
// and seeks to a user-configured, optionally randomized time offset.
// A track tagged JUMPIN=0 opts out of the seek.

#include <deadbeef/deadbeef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static DB_functions_t *deadbeef;

static int s_seek_pending = 0;
static double s_seek_offset = 0.0;

static double
compute_jump_offset (void) {
    double lo = deadbeef->conf_get_float ("jumpin.min_seconds", 8.0f);
    double hi = deadbeef->conf_get_float ("jumpin.max_seconds", 20.0f);
    if (lo > hi) {
        double t = lo;
        lo = hi;
        hi = t;
    }
    double offset = lo;
    if (deadbeef->conf_get_int ("jumpin.randomize", 1) && hi > lo) {
        offset = lo + ((double)rand () / (double)RAND_MAX) * (hi - lo);
    }
    return offset;
}

static int
jumpin_action_callback (DB_plugin_action_t *action, ddb_action_context_t ctx) {
    // Mirror foo_jumpin's greyed-out state: only act while playing or paused
    DB_output_t *output = deadbeef->get_output ();
    if (!output || output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        return 0;
    }

    // Arm the deferred seek BEFORE requesting the next track, so the
    // DB_EV_SONGSTARTED handler catches the very first new-track event.
    s_seek_offset = compute_jump_offset ();
    s_seek_pending = 1;

    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    return 0;
}

static DB_plugin_action_t jumpin_action = {
    .title = "Playback/Jump In (Next Track)",
    .name = "jumpin_next",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = jumpin_action_callback,
    .next = NULL,
};

// Lets `deadbeef --plugin=jumpin` (e.g. from an XFCE global shortcut) trigger
// the action, now that DeaDBeeF 1.10.1+ dropped its own global hotkeys.
static int
jumpin_exec_cmdline (const char *cmdline, int cmdline_size, ddb_response_t *response) {
    (void)cmdline;
    (void)cmdline_size;
    jumpin_action_callback (&jumpin_action, DDB_ACTION_CTX_MAIN);
    if (response) {
        const char msg[] = "jumpin: triggered\n";
        response->append (response, (char *)msg, sizeof (msg) - 1);
    }
    return 0;
}

static DB_plugin_action_t *
jumpin_get_actions (DB_playItem_t *it) {
    return &jumpin_action;
}

static void
jumpin_handle_songstarted (ddb_event_track_t *ev) {
    if (!s_seek_pending) {
        return;
    }
    s_seek_pending = 0;

    if (!ev || !ev->track) {
        return;
    }

    // Honour a JUMPIN=0 tag: track opts out of the seek
    int skip = 0;
    deadbeef->pl_lock ();
    const char *val = deadbeef->pl_find_meta (ev->track, "JUMPIN");
    if (val && !strcmp (val, "0")) {
        skip = 1;
    }
    deadbeef->pl_unlock ();
    if (skip) {
        return;
    }

    double pos = s_seek_offset;
    if (deadbeef->conf_get_int ("jumpin.clamp_to_pct", 1)) {
        float len = deadbeef->pl_get_item_duration (ev->track);
        if (len > 0 && pos > len * 0.80) {
            pos = len * 0.80;
        }
    }

    if (pos > 1.0) {
        deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(pos * 1000.0), 0);
    }
}

static int
jumpin_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGSTARTED:
        jumpin_handle_songstarted ((ddb_event_track_t *)ctx);
        break;
    case DB_EV_STOP:
        s_seek_pending = 0;
        break;
    }
    return 0;
}

static int
jumpin_start (void) {
    srand ((unsigned)time (NULL));
    return 0;
}

static const char settings_dlg[] =
    "property \"Randomize offset\" checkbox jumpin.randomize 1;\n"
    "property \"Minimum seconds\" entry jumpin.min_seconds 8;\n"
    "property \"Maximum seconds\" entry jumpin.max_seconds 20;\n"
    "property \"Clamp to 80% of track length\" checkbox jumpin.clamp_to_pct 1;\n";

static DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.id = "jumpin",
    .plugin.name = "Jump In",
    .plugin.descr =
        "Advances to the next track and seeks to a randomized user-configured offset.\n"
        "Tag a track with JUMPIN=0 to opt it out of the seek.\n"
        "Bind the \"Jump In (Next Track)\" action to a hotkey, or use the Playback menu.\n"
        "For best crossfader results, set minimum offset >= crossfade duration.",
    .plugin.copyright =
        "Copyright (C) 2026 sb\n"
        "\n"
        "This software is provided 'as-is', without any express or implied warranty.",
    .plugin.start = jumpin_start,
    .plugin.message = jumpin_message,
    .plugin.get_actions = jumpin_get_actions,
    .plugin.configdialog = settings_dlg,
    .plugin.exec_cmdline = jumpin_exec_cmdline,
};

DB_plugin_t *
ddb_jumpin_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
