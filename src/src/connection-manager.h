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
#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"

G_BEGIN_DECLS

#define MAX_CONNECTIONS 100
#define MAX_CONNECTIONS_DEFAULT 27

typedef struct _ConnectionManagerClass {
    GObjectClass      parent;
} ConnectionManagerClass;

typedef struct _ConnectionManager {
    GObject           parent_instance;
    pthread_mutex_t   mutex;
    GHashTable       *connection_from_fd_table;
    GHashTable       *connection_from_id_table;
    guint             max_connections;
} ConnectionManager;

/* type for callbacks registered with the 'new-connection' signal*/
typedef gint (*new_connection_callback)(ConnectionManager *connection_manager,
                                        Connection        *connection,
                                        gpointer          *data);

#define TYPE_CONNECTION_MANAGER              (connection_manager_get_type   ())
#define CONNECTION_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_CONNECTION_MANAGER, ConnectionManager))
#define CONNECTION_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_CONNECTION_MANAGER, ConnectionManagerClass))
#define IS_CONNECTION_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_CONNECTION_MANAGER))
#define IS_CONNECTION_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_CONNECTION_MANAGER))
#define CONNECTION_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_CONNECTION_MANAGER, ConnectionManagerClass))

GType          connection_manager_get_type    (void);
ConnectionManager* connection_manager_new     (guint               max_connections);
gint           connection_manager_insert      (ConnectionManager  *manager,
                                               Connection         *connection);
gint           connection_manager_remove      (ConnectionManager  *manager,
                                               Connection         *connection);
Connection*    connection_manager_lookup_fd   (ConnectionManager  *manager,
                                               gint                fd_in);
Connection*    connection_manager_lookup_id   (ConnectionManager  *manager,
                                               gint64              id_in);
gboolean       connection_manager_contains_id (ConnectionManager  *manager,
                                               gint64              id_in);
guint          connection_manager_size        (ConnectionManager  *manager);
gboolean       connection_manager_is_full     (ConnectionManager  *manager);

G_END_DECLS
#endif /* CONNECTION_MANAGER_H */
