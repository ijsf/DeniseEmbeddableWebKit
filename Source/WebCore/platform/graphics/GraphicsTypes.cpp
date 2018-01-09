/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2012 Rik Cabanier (cabanier@adobe.com)
 * Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.
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
#include "GraphicsTypes.h"

#include <wtf/Assertions.h>
#include <wtf/text/TextStream.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static const char* const compositeOperatorNames[] = {
    "clear",
    "copy",
    "source-over",
    "source-in",
    "source-out",
    "source-atop",
    "destination-over",
    "destination-in",
    "destination-out",
    "destination-atop",
    "xor",
    "darker",
    "lighter",
    "difference"
};

static const char* const blendOperatorNames[] = {
    "normal",
    "multiply",
    "screen",
    "darken",
    "lighten",
    "overlay",
    "color-dodge",
    "color-burn",
    "hard-light",
    "soft-light",
    "difference",
    "exclusion",
    "hue",
    "saturation",
    "color",
    "luminosity",
    "plus-darker",
    "plus-lighter"
};
const int numCompositeOperatorNames = WTF_ARRAY_LENGTH(compositeOperatorNames);
const int numBlendOperatorNames = WTF_ARRAY_LENGTH(blendOperatorNames);

bool parseBlendMode(const String& s, BlendMode& blendMode)
{
    for (int i = 0; i < numBlendOperatorNames; i++) {
        if (s == blendOperatorNames[i]) {
            blendMode = static_cast<BlendMode>(i + BlendModeNormal);
            return true;
        }
    }
    
    return false;
}

bool parseCompositeAndBlendOperator(const String& s, CompositeOperator& op, BlendMode& blendOp)
{
    for (int i = 0; i < numCompositeOperatorNames; i++) {
        if (s == compositeOperatorNames[i]) {
            op = static_cast<CompositeOperator>(i);
            blendOp = BlendModeNormal;
            return true;
        }
    }
    
    if (parseBlendMode(s, blendOp)) {
        // For now, blending will always assume source-over. This will be fixed in the future
        op = CompositeSourceOver;
        return true;
    }
    
    return false;
}

// FIXME: when we support blend modes in combination with compositing other than source-over
// this routine needs to be updated.
String compositeOperatorName(CompositeOperator op, BlendMode blendOp)
{
    ASSERT(op >= 0);
    ASSERT(op < numCompositeOperatorNames);
    ASSERT(blendOp >= BlendModeNormal);
    ASSERT(blendOp <= numBlendOperatorNames);
    if (blendOp > BlendModeNormal)
        return blendOperatorNames[blendOp - BlendModeNormal];
    return compositeOperatorNames[op];
}

static String blendModeName(BlendMode blendOp)
{
    ASSERT(blendOp >= BlendModeNormal);
    ASSERT(blendOp <= BlendModePlusLighter);
    return blendOperatorNames[blendOp - BlendModeNormal];
}

bool parseLineCap(const String& s, LineCap& cap)
{
    if (s == "butt") {
        cap = ButtCap;
        return true;
    }
    if (s == "round") {
        cap = RoundCap;
        return true;
    }
    if (s == "square") {
        cap = SquareCap;
        return true;
    }
    return false;
}

String lineCapName(LineCap cap)
{
    ASSERT(cap >= 0);
    ASSERT(cap < 3);
    const char* const names[3] = { "butt", "round", "square" };
    return names[cap];
}

bool parseLineJoin(const String& s, LineJoin& join)
{
    if (s == "miter") {
        join = MiterJoin;
        return true;
    }
    if (s == "round") {
        join = RoundJoin;
        return true;
    }
    if (s == "bevel") {
        join = BevelJoin;
        return true;
    }
    return false;
}

String lineJoinName(LineJoin join)
{
    ASSERT(join >= 0);
    ASSERT(join < 3);
    const char* const names[3] = { "miter", "round", "bevel" };
    return names[join];
}

String textAlignName(TextAlign align)
{
    ASSERT(align >= 0);
    ASSERT(align < 5);
    const char* const names[5] = { "start", "end", "left", "center", "right" };
    return names[align];
}

bool parseTextAlign(const String& s, TextAlign& align)
{
    if (s == "start") {
        align = StartTextAlign;
        return true;
    }
    if (s == "end") {
        align = EndTextAlign;
        return true;
    }
    if (s == "left") {
        align = LeftTextAlign;
        return true;
    }
    if (s == "center") {
        align = CenterTextAlign;
        return true;
    }
    if (s == "right") {
        align = RightTextAlign;
        return true;
    }
    return false;
}

String textBaselineName(TextBaseline baseline)
{
    ASSERT(baseline >= 0);
    ASSERT(baseline < 6);
    const char* const names[6] = { "alphabetic", "top", "middle", "bottom", "ideographic", "hanging" };
    return names[baseline];
}

bool parseTextBaseline(const String& s, TextBaseline& baseline)
{
    if (s == "alphabetic") {
        baseline = AlphabeticTextBaseline;
        return true;
    }
    if (s == "top") {
        baseline = TopTextBaseline;
        return true;
    }
    if (s == "middle") {
        baseline = MiddleTextBaseline;
        return true;
    }
    if (s == "bottom") {
        baseline = BottomTextBaseline;
        return true;
    }
    if (s == "ideographic") {
        baseline = IdeographicTextBaseline;
        return true;
    }
    if (s == "hanging") {
        baseline = HangingTextBaseline;
        return true;
    }
    return false;
}

TextStream& operator<<(TextStream& ts, CompositeOperator op)
{
    return ts << compositeOperatorName(op, BlendModeNormal);
}

TextStream& operator<<(TextStream& ts, BlendMode blendMode)
{
    return ts << blendModeName(blendMode);
}

TextStream& operator<<(TextStream& ts, WindRule rule)
{
    switch (rule) {
    case RULE_NONZERO:
        ts << "NON-ZERO";
        break;
    case RULE_EVENODD:
        ts << "EVEN-ODD";
        break;
    }

    return ts;
}

TextStream& operator<<(TextStream& ts, LineCap capStyle)
{
    switch (capStyle) {
    case ButtCap:
        ts << "BUTT";
        break;
    case RoundCap:
        ts << "ROUND";
        break;
    case SquareCap:
        ts << "SQUARE";
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, LineJoin joinStyle)
{
    switch (joinStyle) {
    case MiterJoin:
        ts << "MITER";
        break;
    case RoundJoin:
        ts << "ROUND";
        break;
    case BevelJoin:
        ts << "BEVEL";
        break;
    }
    return ts;
}


}
