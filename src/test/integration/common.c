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
/*
 * These are common functions used by the integration tests.
 */
#include <glib.h>

#include "common.h"
#include "tcti-tabrmd.h"
#include "tpm2-struct-init.h"

/*
 * Instantiate a TCTI context for communication with the tabrmd.
 */
TSS2_RC
tcti_context_init (TSS2_TCTI_CONTEXT **tcti_context)
{
    TSS2_TCTI_CONTEXT *tmp_tcti_context;
    TSS2_RC rc;
    size_t context_size;

    if (tcti_context == NULL)
        g_error ("tcti_context_init passed NULL reference");
    rc = tss2_tcti_tabrmd_init (NULL, &context_size);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to get size of tcti context");
    g_debug ("tcti size is: %zd", context_size);
    tmp_tcti_context = calloc (1, context_size);
    if (tmp_tcti_context == NULL)
        g_error ("failed to allocate memory for tcti context");
    g_debug ("context structure allocated successfully");
    rc = tss2_tcti_tabrmd_init (tmp_tcti_context, NULL);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize tcti context. RC: 0x%" PRIx32, rc);

    *tcti_context = tmp_tcti_context;
    return rc;
}
/*
 * Initialize a SAPI context using the TCTI context provided by the caller.
 * This function allocates memory for the SAPI context and returns it to the
 * caller. This memory must be freed by the caller.
 */
TSS2_RC
sapi_context_init (TSS2_SYS_CONTEXT  **sapi_context,
                   TSS2_TCTI_CONTEXT  *tcti_context)
{
    TSS2_SYS_CONTEXT *tmp_sapi_context;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = {
        .tssCreator = TSSWG_INTEROP,
        .tssFamily  = TSS_SAPI_FIRST_FAMILY,
        .tssLevel   = TSS_SAPI_FIRST_LEVEL,
        .tssVersion = TSS_SAPI_FIRST_VERSION,
    };

    if (sapi_context == NULL || tcti_context == NULL)
        g_error ("sapi_context_init passed NULL reference");
    size = Tss2_Sys_GetContextSize (0);
    tmp_sapi_context = (TSS2_SYS_CONTEXT*)calloc (1, size);
    if (tmp_sapi_context == NULL) {
        g_error ("Failed to allocate %zd bytes for the SAPI context", size);
    }
    rc = Tss2_Sys_Initialize (tmp_sapi_context, size, tcti_context, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Failed to initialize SAPI context: 0x%" PRIx32, rc);
        free (tmp_sapi_context);
        return rc;
    }

    *sapi_context = tmp_sapi_context;
    return rc;
}
/*
 */
