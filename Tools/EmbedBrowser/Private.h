#pragma once

#include <vector>

#include "Browser.h"

// Forward declarations
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitWebContext WebKitWebContext;
typedef struct _WebKitSettings WebKitSettings;
typedef struct _WebKitUserContentManager WebKitUserContentManager;
typedef struct _GtkWidget GtkWidget;

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Browser private class
///////////////////////////////////////////////////////////////////////////////

// Shielded off from Browser.h interface
class TabPrivate
{
public:
    TabPrivate(const class BrowserPrivate* parent_) : initialized(false), parent(parent_) {};
    ~TabPrivate() = default;

    Browser::Tab::CallbackPaint callbackPaint;
    Browser::Tab::CallbackIsLoading callbackIsLoading;
    Browser::Tab::CallbackLoadFailed callbackLoadFailed;
    
    Browser::Tab::CallbackDeniseLoadProduct callbackDeniseLoadProduct;
    Browser::Tab::CallbackDeniseSetOverlay callbackDeniseSetOverlay;
    Browser::Tab::CallbackDeniseSetHeader callbackDeniseSetHeader;
    
    bool initialized;
    WebKitWebView* webView;
    GtkWidget* window;
    
    const class BrowserPrivate* parent;
};
class BrowserPrivate
{
public:
    BrowserPrivate() : initialized(false) {};
    ~BrowserPrivate() = default;

    // Global browser variables, only valid after initialize()
    bool initialized;
    WebKitWebContext* webContext;
    WebKitSettings* settings;
    WebKitUserContentManager* userContentManager;
    
    // Tabs
    std::vector<std::shared_ptr<Browser::Tab>> tabs;
};

}
