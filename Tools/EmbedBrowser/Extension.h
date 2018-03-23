#pragma once

#include "Browser.h"

namespace WebKitEmbed
{

    // ACHTUNG: Static hack to get the Denise::Internal interfaces from Browser.cpp,
    // as there is currently no easy way to get to the Browser instance in which these interfaces are stored as member vars.
    extern Denise::Internal::Wrapper* g_deniseInterfaceWrapper;

    extern void registerWebExtension(Browser *browser);

};