TSS2_RC
create_primary (TSS2_SYS_CONTEXT *sapi_context,
                TPM_HANDLE       *handle)
{
    TSS2_RC rc;
    TPM2B_SENSITIVE_CREATE in_sensitive    = { 0 };
    TPM2B_PUBLIC           in_public       = { 0 };
    TPM2B_DATA             outside_info    = { 0 };
    TPML_PCR_SELECTION     creation_pcr    = { 0 };
    TPM2B_PUBLIC           out_public      = { 0 };
    TPM2B_CREATION_DATA    creation_data   = { 0 };
    TPM2B_DIGEST           creation_digest = TPM2B_DIGEST_STATIC_INIT;
    TPMT_TK_CREATION       creation_ticket = { 0 };
    TPM2B_NAME             name            = TPM2B_NAME_STATIC_INIT;
    /* command auth stuff */
    TPMS_AUTH_COMMAND auth_command = { .sessionHandle = TPM_RS_PW, };
    TPMS_AUTH_COMMAND *auth_command_array[1] = { &auth_command, };
    TSS2_SYS_CMD_AUTHS cmd_auths = {
        .cmdAuthsCount = 1,
        .cmdAuths      = auth_command_array,
    };

    /* prepare in_sensitive */
    in_sensitive.t.size = in_sensitive.t.sensitive.userAuth.t.size + 2;
    /* prepare in_public TPMT_PUBLIC */
    /* TPMI_ALG_PUBLIC / publicArea */
    in_public.t.publicArea.type    = TPM_ALG_RSA;
    /* TPMI_ALG_HASH / nameAlg */
    in_public.t.publicArea.nameAlg = TPM_ALG_SHA256;
    /* TPMA_OBJECT / objectAttributes */
    in_public.t.publicArea.objectAttributes.val = \
        TPMA_OBJECT_FIXEDTPM            | TPMA_OBJECT_FIXEDPARENT  | \
        TPMA_OBJECT_SENSITIVEDATAORIGIN | TPMA_OBJECT_USERWITHAUTH | \
        TPMA_OBJECT_RESTRICTED          | TPMA_OBJECT_DECRYPT;
    /* TPM2B_DIGEST / authPolicy */
    in_public.t.publicArea.authPolicy.t.size = 0;
    /* TPMU_PUBLIC_PARAMS / parameters: key type is TPM_ALG_RSA, set parameters accordingly */
    in_public.t.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM_ALG_AES;
    in_public.t.publicArea.parameters.rsaDetail.symmetric.keyBits.aes = 128;
    in_public.t.publicArea.parameters.rsaDetail.symmetric.mode.aes = TPM_ALG_CFB;
    in_public.t.publicArea.parameters.rsaDetail.scheme.scheme = TPM_ALG_NULL;
    in_public.t.publicArea.parameters.rsaDetail.keyBits = 2048;
    in_public.t.publicArea.parameters.rsaDetail.exponent = 0;
    /* TPMU_PUBLIC_ID / unique */
    in_public.t.publicArea.unique.rsa.t.size = 0;

    rc = Tss2_Sys_CreatePrimary (
        sapi_context,
        TPM_RH_NULL,      /* in: hierarchy */
        &cmd_auths,       /* in: in sessions / auths */
        &in_sensitive,    /* in: sensitive data? */
        &in_public,       /* in: key template */
        &outside_info,    /* in: data that will be included in the creation data */
        &creation_pcr,    /* in: PCR data used in creation data */
        handle,           /* out: handle for loaded object */
        &out_public,      /* out: public portion of created object */
        &creation_data,   /* out: TPMT_CREATION_DATA for object */
        &creation_digest, /* out: digest of creationData using nameAlg */
        &creation_ticket, /* out: ticket used to associate object and TPM */
        &name,            /* out: name of created object */
        NULL              /* out: sessions / auths returned */
    );
    if (rc == TSS2_RC_SUCCESS) {
        g_print ("  handle returned: 0x%" PRIx32 "\n", *handle);
    } else {
        g_warning ("Tss2_Sys_CreatePrimary returned: 0x%" PRIx32, rc);
    }

    return rc;
}
/*
 */
