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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GPUCommandQueue.h"

#if ENABLE(WEBGPU)

#include "GPUCommandBuffer.h"
#include "GPUDevice.h"
#include "Logging.h"

namespace WebCore {

RefPtr<GPUCommandQueue> GPUCommandQueue::create(GPUDevice* device)
{
    RefPtr<GPUCommandQueue> queue = adoptRef(new GPUCommandQueue(device));
    return queue;
}

GPUCommandQueue::~GPUCommandQueue()
{
    LOG(WebGPU, "GPUCommandQueue::~GPUCommandQueue()");
}

RefPtr<GPUCommandBuffer> GPUCommandQueue::createCommandBuffer()
{
    return GPUCommandBuffer::create(this);
}

#if !PLATFORM(COCOA)
String GPUCommandQueue::label() const
{
    return emptyString();
}

void GPUCommandQueue::setLabel(const String&)
{
}
#endif

} // namespace WebCore

#endif
