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
#include <inttypes.h>
#include <stdint.h>

#include "tpm2-command.h"
#include "tpm2-header.h"
#include "util.h"
/* macros to make getting at fields in the command more simple */

#define HEADER_SIZE (sizeof (TPM_ST) + sizeof (UINT32) + sizeof (TPM_CC))
#define HANDLE_OFFSET HEADER_SIZE
#define HANDLE_INDEX(i) (sizeof (TPM_HANDLE) * i)
#define HANDLE_GET(buffer, count) (*(TPM_HANDLE*)(HANDLE_START (buffer) + \
                                                  HANDLE_INDEX (count)))
#define HANDLE_START(buffer) (buffer + HANDLE_OFFSET)
/*
 * Offset of capability field in TPM2_GetCapability command buffer is
 * immediately after the header.
 */
#define CAP_OFFSET HEADER_SIZE
#define CAP_GET(buffer) (*(TPM_CAP*)(buffer + CAP_OFFSET))
/*
 * Offset of property field in TPM2_GetCapability command buffer is
 * immediately after the capability field.
 */
#define PROPERTY_OFFSET (CAP_OFFSET + sizeof (TPM_CAP))
#define PROPERTY_GET(buffer) (*(UINT32*)(buffer + PROPERTY_OFFSET))
/*
 * The offset of the property count field is immediately following the
 * property field.
 */
#define PROPERTY_COUNT_OFFSET (PROPERTY_OFFSET + sizeof (UINT32))
#define PROPERTY_COUNT_GET(buffer) (*(UINT32*)(buffer + PROPERTY_COUNT_OFFSET))
/*
 * Helper macros to aid in the access of various parts of the command
 * authorization area.
 */
#define AUTH_AREA_OFFSET(handle_count) \
    (HEADER_SIZE + (handle_count * sizeof (TPM_HANDLE)))
#define AUTH_AREA_SIZE(buffer, handle_count) \
    (*(UINT32*)(buffer + AUTH_AREA_OFFSET(handle_count)))
#define AUTH_FIRST_OFFSET(handle_count) \
    (size_t)(AUTH_AREA_OFFSET (handle_count) + sizeof (UINT32))

