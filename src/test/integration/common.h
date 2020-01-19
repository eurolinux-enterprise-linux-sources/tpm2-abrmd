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
#include <inttypes.h>
#include <sapi/tpm20.h>

#define PRIxHANDLE "08" PRIx32

TSS2_RC
tcti_context_init (
    TSS2_TCTI_CONTEXT **tcti_context
    );

TSS2_RC
sapi_context_init (
    TSS2_SYS_CONTEXT    **sapi_context,
    TSS2_TCTI_CONTEXT    *tcti_context
    );

TSS2_RC
create_primary (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE       *handle
    );

TSS2_RC
create_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        parent_handle,
    TPM2B_PRIVATE    *out_private,
    TPM2B_PUBLIC     *out_public
    );

TSS2_RC
load_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        parent_handle,
    TPM_HANDLE       *out_handle,
    TPM2B_PRIVATE    *in_private,
    TPM2B_PUBLIC     *in_public
    );

TSS2_RC
save_context (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        handle,
    TPMS_CONTEXT     *context
    );

TSS2_RC
flush_context (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        handle
    );

TSS2_RC
start_auth_session (
    TSS2_SYS_CONTEXT      *sapi_context,
    TPMI_SH_AUTH_SESSION  *session_handle
    );

void
prettyprint_context (
    TPMS_CONTEXT *context
    );
