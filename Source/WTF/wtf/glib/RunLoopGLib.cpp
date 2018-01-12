/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RunLoop.h"

#include <glib.h>
#include <wtf/MainThread.h>
#include <wtf/Threading.h>
#include <wtf/glib/RunLoopSourcePriority.h>

namespace WTF {

static GSourceFuncs runLoopSourceFunctions = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource* source, GSourceFunc callback, gpointer userData) -> gboolean
    {
        if (g_source_get_ready_time(source) == -1)
            return G_SOURCE_CONTINUE;
        g_source_set_ready_time(source, -1);
        return callback(userData);
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

// Since TLS is initialized to 0, and the AncestorMainContext of the first "global" main thread is also 0,
// we need to distinguish between the two: the (context == NULL) case is detected,
// and set to all-ones (GLOBAL_CONTEXT) instead.
#define GLOBAL_CONTEXT ((RunLoop::AncestorMainContext)~0)

static GPrivate tlsAncestorMainContextKey;

// Sets the ancestor main context for this thread using TLS
void RunLoop::setAncestorMainContext(AncestorMainContext ctx)
{
    if (ctx == NULL) {
        // AncestorMainContext == NULL, use GLOBAL_CONTEXT instead
        g_private_set(&tlsAncestorMainContextKey, (gpointer)GLOBAL_CONTEXT);
    }
    else {
        g_private_set(&tlsAncestorMainContextKey, (gpointer)ctx);
    }
}

RunLoop::AncestorMainContext getTlsAncestorMainContext()
{
    return (RunLoop::AncestorMainContext)g_private_get(&tlsAncestorMainContextKey);
}

RunLoop::AncestorMainContext RunLoop::getAncestorMainContext()
{
    AncestorMainContext ancestorMainContext = getTlsAncestorMainContext();
    if (ancestorMainContext) {
        // Ancestor main context is stored in TLS, which means this thread is a child of a main thread
    }
    else {
        // No ancestor main context, which means this thread itself is a main thread
        ancestorMainContext = (AncestorMainContext)g_main_context_get_thread_default();
        if(ancestorMainContext == NULL) {
            // Global context, store as GLOBAL_CONTEXT
            ancestorMainContext = GLOBAL_CONTEXT;
        }
    }
    return ancestorMainContext;
}

RunLoop::RunLoop()
{
    m_mainContext = g_main_context_get_thread_default();
    if (!m_mainContext) {
        m_mainContext = isMainThread() ? g_main_context_default() : adoptGRef(g_main_context_new());
        g_message("[thread %u] RunLoop::RunLoop global thread 0x%llx", currentThread(), (unsigned long long)m_mainContext.get());
    }
    else {
        g_message("[thread %u] RunLoop::RunLoop emulated process thread 0x%llx", currentThread(), (unsigned long long)m_mainContext.get());
    }
    ASSERT(m_mainContext);

    GRefPtr<GMainLoop> innermostLoop = adoptGRef(g_main_loop_new(m_mainContext.get(), FALSE));
    ASSERT(innermostLoop);
    m_mainLoops.append(innermostLoop);

    m_source = adoptGRef(g_source_new(&runLoopSourceFunctions, sizeof(GSource)));
    g_source_set_priority(m_source.get(), RunLoopSourcePriority::RunLoopDispatcher);
    g_source_set_name(m_source.get(), "[WebKit] RunLoop work");
    g_source_set_can_recurse(m_source.get(), TRUE);
    g_source_set_callback(m_source.get(), [](gpointer userData) -> gboolean {
        static_cast<RunLoop*>(userData)->performWork();
        return G_SOURCE_CONTINUE;
    }, this, nullptr);
    g_source_attach(m_source.get(), m_mainContext.get());
}

RunLoop::~RunLoop()
{
    g_message("RunLoop::~RunLoop");

    g_source_destroy(m_source.get());

    for (int i = m_mainLoops.size() - 1; i >= 0; --i) {
        if (!g_main_loop_is_running(m_mainLoops[i].get()))
            continue;
        g_main_loop_quit(m_mainLoops[i].get());
    }
}

void RunLoop::run()
{
    RunLoop& runLoop = RunLoop::current();
    GMainContext* mainContext = runLoop.m_mainContext.get();

    // The innermost main loop should always be there.
    ASSERT(!runLoop.m_mainLoops.isEmpty());

    // In any case, a new main context is going to be pushed,
    // so make sure we store the ancestor main context to TLS,
    // so this thread can remember who its ancestor main thread is.
    {
        AncestorMainContext ancestorMainContext = RunLoop::getAncestorMainContext();
        RunLoop::setAncestorMainContext(ancestorMainContext);
        g_message("[thread %u] RunLoop::run ctx 0x%llx saved to tls", currentThread(), (unsigned long long)ancestorMainContext);
    }

    GMainLoop* innermostLoop = runLoop.m_mainLoops[0].get();
    if (!g_main_loop_is_running(innermostLoop)) {
        g_main_context_push_thread_default(mainContext);
        g_main_loop_run(innermostLoop);
        g_main_context_pop_thread_default(mainContext);
        return;
    }

    // Create and run a nested loop if the innermost one was already running.
    GMainLoop* nestedMainLoop = g_main_loop_new(mainContext, FALSE);
    runLoop.m_mainLoops.append(adoptGRef(nestedMainLoop));

    g_main_context_push_thread_default(mainContext);
    g_main_loop_run(nestedMainLoop);
    g_main_context_pop_thread_default(mainContext);

    runLoop.m_mainLoops.removeLast();
}

void RunLoop::stop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_mainLoops.isEmpty());
    GRefPtr<GMainLoop> lastMainLoop = m_mainLoops.last();
    if (g_main_loop_is_running(lastMainLoop.get()))
        g_main_loop_quit(lastMainLoop.get());
}

