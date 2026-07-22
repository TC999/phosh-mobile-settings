/*
 * Copyright (c) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Inspired by GNOME Software's gs-plugin-systemd-sysupdate.c
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-systemd-sysupdate"

#include "mobile-settings-config.h"

#include "ms-os-update-priv.h"
#include "ms-os-updater-priv.h"
#include "ms-dbus-systemd-sysupdated.h"
#include "ms-systemd-sysupdate.h"

#include <json-glib/json-glib.h>

#include <gio/gio.h>

/**
 * MsSystemdSysupdate:
 *
 * Perform host updates via systemd's sysupdated sysupdate1 DBus
 * interface
 */

/* We can use the session bus for easy mocking */
#if 0
# define MS_DBUS_BUS (G_BUS_TYPE_SESSION)
#warning "sysupdate1 is using session bus"
#else
# define MS_DBUS_BUS (G_BUS_TYPE_SYSTEM)
#endif


struct _MsSystemdSysupdate {
  MsOsUpdater             parent;

  MsDBusSysupdateManager *manager;
  GDBusConnection        *bus;
  GCancellable           *cancel;
  GHashTable             *finished_jobs;
  GPtrArray *targets;
};
G_DEFINE_TYPE (MsSystemdSysupdate, ms_systemd_sysupdate, MS_TYPE_OS_UPDATER)


typedef struct _Target {
  MsDBusSysupdateTarget *target;
  MsSystemdSysupdate    *sysupdate; /* unowned */
  GCancellable          *cancel;

  char      *class;
  char      *name;
  GPtrArray *versions;
} Target;


typedef struct _Version {
  char       *version;
  Target     *target;
  gboolean    newest;
  gboolean    available;
  gboolean    installed;
  gboolean    obsolete;

  MsDBusSysupdateJob *job_proxy;
  guint64     job_id;
  gulong      progress_id;

  /* The task associated with the current job */
  GTask      *task;
  /* The task for cancelling the current job */
  GTask      *cancel_task;
  MsOsUpdate *os_update;
} Version;


static void version_destroy (Version *version);

static Target *
target_new (const char *name, const char *class, MsSystemdSysupdate *sysupdate)
{
  Target *target = g_new0 (Target, 1);

  target->sysupdate = sysupdate;
  target->cancel = g_cancellable_new ();
  target->name = g_strdup (name);
  target->class = g_strdup (class);
  target->versions = g_ptr_array_new_with_free_func ((GDestroyNotify)version_destroy);

  return target;
}


static void
target_destroy (Target *target)
{
  g_cancellable_cancel (target->cancel);
  g_clear_object (&target->cancel);

  g_clear_object (&target->target);

  g_clear_pointer (&target->name, g_free);
  g_clear_pointer (&target->class, g_free);
  g_clear_pointer (&target->versions, g_ptr_array_unref);

  g_free (target);
}


static Version *
version_new (const char *ver, Target *target)
{
  Version *version = g_new0 (Version, 1);

  version->version = g_strdup (ver);
  version->target = target;

  return version;
}


static void
clear_job_state (Version *version)
{
  g_clear_signal_handler (&version->progress_id, version->job_proxy);
  g_clear_object (&version->job_proxy);
  version->job_id = 0;
}


static void
version_destroy (Version *version)
{
  g_clear_pointer (&version->version, g_free);

  clear_job_state (version);

  g_clear_object (&version->os_update);
  g_clear_object (&version->task);

  g_free (version);
}


static void
on_job_progress (Version *version, GParamSpec *pspec, MsDBusSysupdateJob *job_proxy)
{
  guint progress = ms_dbus_sysupdate_job_get_progress (job_proxy);

  g_assert (MS_IS_OS_UPDATE (version->os_update));

  g_debug ("Job %" G_GUINT64_FORMAT " progress: %u", version->job_id, progress);
  ms_os_update_set_progress (version->os_update, progress);
}


