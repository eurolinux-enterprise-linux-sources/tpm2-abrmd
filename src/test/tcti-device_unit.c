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
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-device.h"

TSS2_RC
__wrap_InitDeviceTcti (TSS2_TCTI_CONTEXT   *tcti_context,
                       size_t              *size,
                       TCTI_DEVICE_CONF    *config)
{
    *size = (size_t) mock ();
    return (TSS2_RC) mock ();
}

static void
tcti_device_setup (void **state)
{
    *state = tcti_device_new ("test");
}

static void
tcti_device_teardown (void **state)
{
    TctiDevice *tcti_sock = *state;

    g_object_unref (tcti_sock);
}
/**
 * Test object life cycle: create new object then unref it.
 */
static void
tcti_device_new_unref_test (void **state)
{
    TctiDevice *tcti_sock;

    tcti_device_setup ((void**)&tcti_sock);
    tcti_device_teardown ((void**)&tcti_sock);
}

/**
 * Calling the initialize function causes two calls to InitDeviceTcti. The
 * first gets the size of the context structure to allocate, the second
 * does the initialization. Inbetween the function allocates the context.
 */
static void
tcti_device_initialize_success_unit (void **state)
{
    TctiDevice *tcti_device = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitDeviceTcti, 512);
    will_return (__wrap_InitDeviceTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitDeviceTcti, 512);
    will_return (__wrap_InitDeviceTcti, TSS2_RC_SUCCESS);

    rc = tcti_device_initialize (tcti_device);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/**
 * Same as the previous case but this time we use the interface function.
 */
static void
tcti_device_initialize_success_interface_unit (void **state)
{
    Tcti       *tcti      = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitDeviceTcti, 512);
    will_return (__wrap_InitDeviceTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitDeviceTcti, 512);
    will_return (__wrap_InitDeviceTcti, TSS2_RC_SUCCESS);

    rc = tcti_initialize (tcti);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/**
 * Cause first call to InitDeviceTcti in tcti_device_initialize to fail.
 * We should get the RC that we provide in the second will_return sent
 * back to us.
 */
static void
tcti_device_initialize_fail_on_first_init_unit (void **state)
{
    TctiDevice *tcti_device = *state;
    TSS2_RC     rc          = TSS2_RC_SUCCESS;

    will_return (__wrap_InitDeviceTcti, 0);
    will_return (__wrap_InitDeviceTcti, TSS2_TCTI_RC_GENERAL_FAILURE);

    rc = tcti_device_initialize (tcti_device);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
/**
 * Cause the second call to InitDeviceTcti in the tcti_device_initialize to
 * fail. We should get the RC taht we provide in the 4th will_return sent
 * back to us.
 */
static void
tcti_device_initialize_fail_on_second_init_unit (void **state)
{
    TctiDevice *tcti_device = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitDeviceTcti, 512);
    will_return (__wrap_InitDeviceTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitDeviceTcti, 0);
    will_return (__wrap_InitDeviceTcti, TSS2_TCTI_RC_GENERAL_FAILURE);

    rc = tcti_device_initialize (tcti_device);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test (tcti_device_new_unref_test),
        unit_test_setup_teardown (tcti_device_initialize_success_unit,
                                  tcti_device_setup,
                                  tcti_device_teardown),
        unit_test_setup_teardown (tcti_device_initialize_success_interface_unit,
                                  tcti_device_setup,
                                  tcti_device_teardown),
        unit_test_setup_teardown (tcti_device_initialize_fail_on_first_init_unit,
                                  tcti_device_setup,
                                  tcti_device_teardown),
        unit_test_setup_teardown (tcti_device_initialize_fail_on_second_init_unit,
                                  tcti_device_setup,
                                  tcti_device_teardown),
    };
    return run_tests (tests);
}
