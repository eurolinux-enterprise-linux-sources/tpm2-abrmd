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
#include <inttypes.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "thread.h"

/*
 * This test exercises the Thread abstract class. We do so by creating a
 * derived class (the TestThread object). We can then use the thread_*
 * functions (from the abstract base class) to drive the internal thread.
 * To test the underlying Thread functions, the TestThread instance has
 * a few boolean flags (canceled / cleaned_up) that it sets when its
 * 'unblock' and cleanup functions are invoked.
 */
/*
 * Begin TestThread GObject implementation
 */
G_BEGIN_DECLS

typedef struct _TestThreadClass {
    ThreadClass parent;
} TestThreadClass;

typedef struct _TestThread {
    Thread      parent_instance;
    gboolean    canceled;
    gboolean    cleaned_up;
    gboolean    running;
} TestThread;

#define TYPE_TEST_THREAD        (test_thread_get_type ())
#define TEST_THREAD(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TEST_THREAD, TestThread))
#define IS_TEST_THREAD(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TEST_THREAD))

G_END_DECLS

G_DEFINE_TYPE (TestThread, test_thread, TYPE_THREAD);

static void
test_thread_cleanup (void *data)
{
    TestThread *self = TEST_THREAD (data);
    self->cleaned_up = TRUE;
}
/*
 * This is the "active" part of the TestThread. When 'thread_start' is
 * invoked (from the Thread base class) this function is run as a pthread.
 */
static void*
test_thread_run (void *data)
{
    TestThread *self = TEST_THREAD (data);

    pthread_cleanup_push (test_thread_cleanup, self);
    while (TRUE) {
        self->running = TRUE;
        g_debug ("test_thread_run before sleep");
        sleep (1);
        g_debug ("test_thread_run after sleep");
    }
    pthread_cleanup_pop (0);
}
/*
 * This function is invoked as part of canceling ('thread_cancel') the Thread.
 * It is used to unblock the thread in case it is blocked on something that
 * isn't a cencelation point. The TestThread just sleeps in a loop and sleep
 * is a pthread cancelation point so we just set a flag so we know this was
 * called.
 */
static void
test_thread_unblock (Thread *thread)
{
    TestThread *self = TEST_THREAD (thread);
    self->canceled = TRUE;
}

static void
test_thread_init (TestThread *self)
{
    self->canceled = FALSE;
    self->cleaned_up = FALSE;
    self->running = FALSE;
}

static void
test_thread_class_init (TestThreadClass *klass)
{
    ThreadClass *thread_klass = THREAD_CLASS (klass);

    if (test_thread_parent_class == NULL) {
        test_thread_parent_class = g_type_class_peek_parent (klass);
    }
    thread_klass->thread_run     = test_thread_run;
    thread_klass->thread_unblock = test_thread_unblock;
}

TestThread*
test_thread_new (void)
{
    return TEST_THREAD (g_object_new (TYPE_TEST_THREAD, NULL));
}
/*
 * End of TestThread GObject implementation.
 */
static void
test_thread_setup (void **state)
{
    *state = test_thread_new ();
}
static void
test_thread_teardown (void **state)
{
    g_object_unref (*state);
}
/*
 * This test ensures that the object created in the _setup function can be
 * recognized as both a Thread and as a TestThread.
 */
static void
test_thread_type_test (void **state)
{
    assert_true (G_IS_OBJECT (*state));
    assert_true (IS_TEST_THREAD (*state));
}
/*
 * This test runs the TestThread through the typical thread lifecycle: start,
 * cancel, then join.
 */
static void
test_thread_lifecycle_test (void **state)
{
    Thread *thread = THREAD (*state);
    TestThread *test_thread = TEST_THREAD (*state);
    int ret;

    ret = thread_start (thread);
    assert_int_equal (ret, 0);
    sched_yield ();
    ret = thread_cancel (thread);
    assert_int_equal (ret, 0);
    sched_yield ();
    ret = thread_join (thread);
    assert_int_equal (ret, 0);
    sched_yield ();
    /*
     * We check these flags at the end of the test to give the thread a
     * chance to run.
     */
    assert_true (test_thread->running);
    assert_true (test_thread->canceled);
    assert_true (test_thread->cleaned_up);
}
int
main (int   argc,
      char *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (test_thread_type_test,
                                  test_thread_setup,
                                  test_thread_teardown),
        unit_test_setup_teardown (test_thread_lifecycle_test,
                                  test_thread_setup,
                                  test_thread_teardown),
    };
    return run_tests(tests);

}