static void
job_finished (Version *version, int status)
{
  MsOsUpdateState state;
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (G_IS_TASK (version->task));
  task = g_steal_pointer (&version->task);

  /* Clear progress, etc */
  clear_job_state (version);

  state = ms_os_update_get_state (version->os_update);
  if (status == 0) {
    state = ms_os_update_get_state (version->os_update);
    if (state == MS_OS_UPDATE_STATE_FETCHING)
      state = MS_OS_UPDATE_STATE_FETCHED;
    else if (state == MS_OS_UPDATE_STATE_INSTALLING)
      state = MS_OS_UPDATE_STATE_INSTALLED;

    ms_os_update_set_state (version->os_update, state);
    g_task_return_boolean (task, TRUE);
  } else {
    GError *err;
    GIOErrorEnum code;

    g_debug ("Job in state %d exited with %d", state, status);
    if (status < 0)
      code = g_io_error_from_errno (-status);
    else
      code = G_IO_ERROR_FAILED;

    if (state == MS_OS_UPDATE_STATE_FETCHING)
      err = g_error_new_literal (G_IO_ERROR, code, "Fetch step failed");
    else if (state == MS_OS_UPDATE_STATE_INSTALLING)
      err = g_error_new_literal (G_IO_ERROR, code, "Install step failed");
    else
      err = g_error_new_literal (G_IO_ERROR, code, "Job failed");

    ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FAILED);
    g_task_return_error (task, err);
  }
}


static void
on_job_proxy_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  MsSystemdSysupdate *self;
  MsDBusSysupdateJob *job_proxy;
  g_autoptr (GError) err = NULL;
  Version *version = user_data;
  const char *object_path;
  gboolean finished;
  gpointer value;
  guint status;

  job_proxy = ms_dbus_sysupdate_job_proxy_new_finish (res, &err);
  if (!job_proxy) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_autoptr (GTask) task = g_steal_pointer (&version->task);

      g_warning ("Failed to get job proxy for update to %s: %s", version->version, err->message);
      g_task_return_error (task, g_steal_pointer (&err));
    }
    return;
  }

  g_debug ("Got job proxy for update to %s", version->version);
  version->job_proxy = job_proxy;
  version->progress_id = g_signal_connect_swapped (version->job_proxy,
                                                   "notify::progress",
                                                   G_CALLBACK (on_job_progress),
                                                   version);


  self = version->target->sysupdate;
  object_path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (job_proxy));
  finished = g_hash_table_lookup_extended (self->finished_jobs, object_path, NULL, &value);
  if (finished) {
    g_hash_table_remove (self->finished_jobs, object_path);
    status = GPOINTER_TO_INT (value);
    g_debug ("Job %s finished before proxy was ready, finishing right away", object_path);
    job_finished (version, status);
  }
}


static void on_job_cancel_ready (GObject *source_object, GAsyncResult *res, gpointer user_data);


static void
ms_systemd_sysupdate_cancel_job (MsOsUpdater        *updater,
                                 MsOsUpdate         *update,
                                 MsOsUpdateFlags     flags,
                                 MsOsUpdateState     state,
                                 GCancellable       *cancel,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data)
{
  MsSystemdSysupdate *self = MS_SYSTEMD_SYSUPDATE (updater);
  Version *version = g_object_get_data (G_OBJECT (update), "sysupdate-version");
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (version);
  g_return_if_fail (version->job_id > 0);
  g_return_if_fail (MS_DBUS_SYSUPDATE_IS_JOB_PROXY (version->job_proxy));

  task = g_task_new (self, cancel, callback, user_data);
  g_task_set_source_tag (task, ms_systemd_sysupdate_cancel_job);
  version->cancel_task = g_steal_pointer (&task);

  if (ms_os_update_get_state (update) != state) {
    GError *err;

    err = g_error_new_literal (G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               "Update not in cancellable state");
    g_task_return_error (task, g_steal_pointer (&err));
    return;
  }

  ms_dbus_sysupdate_job_call_cancel (version->job_proxy,
                                     G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                     -1,
                                     version->target->cancel,
                                     on_job_cancel_ready,
                                     version);
}


