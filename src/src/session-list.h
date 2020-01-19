/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SESSION_LIST_H
#define SESSION_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

#include "connection.h"
#include "session-entry.h"

G_BEGIN_DECLS

#define SESSION_LIST_MAX_ENTRIES_DEFAULT 27
#define SESSION_LIST_MAX_ENTRIES_MAX     100

typedef struct _SessionListClass {
    GObjectClass      parent;
} SessionListClass;

typedef struct _SessionList {
    GObject             parent_instance;
    pthread_mutex_t     mutex;
    guint               max_entries;
    GSList             *session_entry_slist;
} SessionList;

#define TYPE_SESSION_LIST              (session_list_get_type   ())
#define SESSION_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_SESSION_LIST, SessionList))
#define SESSION_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_SESSION_LIST, SessionListClass))
#define IS_SESSION_LIST(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_SESSION_LIST))
#define IS_SESSION_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_SESSION_LIST))
#define SESSION_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_SESSION_LIST, SessionListClass))

GType          session_list_get_type          (void);
SessionList*   session_list_new               (guint             max_entries);
gboolean       session_list_insert            (SessionList      *list,
                                               SessionEntry     *entry);
gint           session_list_lock              (SessionList      *list);
gint           session_list_unlock            (SessionList      *list);
SessionEntry*  session_list_lookup_connection (SessionList      *list,
                                               Connection       *connection);
SessionEntry*  session_list_lookup_handle     (SessionList      *list,
                                              TPM_HANDLE        handle);
gint           session_list_remove_handle     (SessionList      *list,
                                               TPM_HANDLE        handle);
gint           session_list_remove_connection (SessionList      *list,
                                               Connection       *connection);
void           session_list_remove            (SessionList      *list,
                                               SessionEntry     *entry);
guint          session_list_size              (SessionList      *list);
gboolean       session_list_is_full           (SessionList      *list);
void           session_list_prettyprint       (SessionList      *list);
void           session_list_foreach           (SessionList      *list,
                                               GFunc             func,
                                               gpointer          user_data);

G_END_DECLS
#endif /* SESSION_LIST_H */