void RunLoop::wakeUp()
{
    g_source_set_ready_time(m_source.get(), g_get_monotonic_time());
}

class DispatchAfterContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    DispatchAfterContext(Function<void()>&& function)
        : m_function(WTFMove(function))
    {
    }

    void dispatch()
    {
        m_function();
    }

private:
    Function<void()> m_function;
};

void RunLoop::dispatchAfter(Seconds duration, Function<void()>&& function)
{
    GRefPtr<GSource> source = adoptGRef(g_timeout_source_new(duration.millisecondsAs<guint>()));
    g_source_set_priority(source.get(), RunLoopSourcePriority::RunLoopTimer);
    g_source_set_name(source.get(), "[WebKit] RunLoop dispatchAfter");

    std::unique_ptr<DispatchAfterContext> context = std::make_unique<DispatchAfterContext>(WTFMove(function));
    g_source_set_callback(source.get(), [](gpointer userData) -> gboolean {
        std::unique_ptr<DispatchAfterContext> context(static_cast<DispatchAfterContext*>(userData));
        context->dispatch();
        return G_SOURCE_REMOVE;
    }, context.release(), nullptr);
    g_source_attach(source.get(), m_mainContext.get());
}

RunLoop::TimerBase::TimerBase(RunLoop& runLoop)
    : m_runLoop(runLoop)
    , m_source(adoptGRef(g_source_new(&runLoopSourceFunctions, sizeof(GSource))))
{
    g_source_set_priority(m_source.get(), RunLoopSourcePriority::RunLoopTimer);
    g_source_set_name(m_source.get(), "[WebKit] RunLoop::Timer work");
    g_source_set_callback(m_source.get(), [](gpointer userData) -> gboolean {
        RunLoop::TimerBase* timer = static_cast<RunLoop::TimerBase*>(userData);
        timer->fired();
        if (timer->m_isRepeating)
            timer->updateReadyTime();
        return G_SOURCE_CONTINUE;
    }, this, nullptr);
    g_source_attach(m_source.get(), m_runLoop->m_mainContext.get());
}

RunLoop::TimerBase::~TimerBase()
{
    g_source_destroy(m_source.get());
}

void RunLoop::TimerBase::setName(const char* name)
{
    g_source_set_name(m_source.get(), name);
}

void RunLoop::TimerBase::setPriority(int priority)
{
    g_source_set_priority(m_source.get(), priority);
}

void RunLoop::TimerBase::updateReadyTime()
{
    if (!m_fireInterval) {
        g_source_set_ready_time(m_source.get(), 0);
        return;
    }

    gint64 currentTime = g_get_monotonic_time();
    gint64 targetTime = currentTime + std::min<gint64>(G_MAXINT64 - currentTime, m_fireInterval.microsecondsAs<gint64>());
    ASSERT(targetTime >= currentTime);
    g_source_set_ready_time(m_source.get(), targetTime);
}

void RunLoop::TimerBase::start(double fireInterval, bool repeat)
{
    m_fireInterval = Seconds(fireInterval);
    m_isRepeating = repeat;
    updateReadyTime();
}

void RunLoop::TimerBase::stop()
{
    g_source_set_ready_time(m_source.get(), -1);
}

bool RunLoop::TimerBase::isActive() const
{
    return g_source_get_ready_time(m_source.get()) != -1;
}

Seconds RunLoop::TimerBase::secondsUntilFire() const
{
    gint64 time = g_source_get_ready_time(m_source.get());
    if (time != -1)
        return std::max<Seconds>(Seconds::fromMicroseconds(time - g_get_monotonic_time()), 0_s);
    return 0_s;
}

} // namespace WTF
