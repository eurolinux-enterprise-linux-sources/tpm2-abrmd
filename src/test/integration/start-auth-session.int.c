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
#include <glib.h>
#include <stdio.h>

#include <sapi/tpm20.h>

#include "common.h"
#include "test.h"
#define PRIxHANDLE "08" PRIx32
/*
 * This test exercises the session creation logic. We begin by creating the
 * most simple session we can. We then query the TPM (using the GetCapability
 * command) for the handle of all loaded handles. The handle of the session
 * that we just created should be the only handle returned. Finally we query
 * the TPM for the active handles. The only session handle we've created is
 * still loaded so we should receive no handles back.
 *
 * We then save the context of the session. This should trasition it's state
 * from 'loaded' to 'active'. Now when we query the TPM for loaded and active
 * handles again. Since we've saved the context of our session there should
 * be no loaded sessions and one active session (the one we just saved).
 *
 * We then load the saved session again. Again we query the TPM for loaded and
 * active sessions. The state should be the same as our first test.
 *
 * Finally we flush our context and query the TPM for the loaded and active
 * sessions. Since we just flushed our session there should be none loaded
 * or active.
 */

/*
 * This function queries the TPM for the handle types specified in the
 * 'query_handle' parameter. It returns the number of this type of handle
 * that are currently being tracked by the TPM.
 */
TSS2_RC
handles_count (TSS2_SYS_CONTEXT *sapi_context,
               TPM_HANDLE        query_handle,
               UINT32           *count)
{
    TSS2_RC              rc         = TSS2_RC_SUCCESS;
    TPM_CAP              capability = TPM_CAP_HANDLES;
    TPMI_YES_NO          more_data  = NO;
    TPMS_CAPABILITY_DATA cap_data   = { 0, };

    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 capability,
                                 query_handle,
                                 100,
                                 &more_data,
                                 &cap_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("error getting capability: 0x%" PRIx32, rc);
    }
    if (more_data == YES) {
        g_warning ("got 'more_data' from query on loaded sessions");
    }
    *count = cap_data.data.handles.count;

    return rc;
}
/*
 * This function queries the TPM for the handles of the type specified in the
 * 'query_handle' parameter. It then "prettyprints" them.
 */
TSS2_RC
prettyprint_getcap_handles (TSS2_SYS_CONTEXT *sapi_context,
                            TPM_HANDLE        query_handle)
{
    TSS2_RC              rc         = TSS2_RC_SUCCESS;
    TPM_CAP              capability = TPM_CAP_HANDLES;
    TPMI_YES_NO          more_data  = NO;
    TPMS_CAPABILITY_DATA cap_data   = { 0, };
    size_t               count      = 100;
    size_t               i;

    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 capability,
                                 query_handle,
                                 count,
                                 &more_data,
                                 &cap_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("error getting capability: 0x%" PRIx32, rc);
    }
    if (more_data == YES) {
        g_warning ("got 'more_data' from query on loaded sessions");
    }
    g_print ("GetCapability: cap: 0x%" PRIxHANDLE " prop: 0x%" PRIxHANDLE
             " got %" PRIx32 " handles\n", capability, query_handle,
             cap_data.data.handles.count);
    for (i = 0; i < cap_data.data.handles.count; ++i) {
        g_print ("  0x%" PRIxHANDLE "\n", cap_data.data.handles.handle [i]);
    }

    return rc;
}
/*
 * This function queries the TPM for the loaded and active session handles.
 * It then prints the handle IDs (UINT32) to the console.
 */
TSS2_RC
dump_loaded_active_handles (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;

    g_print ("Printing LOADED_SESSION handles\n");
    rc = prettyprint_getcap_handles (sapi_context, LOADED_SESSION_FIRST);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to get handles for LOADED_SESSION_FIRST: 0x%" PRIxHANDLE,
                 rc);
    }
    g_print ("Printing ACTIVE_SESSION handles\n");
    rc = prettyprint_getcap_handles (sapi_context, ACTIVE_SESSION_FIRST);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to get handles for ACTIVE_SESSION_FIRST: 0x%" PRIx32,
                 rc);
    }

    return rc;
}
/*
 * See the leading comment for a description of this test.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC               rc;
    TPMI_SH_AUTH_SESSION  session_handle = 0, session_handle_load = 0;
    TPMS_CONTEXT          context = { 0, };
    UINT32                count = 0;

    /* create an auth session */
    g_info ("Starting unbound, unsaulted auth session");
    rc = start_auth_session (sapi_context, &session_handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxHANDLE, rc);
    }
    g_info ("StartAuthSession for TPM_SE_POLICY success! Session handle: "
            "0x%08" PRIx32, session_handle);
    handles_count (sapi_context, LOADED_SESSION_FIRST, &count);
    g_assert (count == 1);
    handles_count (sapi_context, ACTIVE_SESSION_FIRST, &count);
    g_assert (count == 0);
    dump_loaded_active_handles (sapi_context);

    /* save context */
    g_info ("Saving context for session: 0x%" PRIxHANDLE, session_handle);
    rc = Tss2_Sys_ContextSave (sapi_context, session_handle, &context);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_ContextSave failed: 0x%" PRIxHANDLE, rc);
    }
    g_info ("Successfully saved context for session: 0x%" PRIxHANDLE,
            session_handle);
    prettyprint_context (&context);
    dump_loaded_active_handles (sapi_context);
    handles_count (sapi_context, LOADED_SESSION_FIRST, &count);
    g_assert (count == 0);
    handles_count (sapi_context, ACTIVE_SESSION_FIRST, &count);
    g_assert (count == 1);

    /* load context */
    g_info ("Loading context for session: 0x%" PRIxHANDLE, session_handle);
    rc = Tss2_Sys_ContextLoad (sapi_context, &context, &session_handle_load);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_ContextLoad failed: 0x%" PRIxHANDLE, rc);
    }
    g_info ("Successfully loaded context for session: 0x%" PRIxHANDLE,
            session_handle_load);
    if (session_handle_load == session_handle) {
        g_info ("session_handle == session_handle_load");
    } else {
        g_info ("session_handle != session_handle_load");
    }
    dump_loaded_active_handles (sapi_context);
    handles_count (sapi_context, LOADED_SESSION_FIRST, &count);
    g_assert (count == 1);
    handles_count (sapi_context, ACTIVE_SESSION_FIRST, &count);
    g_assert (count == 0);

    g_info ("Flushing context for session: 0x%" PRIxHANDLE, session_handle);
    rc = Tss2_Sys_FlushContext (sapi_context, session_handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_FlushContext failed: 0x%" PRIx32, rc);
    }
    g_info ("Flushed context for session handle: 0x%" PRIxHANDLE " success!",
            session_handle);
    dump_loaded_active_handles (sapi_context);
    handles_count (sapi_context, LOADED_SESSION_FIRST, &count);
    g_assert (count == 0);
    handles_count (sapi_context, ACTIVE_SESSION_FIRST, &count);
    g_assert (count == 0);

    return 0;
}