TSS2_RC
create_key (TSS2_SYS_CONTEXT *sapi_context,
            TPM_HANDLE        parent_handle,
            TPM2B_PRIVATE    *out_private,
            TPM2B_PUBLIC     *out_public)
{
    TPM_RC rc;

    TPM2B_SENSITIVE_CREATE  in_sensitive     = { 0 };
    TPM2B_PUBLIC	    in_public        = { 0 };
    TPM2B_DATA	            outside_info     = { 0 };
    TPML_PCR_SELECTION	    creation_pcr     = { 0 };
    TPM2B_CREATION_DATA	    creation_data    = { 0 };
    TPM2B_DIGEST	    creation_hash    = TPM2B_DIGEST_STATIC_INIT;
    TPMT_TK_CREATION	    creation_ticket  = { 0 };
    TPMS_AUTH_COMMAND auth_command           = { .sessionHandle = TPM_RS_PW, };
    TPMS_AUTH_COMMAND *auth_command_array[1] = { &auth_command, };
    TSS2_SYS_CMD_AUTHS cmd_auths = {
        .cmdAuthsCount = 1,
        .cmdAuths      = auth_command_array,
    };

    g_debug ("create_key with parent_handle: 0x%" PRIx32, parent_handle);
    in_sensitive.t.size = in_sensitive.t.sensitive.userAuth.t.size + 2;
    in_public.t.publicArea.type = TPM_ALG_RSA;
    in_public.t.publicArea.nameAlg = TPM_ALG_SHA256;
    in_public.t.publicArea.objectAttributes.val = \
        TPMA_OBJECT_FIXEDTPM            | TPMA_OBJECT_FIXEDPARENT  | \
        TPMA_OBJECT_SENSITIVEDATAORIGIN | TPMA_OBJECT_USERWITHAUTH | \
        TPMA_OBJECT_DECRYPT             | TPMA_OBJECT_SIGN;
    /* TPM2B_DIGEST / authPolicy */
    in_public.t.publicArea.authPolicy.t.size = 0;
    /* TPMU_PUBLIC_PARAMS / parameters: key type is TPM_ALG_RSA, set parameters accordingly */
    in_public.t.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM_ALG_NULL;
    in_public.t.publicArea.parameters.rsaDetail.scheme.scheme = TPM_ALG_NULL;
    in_public.t.publicArea.parameters.rsaDetail.keyBits = 2048;
    in_public.t.publicArea.parameters.rsaDetail.exponent = 0;
    /* TPMU_PUBLIC_ID / unique */
    in_public.t.publicArea.unique.rsa.t.size = 0;

    g_print ("Tss2_Sys_Create with parent handle: 0x%" PRIx32 "\n",
             parent_handle);
    rc = Tss2_Sys_Create(
        sapi_context,
        parent_handle,
        &cmd_auths,
        &in_sensitive,
        &in_public,
        &outside_info,
        &creation_pcr,
        out_private, /* private part of new key returned to caller */
        out_public,  /* public part of new key returned to caller */
        &creation_data,
        &creation_hash,
        &creation_ticket,
        NULL
    );
    if (rc == TSS2_RC_SUCCESS) {
        g_print ("Tss2_Sys_Create returned TSS2_RC_SUCCESS\n  parent handle: "
                 "0x%" PRIx32 "\n  out_private: 0x%" PRIxPTR "\n  out_public: "
                 "0x%" PRIxPTR "\n", parent_handle, (uintptr_t)&out_private,
                 (uintptr_t)&out_public);
    } else {
        g_warning ("Tss2_Sys_Create returned: 0x%" PRIx32, rc);
    }

    return rc;
}
/*
 */