static void
on_job_cancel_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GError) err = NULL;
  gboolean success;
  Version *version = user_data;
  g_autoptr (GTask) task = NULL;

  success = ms_dbus_sysupdate_job_call_cancel_finish (MS_DBUS_SYSUPDATE_JOB (source_object),
                                                      res,
                                                      &err);
  if (!success) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      task = g_steal_pointer (&version->cancel_task);

      if (g_dbus_error_is_remote_error (err) &&
          g_strcmp0 (g_dbus_error_get_remote_error (err),
                     "org.freedesktop.DBus.Error.UnknownObj") == 0) {
        g_message ("Job for %s is already gone, nothing to cancel", version->version);
        ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FAILED);
        g_task_return_boolean (task, TRUE);
        return;
      }

      g_warning ("Failed to cancel job for %s: %s", version->version, err->message);
      /* Job status will be set in `job-removed` */
      g_task_return_error (task, g_steal_pointer (&err));
    }
    return;
  }

  ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_READY);
  /* Job state itself will be cleared when handling `job-removed` */
  task = g_steal_pointer (&version->cancel_task);
  g_assert (g_task_get_source_tag (task) == ms_systemd_sysupdate_cancel_job);
  g_task_return_boolean (task, TRUE);
}


static void
ms_systemd_sysupdate_cancel_install_update (MsOsUpdater        *updater,
                                            MsOsUpdate         *update,
                                            MsOsUpdateFlags     flags,
                                            GCancellable       *cancel,
                                            GAsyncReadyCallback callback,
                                            gpointer            user_data)
{
  ms_systemd_sysupdate_cancel_job (updater,
                                   update,
                                   flags,
                                   MS_OS_UPDATE_STATE_INSTALLING,
                                   cancel,
                                   callback,
                                   user_data);
}


static void on_target_install_update_ready (GObject *source_object, GAsyncResult *res,
                                            gpointer user_data);


static void
ms_systemd_sysupdate_install_update (MsOsUpdater        *updater,
                                     MsOsUpdate         *update,
                                     MsOsUpdateFlags     flags,
                                     GCancellable       *cancel,
                                     GAsyncReadyCallback callback,
                                     gpointer            user_data)
{
  MsSystemdSysupdate *self = MS_SYSTEMD_SYSUPDATE (updater);
  Version *version = g_object_get_data (G_OBJECT (update), "sysupdate-version");
  const char *new_version = "";
  GTask *task;

  g_return_if_fail (version);
  g_return_if_fail (version->job_id == 0);

  task = g_task_new (self, cancel, callback, user_data);
  g_task_set_source_tag (task, ms_systemd_sysupdate_install_update);
  version->task = g_steal_pointer (&task);

  /* sysupdate1 allows the user to install the latest version, other versions need creds */
  if (!version->newest)
    new_version = version->version;

  ms_dbus_sysupdate_target_call_install (version->target->target,
                                         new_version,
                                         0,
                                         G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                         -1,
                                         version->target->cancel,
                                         on_target_install_update_ready,
                                         version);
}


static void
on_target_install_update_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autofree char *new_version = NULL;
  g_autofree char *job_path = NULL;
  g_autoptr (GError) err = NULL;
  guint64 job_id;
  gboolean success;
  Version *version = user_data;

  success = ms_dbus_sysupdate_target_call_install_finish (MS_DBUS_SYSUPDATE_TARGET (source_object),
                                                          &new_version,
                                                          &job_id,
                                                          &job_path,
                                                          res,
                                                          &err);
  if (!success) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("Canceled install");
    } else {
      g_autoptr (GTask) task = g_steal_pointer (&version->task);

      g_debug ("Can't start install job: %d", err->code);
      if (g_dbus_error_is_remote_error (err) &&
          g_strcmp0 (g_dbus_error_get_remote_error (err),
                     "org.freedesktop.sysupdate1.NoCandidate") == 0) {
        /* When writing to a partition install is a noop */
        g_debug ("Version %s already installed", version->version);
        ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_INSTALLED);
        g_task_return_boolean (task, TRUE);
        return;
      }

      g_warning ("Failed to trigger install job for %s: %s", version->version, err->message);
      ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FAILED);
      g_task_return_error (task, g_steal_pointer (&err));
    }
    return;
  }

  g_assert (g_task_get_source_tag (version->task) == ms_systemd_sysupdate_install_update);
  ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_INSTALLING);
  g_debug ("Getting proxy for %" G_GUINT64_FORMAT " %s", job_id, job_path);
  version->job_id = job_id;

  ms_dbus_sysupdate_job_proxy_new (g_dbus_proxy_get_connection (G_DBUS_PROXY (source_object)),
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   "org.freedesktop.sysupdate1",
                                   job_path,
                                   version->target->cancel,
                                   on_job_proxy_ready,
                                   version);
}