G_DEFINE_TYPE (Tpm2Command, tpm2_command, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_ATTRIBUTES,
    PROP_SESSION,
    PROP_BUFFER,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
tpm2_command_set_property (GObject        *object,
                           guint           property_id,
                           GValue const   *value,
                           GParamSpec     *pspec)
{
    Tpm2Command *self = TPM2_COMMAND (object);

    switch (property_id) {
    case PROP_ATTRIBUTES:
        self->attributes = (TPMA_CC)g_value_get_uint (value);
        break;
    case PROP_BUFFER:
        if (self->buffer != NULL) {
            g_warning ("  buffer already set");
            break;
        }
        self->buffer = (guint8*)g_value_get_pointer (value);
        break;
    case PROP_SESSION:
        if (self->connection != NULL) {
            g_warning ("  connection already set");
            break;
        }
        self->connection = g_value_get_object (value);
        g_object_ref (self->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * GObject property getter.
 */
static void
tpm2_command_get_property (GObject     *object,
                           guint        property_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    Tpm2Command *self = TPM2_COMMAND (object);

    g_debug ("tpm2_command_get_property: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_ATTRIBUTES:
        g_value_set_uint (value, self->attributes.val);
        break;
    case PROP_BUFFER:
        g_value_set_pointer (value, self->buffer);
        break;
    case PROP_SESSION:
        g_value_set_object (value, self->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * override the parent finalize method so we can free the data associated with
 * the DataMessage instance.
 */
static void
tpm2_command_finalize (GObject *obj)
{
    Tpm2Command *cmd = TPM2_COMMAND (obj);

    g_debug ("tpm2_command_finalize");
    if (cmd->buffer)
        g_free (cmd->buffer);
    if (cmd->connection)
        g_object_unref (cmd->connection);
    G_OBJECT_CLASS (tpm2_command_parent_class)->finalize (obj);
}
static void
tpm2_command_init (Tpm2Command *command)
{ /* noop */ }
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_command_class_init (Tpm2CommandClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_command_parent_class == NULL)
        tpm2_command_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tpm2_command_finalize;
    object_class->get_property = tpm2_command_get_property;
    object_class->set_property = tpm2_command_set_property;

    obj_properties [PROP_ATTRIBUTES] =
        g_param_spec_uint ("attributes",
                           "TPMA_CC",
                           "Attributes for command.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_BUFFER] =
        g_param_spec_pointer ("buffer",
                              "TPM2 command buffer",
                              "memory buffer holding a TPM2 command",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("connection",
                             "Session object",
                             "The Connection object that sent the command",
                             TYPE_CONNECTION,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Command*
tpm2_command_new (Connection     *connection,
                  guint8          *buffer,
                  TPMA_CC          attributes)
{
    return TPM2_COMMAND (g_object_new (TYPE_TPM2_COMMAND,
                                       "attributes", attributes,
                                       "buffer",  buffer,
                                       "connection", connection,
                                       NULL));
}
/* Simple "getter" to expose the attributes associated with the command. */
TPMA_CC
tpm2_command_get_attributes (Tpm2Command *command)
{
    return command->attributes;
}
/**
 */
guint8*
tpm2_command_get_buffer (Tpm2Command *command)
{
    return command->buffer;
}
/**
 */
TPM_CC
tpm2_command_get_code (Tpm2Command *command)
{
    return get_command_code (command->buffer);
}
/**
 */
guint32
tpm2_command_get_size (Tpm2Command *command)
{
    return get_command_size (command->buffer);
}
/**
 */
TPMI_ST_COMMAND_TAG
tpm2_command_get_tag (Tpm2Command *command)
{
    return get_command_tag (command->buffer);
}
/*
 * Return the Connection object associated with this Tpm2Command. This
 * is the Connection object representing the client that sent this command.
 * The reference count on this object is incremented before the Connection
 * object is returned to the caller. The caller is responsible for
 * decrementing the reference count when it is done using the connection object.
 */
Connection*
tpm2_command_get_connection (Tpm2Command *command)
{
    g_object_ref (command->connection);
    return command->connection;
}
/* Return the number of handles in the command. */
guint8
tpm2_command_get_handle_count (Tpm2Command *command)
{
    g_debug ("tpm2_command_get_handle_count");
    uint32_t tmp;

    if (command == NULL) {
        g_warning ("tpm2_command_get_handle_count received NULL parameter");
        return 0;
    }
    tmp = tpm2_command_get_attributes (command).val;
    return (guint8)((tmp & TPMA_CC_CHANDLES) >> 25);
}
/*
 * Simple function to access handles in the provided Tpm2Command. The
 * 'handle_number' parameter is a 0-based index into the handle area
 * which is effectively an array. If the handle_number requests a handle
 * beyond the end of this array 0 is returned.
 */
TPM_HANDLE
tpm2_command_get_handle (Tpm2Command *command,
                         guint8       handle_number)
{
    guint8 real_count;

    if (command == NULL)
        return 0;
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > handle_number) {
        return be32toh (HANDLE_GET (command->buffer, handle_number));
    } else {
        return 0;
    }
}
/*
 * Simple function to set a handle at the 0-based index into the Tpm2Command
 * handle area to the provided value. If the handle_number is past the bounds
 * FALSE is returned.
 */
gboolean
tpm2_command_set_handle (Tpm2Command *command,
                         TPM_HANDLE   handle,
                         guint8       handle_number)
{
    guint8 real_count;

    if (command == NULL)
        return FALSE;
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > handle_number) {
        HANDLE_GET (command->buffer, handle_number) = htobe32 (handle);
        return TRUE;
    } else {
        return FALSE;
    }
}
/*
 * Return the handles from the Tpm2Command back to the caller by way of the
 * handles parameter. The caller must allocate sufficient space to hold
 * however many handles are in this command. Take a look @
 * tpm2_command_get_handle_count.
 */
gboolean
tpm2_command_get_handles (Tpm2Command *command,
                          TPM_HANDLE   handles[],
                          guint8       count)
{
    guint8 real_count, i;

    if (command == NULL || handles == NULL) {
        g_warning ("tpm2_command_get_handles passed NULL parameter");
        return FALSE;
    }
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > count) {
        g_warning ("tpm2_command_get_handles passed insufficient handle array");
        return FALSE;
    }

    for (i = 0; i < real_count; ++i)
        handles[i] = be32toh (HANDLE_GET (command->buffer, i));

    return TRUE;
}
/*
 * Set handles in the Tpm2Command buffer. The handles are passed in the
 * 'handles' parameter, the size of this array through the 'count'
 * parameter. Each invocation of this function sets all handles in the
 * Tpm2Command and so the size of the handles array (aka 'count') must be
 * the same as the number of handles in the command.
 */
gboolean
tpm2_command_set_handles (Tpm2Command *command,
                          TPM_HANDLE   handles[],
                          guint8       count)
{
    guint8 real_count, i;

    if (command == NULL || handles == NULL) {
        g_warning ("tpm2_command_get_handles passed NULL parameter");
        return FALSE;
    }
    real_count = tpm2_command_get_handle_count (command);
    if (real_count != count) {
        g_warning ("tpm2_command_set_handles passed handle array with wrong "
                   "number of handles");
        return FALSE;
    }

    for (i = 0; i < real_count; ++i)
        HANDLE_GET (command->buffer, i) = htobe32 (handles [i]);

    return TRUE;
}
/*
 * This function is a work around. The handle in a command buffer for the
 * FlushContext command is not in the handle area and no handles are reported
 * in the attributes structure. This means that in order to cope with
 * virtualized handles in this command we must reach into the parameter area
 * which breaks the abstractions we've built for accessing handles. Thus we
 * provide a special function for getting at this single handle.
 * Use this function with caution.
 */
TPM_HANDLE
tpm2_command_get_flush_handle (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_get_flush_handle passed null parameter");
        return 0;
    }
    if (tpm2_command_get_code (command) != TPM_CC_FlushContext) {
        g_warning ("tpm2_command_get_flush_handle called with wrong command");
        return 0;
    }

    /*
     * Despite not techncially being in the "handle area" of the command we
     * are still able to access the handle as though it were. This is because
     * there are no other handles or authorizations allowd in the command and
     * the handle being flushed is the first parameter.
     */
    return be32toh (HANDLE_GET (tpm2_command_get_buffer (command), 0));
}
/*
 * When provided with a Tpm2Command that represents a call to the
 * GetCapability command this function will extract the 'capability' field.
 * On error 0 is returned.
 */
TPM_CAP
tpm2_command_get_cap (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_get_cap passed NULL parameter");
        return 0;
    }
    if (tpm2_command_get_code (command) != TPM_CC_GetCapability) {
        g_warning ("tpm2_command_get_cap provided a Tpm2Command buffer "
                   "containing the wrong command code.");
        return 0;
    }
    return (TPM_CAP)be32toh (CAP_GET (tpm2_command_get_buffer (command)));
}
/*
 * When provided with a Tpm2Command that represents a call to the
 * GetCapability command this function will extract the 'property' field.
 * On error 0 is returned.
 */
UINT32
tpm2_command_get_prop (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_get_prop passed NULL parameter");
        return 0;
    }
    if (tpm2_command_get_code (command) != TPM_CC_GetCapability) {
        g_warning ("tpm2_command_get_cap provided a Tpm2Command buffer "
                   "containing the wrong command code.");
        return 0;
    }
    return (UINT32)be32toh (PROPERTY_GET (tpm2_command_get_buffer (command)));
}
/*
 * When provided with a Tpm2Command that represents a call to the
 * GetCapability command this function will extract the 'propertyCount' field.
 * On error 0 is returned.
 */
UINT32
tpm2_command_get_prop_count (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_get_prop_count assed NULL parameter");
        return 0;
    }
    if (tpm2_command_get_code (command) != TPM_CC_GetCapability) {
        g_warning ("tpm2_command_get_cap provided a Tpm2Command buffer "
                   "containing the wrong command code.");
        return 0;
    }
    return (UINT32)be32toh (PROPERTY_COUNT_GET (tpm2_command_get_buffer (command)));
}
/*
 * This is a convencience function to keep from having to compare the tag
 * value to TPM_ST_(NO_)?_SESSIONS repeatedly.
 */
gboolean
tpm2_command_has_auths (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_has_auths passed NULL parameter");
        return FALSE;
    }
    if (tpm2_command_get_tag (command) == TPM_ST_NO_SESSIONS) {
        return FALSE;
    } else {
        return TRUE;
    }
}
/*
 * When provided with a Tpm2Command with auths in the auth area this function
 * will return the total size of the auth area.
 */
UINT32
tpm2_command_get_auths_size (Tpm2Command *command)
{
    guint8 handle_count;
    uint8_t *buffer;

    if (command == NULL) {
        g_warning ("tpm2_command_get_auths_size passed NULL parameter");
        return 0;
    }
    if (!tpm2_command_has_auths (command)) {
        g_warning ("tpm2_command_get_auths_size, Tpm2Command 0x%" PRIxPTR
                   " has no auths", (uintptr_t)command);
        return 0;
    }
    handle_count = tpm2_command_get_handle_count (command);
    buffer = tpm2_command_get_buffer (command);

    return (UINT32)be32toh (AUTH_AREA_SIZE (buffer, handle_count));
}
/*
 * Given a pointer to a command buffer and an offset into said buffer that
 * should point to an entry in the auth area this function will return the
 * number of bytes till the next auth entry.
 */
size_t
next_auth_offset (uint8_t *buffer)
{
    UINT16 hash_size, auth_size;
    size_t accumulator = 0;

    accumulator += sizeof (UINT32); /* step over session handle */
    hash_size = be16toh (*(UINT16*)(&buffer [accumulator])); /* get hash size */
    accumulator += sizeof (UINT16) + hash_size; /* step over hash size & hash */
    accumulator += sizeof (UINT8); /* step over session attributes */
    auth_size = be16toh (*(UINT16*)(&buffer [accumulator])); /* get auth size */
    accumulator += sizeof (UINT16) + auth_size; /* step over auth size + auth */

    return accumulator;
}
/*
 * The caller provided GFunc is invoked once for each authorization in the
 * command authorization area. The first parameter passed to 'func' is a
 * pointer to the start of the authorization data. The second parameter is
 * the caller provided 'user_data'.
 */
gboolean
tpm2_command_foreach_auth (Tpm2Command *command,
                           GFunc        callback,
                           gpointer     user_data)
{
    guint8   handle_count;
    UINT32   auth_area_size;
    size_t   offset;
    uint8_t *buffer;

    if (command == NULL || callback == NULL) {
        g_warning ("tpm2_command_get_auth_handle passed NULL parameter");
        return FALSE;
    }
    if (!tpm2_command_has_auths (command)) {
        g_warning ("tpm2_command_has_auths, Tpm2Command 0x%" PRIxPTR
                   " has no auths", (uintptr_t)command);
        return FALSE;
    }

    buffer = tpm2_command_get_buffer (command);
    handle_count = tpm2_command_get_handle_count (command);
    auth_area_size = be32toh (AUTH_AREA_SIZE (buffer, handle_count));

    for (offset = AUTH_FIRST_OFFSET (handle_count);
         offset < AUTH_FIRST_OFFSET (handle_count) + auth_area_size;
         offset += next_auth_offset (&buffer [offset]))
    {
        g_debug ("invoking callback at 0x%" PRIxPTR " with authorization at: "
                 "0x%" PRIxPTR " and user data: 0x%" PRIxPTR,
                 (uintptr_t)callback,
                 (uintptr_t)&buffer [offset],
                 (uintptr_t)user_data);
        callback (&buffer [offset], user_data);
    }

    return TRUE;
}