TSS2_RC
load_key (TSS2_SYS_CONTEXT *sapi_context,
          TPM_HANDLE        parent_handle,
          TPM_HANDLE       *out_handle,
          TPM2B_PRIVATE    *in_private,
          TPM2B_PUBLIC     *in_public)
{
    TSS2_RC            rc;
    TPM2B_NAME         name                  = TPM2B_NAME_STATIC_INIT;
    TPMS_AUTH_COMMAND  auth_command          = { .sessionHandle = TPM_RS_PW, };
    TPMS_AUTH_COMMAND *auth_command_array[1] = { &auth_command, };
    TSS2_SYS_CMD_AUTHS cmd_auths             = {
        .cmdAuthsCount = 1,
        .cmdAuths      = auth_command_array,
    };

    g_print ("Tss2_Sys_Load with parent handle: 0x%" PRIx32 "\n  in_private: "
             "0x%" PRIxPTR "\n  in_public: 0x%" PRIxPTR "\n",
             parent_handle, (uintptr_t)in_private, (uintptr_t)in_public);
    rc = Tss2_Sys_Load (sapi_context,
                        parent_handle,
                        &cmd_auths,
                        in_private,
                        in_public,
                        out_handle,
                        &name,
                        NULL);

    if (rc == TSS2_RC_SUCCESS) {
        g_print ("Tss2_Sys_Load returned TSS2_RC_SUCCESS\n  parent handle: "
                 "0x%" PRIx32 "\n  new handle: 0x%" PRIx32 "\n",
                 parent_handle, *out_handle);
    } else {
        g_warning ("Tss2_Sys_Create returned: 0x%" PRIx32, rc);
    }

    return rc;
}
TSS2_RC
save_context (TSS2_SYS_CONTEXT *sapi_context,
              TPM_HANDLE        handle,
              TPMS_CONTEXT     *context)
{
    TSS2_RC rc;

    g_debug ("save_context: sapi_context: 0x%" PRIxPTR " handle: 0x%"
             PRIx32 " context: 0x%" PRIxPTR, (uintptr_t)sapi_context,
             handle, (uintptr_t)context);
    if (sapi_context == NULL || context == NULL) {
        g_error ("save_context passed NULL reference");
    }

    rc = Tss2_Sys_ContextSave (sapi_context, handle, context);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_ContextSave: failed to save context for handle: "
                   "0x%" PRIx32, handle);
    }

    return rc;
}
TSS2_RC
flush_context (TSS2_SYS_CONTEXT *sapi_context,
               TPM_HANDLE        handle)
{
    TSS2_RC rc;

    g_debug ("flush_context: sapi_context: 0x%" PRIxPTR " handle: 0x%"
             PRIx32, (uintptr_t)sapi_context, handle);
    if (sapi_context == NULL) {
        g_error ("flush_context passed NULL reference");
    }

    rc = Tss2_Sys_FlushContext (sapi_context, handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_FlushContext: failed to flush context for "
                   "handle: 0x%" PRIx32 " TSS2_RC: 0x%" PRIx32, handle, rc);
    }

    return rc;
}
/*
 * This fucntion is a very simple wrapper around the TPM2_StartAuthSession
 * function. It uses the most simple / default values to create an unsalted,
 * unbound session.
 */
TSS2_RC
start_auth_session (TSS2_SYS_CONTEXT      *sapi_context,
                    TPMI_SH_AUTH_SESSION  *session_handle)
{
    TSS2_RC rc;
    TPM2B_NONCE nonce_caller = {
        .t = {
            .size   = SHA256_DIGEST_SIZE,
            .buffer = {
                0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
                0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
                0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
                0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
            }
        }
    };
    TPM2B_NONCE nonce_tpm = {
        .t = {
            .size   = SHA256_DIGEST_SIZE,
            .buffer = { 0 }
        }
    };
    TPM2B_ENCRYPTED_SECRET encrypted_salt = { 0 };
    TPMT_SYM_DEF           symmetric      = { .algorithm = TPM_ALG_NULL };

    g_debug ("StartAuthSession for TPM_SE_POLICY (policy session)");
    rc = Tss2_Sys_StartAuthSession (sapi_context,
                                    TPM_RH_NULL,     /* tpmKey */
                                    TPM_RH_NULL,     /* bind */
                                    0,               /* cmdAuthsArray */
                                    &nonce_caller,   /* nonceCaller */
                                    &encrypted_salt, /* encryptedSalt */
                                    TPM_SE_POLICY,   /* sessionType */
                                    &symmetric,      /* symmetric */
                                    TPM_ALG_SHA256,  /* authHash */
                                    session_handle,  /* sessionHandle */
                                    &nonce_tpm,      /* nonceTPM */
                                    0                /* rspAuthsArray */
                                    );
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIx32, rc);

    return rc;
}
/*
 * This function dumps the fields of the TPMS_CONTEXT structure. The one
 * encrypted field (contextBlob) is dumped as an address.
 */
void
prettyprint_context (TPMS_CONTEXT *context)
{
    g_debug ("sequence:    0x%" PRIx64, context->sequence);
    g_debug ("savedHandle: 0x%" PRIxHANDLE, context->savedHandle);
    g_debug ("hierarchy:   0x%" PRIxHANDLE, context->hierarchy);
    g_debug ("contextBlob: 0x%" PRIxPTR, (uintptr_t)&context->contextBlob);
}