static void
ms_systemd_sysupdate_cancel_fetch_update (MsOsUpdater        *updater,
                                          MsOsUpdate         *update,
                                          MsOsUpdateFlags     flags,
                                          GCancellable       *cancel,
                                          GAsyncReadyCallback callback,
                                          gpointer            user_data)
{
  ms_systemd_sysupdate_cancel_job (updater,
                                   update,
                                   flags,
                                   MS_OS_UPDATE_STATE_FETCHING,
                                   cancel,
                                   callback,
                                   user_data);
}


static void
on_target_acquire_update_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autofree char *new_version = NULL;
  g_autofree char *job_path = NULL;
  g_autoptr (GError) err = NULL;
  guint64 job_id;
  gboolean success;
  Version *version = user_data;

  success = ms_dbus_sysupdate_target_call_acquire_finish (MS_DBUS_SYSUPDATE_TARGET (source_object),
                                                          &new_version,
                                                          &job_id,
                                                          &job_path,
                                                          res,
                                                          &err);
  if (!success) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("Canceled acquire");
    } else {
      g_autoptr (GTask) task = g_steal_pointer (&version->task);

      g_debug ("Can't start acquire job: %d", err->code);
      if (g_dbus_error_is_remote_error (err) &&
          g_strcmp0 (g_dbus_error_get_remote_error (err),
                     "org.freedesktop.sysupdate1.NoCandidate") == 0) {
        g_debug ("Version %s already fetched", version->version);
        ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FETCHED);
        g_task_return_boolean (task, TRUE);
        return;
      }

      g_warning ("Failed to trigger install job for %s: %s", version->version, err->message);
      ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FAILED);
      g_task_return_error (task, g_steal_pointer (&err));
    }
    return;
  }

  ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_FETCHING);
  g_debug ("Getting proxy for %" G_GUINT64_FORMAT " %s", job_id, job_path);
  version->job_id = job_id;
  ms_dbus_sysupdate_job_proxy_new (g_dbus_proxy_get_connection (G_DBUS_PROXY (source_object)),
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   "org.freedesktop.sysupdate1",
                                   job_path,
                                   version->target->cancel,
                                   on_job_proxy_ready,
                                   version);
}


static void
ms_systemd_sysupdate_fetch_update (MsOsUpdater        *updater,
                                   MsOsUpdate         *update,
                                   MsOsUpdateFlags     flags,
                                   GCancellable       *cancel,
                                   GAsyncReadyCallback callback,
                                   gpointer            user_data)
{
  MsSystemdSysupdate *self = MS_SYSTEMD_SYSUPDATE (updater);
  Version *version = g_object_get_data (G_OBJECT (update), "sysupdate-version");
  const char *new_version = "";
  GTask *task;

  g_return_if_fail (version);
  g_return_if_fail (version->job_id == 0);

  task = g_task_new (self, cancel, callback, user_data);
  g_task_set_source_tag (task, ms_systemd_sysupdate_install_update);
  version->task = g_steal_pointer (&task);

  /* sysupdate1 allows the user to install the latest version, other versions need creds */
  if (!version->newest)
    new_version = version->version;

  ms_dbus_sysupdate_target_call_acquire (version->target->target,
                                         new_version,
                                         0,
                                         G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                         -1,
                                         version->target->cancel,
                                         on_target_acquire_update_ready,
                                         version);
}


