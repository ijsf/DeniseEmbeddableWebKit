/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#pragma once

#include "SlotVisitor.h"
#include <wtf/TimeWithDynamicClockType.h>

namespace JSC {

class VisitingTimeout {
public:
    VisitingTimeout()
    {
    }
    
    VisitingTimeout(SlotVisitor& visitor, bool didVisitSomething, MonotonicTime timeout)
        : m_didVisitSomething(didVisitSomething)
        , m_visitCountBefore(visitor.visitCount())
        , m_timeout(timeout)
    {
    }
    
    size_t visitCount(SlotVisitor& visitor) const
    {
        return visitor.visitCount() - m_visitCountBefore;
    }

    bool didVisitSomething(SlotVisitor& visitor) const
    {
        return m_didVisitSomething || visitCount(visitor);
    }
    
    bool shouldTimeOut(SlotVisitor& visitor) const
    {
        return didVisitSomething(visitor) && hasElapsed(m_timeout);
    }
    
private:
    bool m_didVisitSomething { false };
    size_t m_visitCountBefore { 0 };
    MonotonicTime m_timeout;
};

} // namespace JSC

