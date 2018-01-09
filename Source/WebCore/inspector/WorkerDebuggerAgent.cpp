/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "WorkerDebuggerAgent.h"

#include "ScriptState.h"
#include "WorkerGlobalScope.h"
#include <inspector/ConsoleMessage.h>
#include <inspector/InjectedScript.h>
#include <inspector/InjectedScriptManager.h>
#include <inspector/ScriptCallStack.h>
#include <inspector/ScriptCallStackFactory.h>

using namespace JSC;
using namespace Inspector;

namespace WebCore {

WorkerDebuggerAgent::WorkerDebuggerAgent(WorkerAgentContext& context)
    : WebDebuggerAgent(context)
    , m_workerGlobalScope(context.workerGlobalScope)
{
}

WorkerDebuggerAgent::~WorkerDebuggerAgent()
{
}

void WorkerDebuggerAgent::breakpointActionLog(ExecState& state, const String& message)
{
    m_workerGlobalScope.addConsoleMessage(std::make_unique<ConsoleMessage>(MessageSource::JS, MessageType::Log, MessageLevel::Log, message, createScriptCallStack(&state, ScriptCallStack::maxCallStackSizeToCapture)));
}

InjectedScript WorkerDebuggerAgent::injectedScriptForEval(ErrorString& errorString, const int* executionContextId)
{
    if (executionContextId) {
        errorString = ASCIILiteral("Execution context id is not supported for workers as there is only one execution context.");
        return InjectedScript();
    }

    JSC::ExecState* scriptState = execStateFromWorkerGlobalScope(&m_workerGlobalScope);
    return injectedScriptManager().injectedScriptFor(scriptState);
}

} // namespace WebCore