static void
on_target_describe_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  gboolean success;
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autofree char *json = NULL;
  g_autoptr (GError) err = NULL;
  JsonNode *node;
  JsonObject *obj;
  Version *version = user_data;

  success = ms_dbus_sysupdate_target_call_describe_finish (MS_DBUS_SYSUPDATE_TARGET (source_object),
                                                           &json,
                                                           res,
                                                           &err);
  if (!success) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_warning ("Failed to get describe target version %s: %s", version->version, err->message);
    return;
  }

  success = json_parser_load_from_data (parser, json, -1, &err);
  if (!success) {
    g_warning ("Failed to parse json %s: %s", json, err->message);
    return;
  }

  node = json_parser_get_root (parser);
  obj = json_node_get_object (node);

  version->newest = json_object_get_boolean_member (obj, "newest");
  version->available = json_object_get_boolean_member (obj, "available");
  version->installed = json_object_get_boolean_member (obj, "installed");
  version->obsolete = json_object_get_boolean_member (obj, "obsolete");

  g_debug ("Parsed %s: newest: %d", version->version, version->newest);
  if (version->newest && version->available && !version->installed) {
    MsOsUpdate *os_update = ms_os_update_new (version->version);

    g_debug ("Update %s is usable", version->version);
    /* TODO: we should use a derived class but this updater will be replaced with systemd 262 */
    g_object_set_data (G_OBJECT (os_update), "sysupdate-version", version);
    version->os_update = g_object_ref (os_update);
    ms_os_update_set_state (version->os_update, MS_OS_UPDATE_STATE_READY);

    ms_os_updater_set_latest_update (MS_OS_UPDATER (version->target->sysupdate), os_update);
  }
}


static void
on_target_list_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  gboolean success;
  Target *target;
  g_auto (GStrv) versions = NULL;
  g_autoptr (GError) err = NULL;
  target = user_data;

  success = ms_dbus_sysupdate_target_call_list_finish (MS_DBUS_SYSUPDATE_TARGET (source_object),
                                                       &versions,
                                                       res,
                                                       &err);
  if (!success) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_warning ("Failed to get versions for target %s: %s", target->name, err->message);
    return;
  }

  for (int i = 0; versions[i]; i++) {
    Version *version = version_new (versions[i], target);

    g_debug ("Found Version: %s", version->version);

    g_ptr_array_add (target->versions, version);

    ms_dbus_sysupdate_target_call_describe (target->target,
                                            version->version,
                                            0, /* flags */
                                            G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                            10000,
                                            target->cancel,
                                            on_target_describe_ready,
                                            version);
  }
}


static void
on_target_proxy_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  Target *target;
  MsDBusSysupdateTarget *target_proxy;
  g_autoptr (GError) err = NULL;

  target_proxy = ms_dbus_sysupdate_target_proxy_new_finish (res, &err);
  if (!target_proxy) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("Cancelled getting target proxy");
      return;
    }

    g_warning ("Failed to get target proxy: %s", err->message);
    return;
  }

  target = user_data;
  target->target = target_proxy;

  ms_dbus_sysupdate_target_call_list (target->target,
                                      0, /* flags */
                                      G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
                                      10000,
                                      target->cancel,
                                      on_target_list_ready,
                                      target);
}


static void
on_job_removed (MsSystemdSysupdate *self, guint64 job_id, const char *job_path, int status)
{
  g_debug ("Job %" G_GUINT64_FORMAT " at %s finished: %d", job_id, job_path, status);

  g_return_if_fail (MS_IS_SYSTEMD_SYSUPDATE (self));

  for (guint i = 0; i < self->targets->len; i++) {
    Target *target = g_ptr_array_index (self->targets, i);

    for (guint j = 0; j < target->versions->len; j++) {
      Version *version = g_ptr_array_index (target->versions, j);

      if (version->job_proxy == NULL)
        continue;

      if (version->job_id != job_id)
        continue;

      if (g_strcmp0 (g_dbus_proxy_get_object_path (G_DBUS_PROXY (version->job_proxy)), job_path))
        continue;

      g_debug ("Found removed job %" G_GUINT64_FORMAT, job_id);

      job_finished (version, status);
      return;
    }
  }
  g_debug ("No matching job for %" G_GUINT64_FORMAT " at %s found.", job_id, job_path);
  /* Keep job path around to check against it when the proxy becomes ready */
  g_hash_table_insert (self->finished_jobs, g_strdup (job_path), GINT_TO_POINTER (status));
}


