#pragma once

#include <map>

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
    TabPrivate(const Browser::Tab::Index tabIndex_, class BrowserPrivate* parent_) : initialized(false), tabIndex(tabIndex_), parent(parent_) {};
    ~TabPrivate() = default;

    Browser::Tab::CallbackPaint callbackPaint;
    Browser::Tab::CallbackIsLoading callbackIsLoading;
    Browser::Tab::CallbackLoadFailed callbackLoadFailed;
    
    Browser::Tab::CallbackDeniseLoadProduct callbackDeniseLoadProduct;
    Browser::Tab::CallbackDeniseSetOverlay callbackDeniseSetOverlay;
    Browser::Tab::CallbackDeniseSetHeader callbackDeniseSetHeader;
	Browser::Tab::CallbackDeniseAppNotificationSet callbackDeniseAppNotificationSet;
	Browser::Tab::CallbackDeniseAppNotificationReset callbackDeniseAppNotificationReset;
	    
    bool initialized;
    WebKitWebView* webView;
    GtkWidget* window;
    
    const Browser::Tab::Index tabIndex;
    class BrowserPrivate* parent;
};
class BrowserPrivate
{
public:
    BrowserPrivate() : initialized(false), currentTabIndex(0) {};
    ~BrowserPrivate() = default;

    // Global browser variables, only valid after initialize()
    bool initialized;
    WebKitWebContext* webContext;
    WebKitSettings* settings;
    WebKitUserContentManager* userContentManager;
    
    // Tabs
    typedef std::map<Browser::Tab::Index, std::shared_ptr<Browser::Tab>> TabMap;
    TabMap tabs;
    Browser::Tab::Index currentTabIndex;
};

}
