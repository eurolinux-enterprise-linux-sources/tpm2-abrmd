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
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <tpm20.h>

#include "tpm2-header.h"
#include "tpm2-response.h"

#define TPM_RESPONSE_TAG(buffer) (*(TPM_ST*)buffer)
#define TPM_RESPONSE_SIZE(buffer) (*(guint32*)(buffer + \
                                               sizeof (TPM_ST)))
#define TPM_RESPONSE_CODE(buffer) (*(TPM_RC*)(buffer + \
                                              sizeof (TPM_ST) + \
                                              sizeof (UINT32)))
#define TPM_RESPONSE_HANDLE(buffer) (*(TPM_HANDLE*)(buffer + \
                                                    sizeof (TPM_ST) + \
                                                    sizeof (UINT32) + \
                                                    sizeof (TPM_RC)))

G_DEFINE_TYPE (Tpm2Response, tpm2_response, G_TYPE_OBJECT);

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
tpm2_response_set_property (GObject        *object,
                            guint           property_id,
                            GValue const   *value,
                            GParamSpec     *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

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
tpm2_response_get_property (GObject     *object,
                            guint        property_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

    g_debug ("tpm2_response_get_property: 0x%" PRIxPTR, (uintptr_t)self);
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
tpm2_response_finalize (GObject *obj)
{
    Tpm2Response *self = TPM2_RESPONSE (obj);

    g_debug ("tpm2_response_finalize");
    if (self->buffer)
        g_free (self->buffer);
    if (self->connection)
        g_object_unref (self->connection);
    G_OBJECT_CLASS (tpm2_response_parent_class)->finalize (obj);
}
static void
tpm2_response_init (Tpm2Response *response)
{ /* noop */ }
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_response_class_init (Tpm2ResponseClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_response_parent_class == NULL)
        tpm2_response_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tpm2_response_finalize;
    object_class->get_property = tpm2_response_get_property;
    object_class->set_property = tpm2_response_set_property;

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
                              "TPM2 response buffer",
                              "memory buffer holding a TPM2 response",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("connection",
                             "Connection object",
                             "The Connection object that sent the response",
                             TYPE_CONNECTION,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Response*
tpm2_response_new (Connection     *connection,
                   guint8          *buffer,
                   TPMA_CC          attributes)
{
    return TPM2_RESPONSE (g_object_new (TYPE_TPM2_RESPONSE,
                                        "attributes", attributes,
                                       "buffer",  buffer,
                                       "connection", connection,
                                       NULL));
}
/**
 * This is a convenience wrapper that is used to create an error response
 * where all we need is a header with the RC set to something specific.
 */
Tpm2Response*
tpm2_response_new_rc (Connection *connection,
                      TPM_RC       rc)
{
    guint8 *buffer;

    buffer = calloc (1, TPM_RESPONSE_HEADER_SIZE);
    if (buffer == NULL) {
        g_warning ("tpm2_response_new_rc: failed to allocate 0x%zx"
                   " bytes for response: errno: %d: %s",
                   TPM_RESPONSE_HEADER_SIZE, errno, strerror (errno));
        return NULL;
    }
    TPM_RESPONSE_TAG (buffer)  = htobe16 (TPM_ST_NO_SESSIONS);
    TPM_RESPONSE_SIZE (buffer) = htobe32 (TPM_RESPONSE_HEADER_SIZE);
    TPM_RESPONSE_CODE (buffer) = htobe32 (rc);
    return tpm2_response_new (connection, buffer, (TPMA_CC){ 0 });
}
/* Simple "getter" to expose the attributes associated with the command. */
TPMA_CC
tpm2_response_get_attributes (Tpm2Response *response)
{
    return response->attributes;
}
/**
 */
guint8*
tpm2_response_get_buffer (Tpm2Response *response)
{
    return response->buffer;
}
/**
 */
TPM_RC
tpm2_response_get_code (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_CODE (response->buffer));
}
/**
 */
guint32
tpm2_response_get_size (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_SIZE (response->buffer));
}
/**
 */
TPM_ST
tpm2_response_get_tag (Tpm2Response *response)
{
    return be16toh (TPM_RESPONSE_TAG (response->buffer));
}
/*
 * Return the Connection object associated with this Tpm2Response. This
 * is the Connection object representing the client that should receive
 * this response. The reference count on this object is incremented before
 * the object is returned to the caller. The caller is responsible for
 * decrementing the reference count when it is done using the connection
 * object.
 */
Connection*
tpm2_response_get_connection (Tpm2Response *response)
{
    g_object_ref (response->connection);
    return response->connection;
}
/* Return the number of handles in the command. */
gboolean
tpm2_response_has_handle (Tpm2Response  *response)
{
    g_debug ("tpm2_response_has_handle");
    uint32_t tmp;

    if (tpm2_response_get_size (response) < TPM_HEADER_SIZE) {
        return FALSE;
    } else {
        tmp = tpm2_response_get_attributes (response).val;
        return tmp & TPMA_CC_RHANDLE ? TRUE : FALSE;
    }
}
/*
 * Return the handle from the response handle area. Always check to be sure
 * the response has a handle in it before calling this function. If the
 * Tpm2Response has no handle in the handle area the return value from this
 * function will be indetermanent.
 */
TPM_HANDLE
tpm2_response_get_handle (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_HANDLE (response->buffer));
}
/*
 */
void
tpm2_response_set_handle (Tpm2Response *response,
                          TPM_HANDLE    handle)
{
    TPM_RESPONSE_HANDLE (response->buffer) = htobe32 (handle);
}
/*
 * Return the type of the handle from the Tpm2Response object.
 */
TPM_HT
tpm2_response_get_handle_type (Tpm2Response *response)
{
    /*
     * Explicit cast required to keep compiler type checking happy. The
     * shift operation preserves the 8bits we want to return but compiler
     * must consider the result a TPM_HANDLE (32bits).
     */
    return (TPM_HT)(tpm2_response_get_handle (response) >> HR_SHIFT);
}
