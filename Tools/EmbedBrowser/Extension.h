#pragma once

#include <memory>

#include "Browser.h"

namespace WebKitEmbed
{
    extern void registerWebExtension(class BrowserPrivate* tab, const Browser::Tab::Index tabIndex);
};
