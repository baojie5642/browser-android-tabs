/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/timing/Performance.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/inspector/InspectedFrames.h"
#include "core/inspector/InspectorWebPerfAgent.h"
#include "core/loader/DocumentLoader.h"
#include "core/origin_trials/OriginTrials.h"
#include "core/timing/PerformanceTiming.h"

namespace blink {

static double toTimeOrigin(LocalFrame* frame) {
  if (!frame)
    return 0.0;

  Document* document = frame->document();
  if (!document)
    return 0.0;

  DocumentLoader* loader = document->loader();
  if (!loader)
    return 0.0;

  return loader->timing().referenceMonotonicTime();
}

Performance::Performance(LocalFrame* frame)
    : PerformanceBase(toTimeOrigin(frame)), DOMWindowProperty(frame) {}

Performance::~Performance() {}

ExecutionContext* Performance::getExecutionContext() const {
  if (!frame())
    return nullptr;
  return frame()->document();
}

MemoryInfo* Performance::memory() {
  return MemoryInfo::create();
}

PerformanceNavigation* Performance::navigation() const {
  if (!m_navigation)
    m_navigation = PerformanceNavigation::create(frame());

  return m_navigation.get();
}

PerformanceTiming* Performance::timing() const {
  if (!m_timing)
    m_timing = PerformanceTiming::create(frame());

  return m_timing.get();
}

void Performance::updateLongTaskInstrumentation() {
  if (hasObserverFor(PerformanceEntry::LongTask) && !m_longTaskInspectorAgent) {
    if (!frame() || !frame()->document() ||
        !OriginTrials::longTaskObserverEnabled(frame()->document()))
      return;
    m_longTaskInspectorAgent = new InspectorWebPerfAgent(frame());
    m_longTaskInspectorAgent->enable();
  } else if (!hasObserverFor(PerformanceEntry::LongTask) &&
             m_longTaskInspectorAgent) {
    m_longTaskInspectorAgent->disable();
    m_longTaskInspectorAgent = nullptr;
  }
}

DEFINE_TRACE(Performance) {
  visitor->trace(m_navigation);
  visitor->trace(m_timing);
  visitor->trace(m_longTaskInspectorAgent);
  DOMWindowProperty::trace(visitor);
  PerformanceBase::trace(visitor);
}

}  // namespace blink
