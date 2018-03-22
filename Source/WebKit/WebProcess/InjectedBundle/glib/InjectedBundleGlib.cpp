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
#include "InjectedBundle.h"

#include "WKBundleAPICast.h"
#include "WKBundleInitialize.h"
#include <WebCore/FileSystem.h>
#include <wtf/text/CString.h>

using namespace WebCore;

// WKBundleInitialize forward declaration for static linking
// as copied from API/glib/WebKitInjectedBundleMain.cpp
#if defined(WIN32) || defined(_WIN32)
extern "C" __declspec(dllexport)
#else
extern "C"
#endif
void WKBundleInitialize(WKBundleRef bundle, WKTypeRef userData);

namespace WebKit {

bool InjectedBundle::initialize(const WebProcessCreationParameters&, API::Object* initializationUserData)
{
    // webkit2gtkinjectedbundle is linked in statically in this port, so any dynamic loading is avoided here
    WKBundleInitialize(toAPI(this), toAPI(initializationUserData));
    return true;
}

void InjectedBundle::setBundleParameter(WTF::String const&, IPC::DataReference const&)
{
}

void InjectedBundle::setBundleParameters(const IPC::DataReference&)
{
}

} // namespace WebKit
