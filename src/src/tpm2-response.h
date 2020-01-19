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
#ifndef TPM2_RESPONSE_H
#define TPM2_RESPONSE_H

#include <glib-object.h>
#include <tpm20.h>

#include "connection.h"

G_BEGIN_DECLS

typedef struct _Tpm2ResponseClass {
    GObjectClass    parent;
} Tpm2ResponseClass;

typedef struct _Tpm2Response {
    GObject         parent_instance;
    Connection     *connection;
    guint8         *buffer;
    TPMA_CC         attributes;
} Tpm2Response;

#define TPM_RESPONSE_HEADER_SIZE (sizeof (TPM_ST) + sizeof (UINT32) + sizeof (TPM_RC))

#define TYPE_TPM2_RESPONSE            (tpm2_response_get_type      ())
#define TPM2_RESPONSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2_RESPONSE, Tpm2Response))
#define TPM2_RESPONSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2_RESPONSE, Tpm2ResponseClass))
#define IS_TPM2_RESPONSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2_RESPONSE))
#define IS_TPM2_RESPONSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2_RESPONSE))
#define TPM2_RESPONSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2_RESPONSE, Tpm2ResponseClass))

GType               tpm2_response_get_type      (void);
Tpm2Response*       tpm2_response_new           (Connection      *connection,
                                                 guint8          *buffer,
                                                 TPMA_CC          attributes);
Tpm2Response*       tpm2_response_new_rc        (Connection      *connection,
                                                 TPM_RC           rc);
TPMA_CC             tpm2_response_get_attributes (Tpm2Response   *response);
guint8*             tpm2_response_get_buffer    (Tpm2Response    *response);
TPM_RC              tpm2_response_get_code      (Tpm2Response    *response);
TPM_HANDLE          tpm2_response_get_handle    (Tpm2Response    *response);
TPM_HT              tpm2_response_get_handle_type (Tpm2Response  *response);
gboolean            tpm2_response_has_handle    (Tpm2Response    *response);
guint32             tpm2_response_get_size      (Tpm2Response    *response);
TPM_ST              tpm2_response_get_tag       (Tpm2Response    *response);
Connection*         tpm2_response_get_connection (Tpm2Response    *response);
void                tpm2_response_set_handle    (Tpm2Response    *response,
                                                 TPM_HANDLE       handle);

G_END_DECLS

#endif /* TPM2_RESPONSE_H */
