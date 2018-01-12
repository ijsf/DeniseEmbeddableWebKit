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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProcessLauncher.h"

#include "Connection.h"
#include "ProcessExecutablePath.h"
#include <WebCore/FileSystem.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <wtf/RunLoop.h>
#include <wtf/UniStdExtras.h>
#include <wtf/glib/GLibUtilities.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(WPE)
#include <wpe/renderer-host.h>
#endif

#include "StorageProcess.h"
#include "NetworkProcess.h"
#include "WebProcess.h"
#include "ChildProcessMain.h"
#include <WebCore/SoupNetworkSession.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <libsoup/soup.h>
#include <wtf/CurrentTime.h>

#include "LogInitialization.h"
#include <WebCore/LogInitialization.h>
#include <runtime/InitializeThreading.h>
#include <wtf/MainThread.h>
#include <wtf/RunLoop.h>

using namespace WebCore;

namespace WebKit {

struct EmulateProcessData {
    // Process type (e.g. web, network, storage)
    ProcessLauncher::ProcessType type;

    // Initialization parameters (e.g. socket data)
    ChildProcessInitializationParameters params;
};

//
// This function represents the main() function of what would normally be a process.
// It is expected to launch in its own thread.
//
// It does this by taking care of the following:
//
// * New glib main context for this thread
// * Emulate ChildProcessMain functionality that would normally occur in new process
//
// Remarks (TODO):
//
// * g_main_context_default should be replaced with g_main_context_get_thread_default everywhere except in RunLoop::RunLoop
//
static gpointer EmulateProcessMain(gpointer data)
{
    // Retrieve EmulateProcess and wrap in unique_ptr, will auto cleanup when out-of-scope
    std::unique_ptr<EmulateProcessData> processData((EmulateProcessData *)data);

    // Add this thread as main thread
    WTF::addAsMainThread();

    // Create a new top main context for this "main thread" of the emulated process
    GMainContext *mainContext = g_main_context_new();
    g_main_context_push_thread_default(mainContext);

    // Emulate platformInitialize functionality
    if (processData->type == ProcessLauncher::ProcessType::Web) {
        g_message("ProcessLauncher::ProcessType::Web");

        // Based on WebProcessMain::platformInitialize
        {
#if (USE(COORDINATED_GRAPHICS_THREADED) || USE(GSTREAMER_GL)) && PLATFORM(X11)
            XInitThreads();
#endif
            // ACHTUNG already handled in main app?
            //gtk_init(nullptr, nullptr);

            bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
            bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
        }
    }
    else if (processData->type == ProcessLauncher::ProcessType::Network) {
        g_message("ProcessLauncher::ProcessType::Network");
    }
    else if (processData->type == ProcessLauncher::ProcessType::Storage) {
        g_message("ProcessLauncher::ProcessType::Storage");
    }
    else {
        g_error("Unsupported process %u", processData->type);
    }

    // Emulate InitializeWebKit2 functionality
    {
        JSC::initializeThreading();
        RunLoop::initializeMainRunLoop();
#if !LOG_DISABLED || !RELEASE_LOG_DISABLED
        WebCore::initializeLogChannelsIfNecessary();
        WebKit::initializeLogChannelsIfNecessary();
#endif // !LOG_DISABLED || !RELEASE_LOG_DISABLED
    }

    // Emulate initialize() functionality
    if (processData->type == ProcessLauncher::ProcessType::Web) {
        WebProcess::singleton().initialize(processData->params);
    }
    else if (processData->type == ProcessLauncher::ProcessType::Network) {
        NetworkProcess::singleton().initialize(processData->params);
    }
    else if (processData->type == ProcessLauncher::ProcessType::Storage) {
        StorageProcess::singleton().initialize(processData->params);
    }

    // Start RunLoop for this emulated process
    RunLoop::run();
    
    // Clean up
    g_main_context_pop_thread_default(mainContext);
    return NULL;
}

//
// This function emulates what would normally be a separate process, by using threads only.
// It spawns a new thread instead of a new process and uses EmulateProcessMain as main() analog function.
//
static void EmulateProcess(EmulateProcessData *processData)
{
    g_thread_new("", EmulateProcessMain, (gpointer)processData);
}

void ProcessLauncher::launchProcess()
{
    IPC::Connection::SocketPair socketPair = IPC::Connection::createPlatformConnection(IPC::Connection::ConnectionOptions::SetCloexecOnServer);

    GUniquePtr<gchar> webkitSocket(g_strdup_printf("%d", socketPair.client));

    // Emulate this process
    EmulateProcessData *data = new EmulateProcessData();
    data->params.connectionIdentifier = atoi(webkitSocket.get());
    data->type = m_launchOptions.processType;
    EmulateProcess(data);

#if 0
    // Don't expose the parent socket to potential future children.
    if (!setCloseOnExec(socketPair.client))
        RELEASE_ASSERT_NOT_REACHED();

    close(socketPair.client);
#endif

    // ACHTUNG hack
    m_processIdentifier = getpid();

    // We've finished launching the process, message back to the main run loop.
    RunLoop::main().dispatch([protectedThis = makeRef(*this), this, serverSocket = socketPair.server] {
        didFinishLaunchingProcess(m_processIdentifier, serverSocket);
    });
}

void ProcessLauncher::terminateProcess()
{
}

void ProcessLauncher::platformInvalidate()
{
}

} // namespace WebKit
