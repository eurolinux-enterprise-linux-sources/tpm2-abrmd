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
 * This test instantiates CONNECTION_COUNT TCTI instances sending a pre-canned
 * TPM command through each. It terminates each connection before receiving
 * the result.
 *
 * This test ensures that a single process is able to instantiate and use
 * multiple connections to the tabrmd.
 *
 * NOTE: this test can't and doesn't use the main.c driver from the
 * integration test harness since we need to instantiate more than a single
 * TCTI context.
 */

#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>

#include "tcti-tabrmd.h"
#include "test-options.h"

#define CONNECTION_COUNT 25

TSS2_RC
tcti_tabrmd_init (TSS2_TCTI_CONTEXT **tcti_context,
                  TCTI_TABRMD_DBUS_TYPE bus_type,
                  const char         *bus_name)
{
    TSS2_RC rc;
    size_t size;

    rc = tss2_tcti_tabrmd_init_full (NULL, &size, bus_type, bus_name);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to get allocation size for tabrmd TCTI "
                 " context: 0x%" PRIx32 "\n", rc);
        return rc;
    }
    *tcti_context = calloc (1, size);
    if (*tcti_context == NULL) {
        fprintf (stderr, "Allocation for TCTI context failed: %s\n",
                 strerror (errno));
        return rc;
    }
    rc = tss2_tcti_tabrmd_init_full (*tcti_context,
                                     &size,
                                     bus_type,
                                     bus_name);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to initialize tabrmd TCTI context: "
                 "0x%" PRIx32 "\n", rc);
        free (tcti_context);
    }
    return rc;
}

int
main (int   argc,
      char *argv[])
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *tcti_context[CONNECTION_COUNT] = { 0 };
    test_opts_t opts = {
        .tcti_type      = TCTI_DEFAULT,
        .tabrmd_bus_type = TCTI_TABRMD_DBUS_TYPE_DEFAULT,
        .tabrmd_bus_name = TCTI_TABRMD_DBUS_NAME_DEFAULT,
    };
    /* This is a pre-canned TPM2 command buffer to invoke 'GetCapability' */
    uint8_t cmd_buf[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00,
        0x01, 0x7a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x0a
    };

    g_info ("Executing test: %s", argv[0]);
    get_test_opts_from_env (&opts);
    if (sanity_check_test_opts (&opts) != 0) {
        g_error ("option sanity test failed");
    }
    if (opts.tcti_type != TABRMD_TCTI) {
        g_error ("TCTI type is not TABRMD, abroting test");
    }
    size_t i;
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        rc = tcti_tabrmd_init (&tcti_context [i],
                               opts.tabrmd_bus_type,
                               opts.tabrmd_bus_name);
        if (tcti_context [i] == NULL || rc != TSS2_RC_SUCCESS) {
            g_error ("failed to connect to TCTI: 0x%" PRIx32, rc);
        }
    }
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        rc = tss2_tcti_transmit (tcti_context [i], sizeof (cmd_buf), cmd_buf);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("failed to transmit TPM command");
        }
    }
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        tss2_tcti_finalize (tcti_context [i]);
        free (tcti_context [i]);
    }
}