static void
check_service (MsSystemdSysupdate *self)
{
  g_autoptr (GError) err = NULL;
  const char *class = NULL;
  const char *name = NULL;
  const char *object_path = NULL;
  g_autoptr (GVariant) ret_targets = NULL;
  g_autoptr (GVariantIter) variant_iter = NULL;
  gboolean success;

  self->manager = ms_dbus_sysupdate_manager_proxy_new_sync (self->bus,
                                                            G_DBUS_PROXY_FLAGS_NONE,
                                                            "org.freedesktop.sysupdate1",
                                                            "/org/freedesktop/sysupdate1",
                                                            NULL,
                                                            &err);
  if (self->manager == NULL) {
    g_debug ("Failed to get sysupdate_proxy: %s", err->message);
    ms_os_updater_set_supported (MS_OS_UPDATER (self), FALSE);
    return;
  }

  g_signal_connect_object (self->manager,
                           "job-removed",
                           G_CALLBACK (on_job_removed),
                           self,
                           G_CONNECT_SWAPPED);

  success = ms_dbus_sysupdate_manager_call_list_targets_sync (self->manager,
                                                              G_DBUS_CALL_FLAGS_NONE,
                                                              10000,
                                                              &ret_targets,
                                                              NULL,
                                                              &err);
  if (!success) {
    g_debug ("Failed to list sysupdate targets: %s", err->message);
    ms_os_updater_set_supported (MS_OS_UPDATER (self), FALSE);
    return;
  }

  g_variant_get (ret_targets, "a(sso)", &variant_iter);
  /* Get info about all the targets */
  while (g_variant_iter_loop (variant_iter, "(&s&s&o)", &class, &name, &object_path)) {
    Target *target;

    g_debug ("Target: %s, class: %s, path: %s", name, class, object_path);

    /* We're only interested in host updates */
    if (g_strcmp0 (name, "host") || g_strcmp0 (class, "host")) {
      g_debug ("Skipping %s/%s", name, class);
      continue;
    }

    /* Since we can update the host, lets signal that: */
    ms_os_updater_set_supported (MS_OS_UPDATER (self), TRUE);
    /* TODO: do we need more than one? Isn't there only one host? */
    target = target_new (name, class, self);
    g_ptr_array_add (self->targets, target);

    ms_dbus_sysupdate_target_proxy_new (g_dbus_proxy_get_connection (G_DBUS_PROXY (self->manager)),
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        "org.freedesktop.sysupdate1",
                                        object_path,
                                        target->cancel,
                                        on_target_proxy_ready,
                                        target);
  }
}




static void
ms_systemd_sysupdate_dispose (GObject *object)
{
  MsSystemdSysupdate *self = MS_SYSTEMD_SYSUPDATE (object);

  ms_os_updater_set_latest_update (MS_OS_UPDATER (self), NULL);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->manager);
  g_clear_object (&self->bus);
  g_clear_pointer (&self->targets, g_ptr_array_unref);
  g_clear_pointer (&self->finished_jobs, g_hash_table_unref);

  G_OBJECT_CLASS (ms_systemd_sysupdate_parent_class)->dispose (object);
}


static void
ms_systemd_sysupdate_class_init (MsSystemdSysupdateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MsOsUpdaterClass *os_updater_class = MS_OS_UPDATER_CLASS (klass);

  object_class->dispose = ms_systemd_sysupdate_dispose;

  os_updater_class->fetch_update = ms_systemd_sysupdate_fetch_update;
  os_updater_class->cancel_fetch_update = ms_systemd_sysupdate_cancel_fetch_update;
  os_updater_class->install_update = ms_systemd_sysupdate_install_update;
  os_updater_class->cancel_install_update = ms_systemd_sysupdate_cancel_install_update;
}


static void
ms_systemd_sysupdate_init (MsSystemdSysupdate *self)
{
  g_autoptr (GError) err = NULL;
  self->cancel = g_cancellable_new ();
  self->bus = g_bus_get_sync (MS_DBUS_BUS, NULL, &err);
  self->targets = g_ptr_array_new_with_free_func ((GDestroyNotify)target_destroy);
  self->finished_jobs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  if (self->bus)
    check_service (self);
  else
    g_debug ("sysupdate: Can't connect to dbus");
}


MsSystemdSysupdate *
ms_systemd_sysupdate_new (void)
{
  return g_object_new (MS_TYPE_SYSTEMD_SYSUPDATE, NULL);
}
