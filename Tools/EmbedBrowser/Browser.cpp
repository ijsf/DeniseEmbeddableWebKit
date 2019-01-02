#include "Private.h"
#include "Browser.h"
#include "Extension.h"

#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>

#include <pthread.h>
#include <gdk/gdk.h>

#define UNUSED_PARAM(variable) (void)variable

G_BEGIN_DECLS \
    void g_io_gnomeproxy_load(GIOModule *module);
    void g_io_gnutls_load(GIOModule *module);
G_END_DECLS

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

class GlobalData {
public:
    GlobalData()
        : initialized(false)
    {}

    bool initialized;
    uint64_t tid;
};
static GlobalData g_Once;

///////////////////////////////////////////////////////////////////////////////
// GTK callbacks
///////////////////////////////////////////////////////////////////////////////

static gboolean webViewLoadFailed(WebKitWebView *webView, WebKitLoadEvent loadEvent_, const char* URI_, GError* error_, TabPrivate* tabPrivate) {
    if (tabPrivate->callbackLoadFailed) {
        // Translate to our own types
        Browser::LoadEvent loadEvent;
        if (loadEvent_ == WEBKIT_LOAD_STARTED) {
            loadEvent = Browser::LOAD_STARTED;
        }
        else if (loadEvent_ == WEBKIT_LOAD_REDIRECTED) {
            loadEvent = Browser::LOAD_REDIRECTED;
        }
        else if (loadEvent_ == WEBKIT_LOAD_COMMITTED) {
            loadEvent = Browser::LOAD_COMMITTED;
        }
        else if (loadEvent_ == WEBKIT_LOAD_FINISHED) {
            loadEvent = Browser::LOAD_FINISHED;
        }
        else {
            loadEvent = Browser::LOAD_UNKNOWN;
        }
        Browser::Error error = {
            error_->code,
            std::string(error_->message)
        };
        if (tabPrivate->callbackLoadFailed(loadEvent, std::string(URI_), error)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void webViewIsLoadingChanged(WebKitWebView* webView, GParamSpec* paramSpec, TabPrivate* tabPrivate) {
    if (tabPrivate && tabPrivate->callbackIsLoading) {
        tabPrivate->callbackIsLoading(webkit_web_view_is_loading(webView));
    }
}

#if 0
static void webViewReadyToShow(WebKitWebView *webView, class Browser* browser) {
    UNUSED_PARAM(browser);

    WebKitWindowProperties *windowProperties = webkit_web_view_get_window_properties(webView);

    GdkRectangle geometry;
    webkit_window_properties_get_geometry(windowProperties, &geometry);

    printf("webViewReadyToShow (%u,%u,%u,%u)\n", geometry.x, geometry.y, geometry.width, geometry.height);
}
#endif

/* Based on Tools/TestWebKitAPI/glib/WebKitGLib/WebViewTest.cpp */
#include <JavaScriptCore/JSRetainPtr.h>
static char* jsValueToCString(JSGlobalContextRef context, JSValueRef value) {
    g_assert(value);
    g_assert(JSValueIsString(context, value));

    JSRetainPtr<JSStringRef> stringValue(Adopt, JSValueToStringCopy(context, value, 0));
    g_assert(stringValue);

    size_t cStringLength = JSStringGetMaximumUTF8CStringSize(stringValue.get());
    char* cString = static_cast<char*>(g_malloc(cStringLength));
    JSStringGetUTF8CString(stringValue.get(), cString, cStringLength);
    return cString;
}
char* javascriptResultToCString(WebKitJavascriptResult* javascriptResult) {
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    return jsValueToCString(context, webkit_javascript_result_get_value(javascriptResult));
}
///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

static guint keyToGdkKey(const unsigned int key) {
    switch(key) {
        case Browser::Key::KEY_SPACE:
            return GDK_KEY_space;
        case Browser::Key::KEY_ESCAPE:
            return GDK_KEY_Escape;
        case Browser::Key::KEY_RETURN:
            return GDK_KEY_Return;
        case Browser::Key::KEY_TAB:
            return GDK_KEY_Tab;
        case Browser::Key::KEY_DELETE:
            return GDK_KEY_Delete;
        case Browser::Key::KEY_BACKSPACE:
            return GDK_KEY_BackSpace;
        case Browser::Key::KEY_INSERT:
            return GDK_KEY_Insert;
        case Browser::Key::KEY_UP:
            return GDK_KEY_Up;
        case Browser::Key::KEY_DOWN:
            return GDK_KEY_Down;
        case Browser::Key::KEY_LEFT:
            return GDK_KEY_Left;
        case Browser::Key::KEY_RIGHT:
            return GDK_KEY_Right;
        case Browser::Key::KEY_PAGE_UP:
            return GDK_KEY_Page_Up;
        case Browser::Key::KEY_PAGE_DOWN:
            return GDK_KEY_Page_Down;
        case Browser::Key::KEY_HOME:
            return GDK_KEY_Home;
        case Browser::Key::KEY_END:
            return GDK_KEY_End;

        case Browser::Key::KEY_F1:
            return GDK_KEY_F1;
        case Browser::Key::KEY_F2:
            return GDK_KEY_F2;
        case Browser::Key::KEY_F3:
            return GDK_KEY_F3;
        case Browser::Key::KEY_F4:
            return GDK_KEY_F4;
        case Browser::Key::KEY_F5:
            return GDK_KEY_F5;
        case Browser::Key::KEY_F6:
            return GDK_KEY_F6;
        case Browser::Key::KEY_F7:
            return GDK_KEY_F7;
        case Browser::Key::KEY_F8:
            return GDK_KEY_F8;
        case Browser::Key::KEY_F9:
            return GDK_KEY_F9;
        case Browser::Key::KEY_F10:
            return GDK_KEY_F10;
        case Browser::Key::KEY_F11:
            return GDK_KEY_F11;
        case Browser::Key::KEY_F12:
            return GDK_KEY_F12;

        case Browser::Key::KEY_NUMPAD_0:
            return GDK_KEY_KP_0;
        case Browser::Key::KEY_NUMPAD_1:
            return GDK_KEY_KP_1;
        case Browser::Key::KEY_NUMPAD_2:
            return GDK_KEY_KP_2;
        case Browser::Key::KEY_NUMPAD_3:
            return GDK_KEY_KP_3;
        case Browser::Key::KEY_NUMPAD_4:
            return GDK_KEY_KP_4;
        case Browser::Key::KEY_NUMPAD_5:
            return GDK_KEY_KP_5;
        case Browser::Key::KEY_NUMPAD_6:
            return GDK_KEY_KP_6;
        case Browser::Key::KEY_NUMPAD_7:
            return GDK_KEY_KP_7;
        case Browser::Key::KEY_NUMPAD_8:
            return GDK_KEY_KP_8;
        case Browser::Key::KEY_NUMPAD_9:
            return GDK_KEY_KP_9;

        case Browser::Key::KEY_NUMPAD_ADD:
            return GDK_KEY_KP_Add;
        case Browser::Key::KEY_NUMPAD_SUBTRACT:
            return GDK_KEY_KP_Subtract;
        case Browser::Key::KEY_NUMPAD_MULTIPLY:
            return GDK_KEY_KP_Multiply;
        case Browser::Key::KEY_NUMPAD_DIVIDE:
            return GDK_KEY_KP_Divide;
        case Browser::Key::KEY_NUMPAD_SEPARATOR:
            return GDK_KEY_KP_Separator;
        case Browser::Key::KEY_NUMPAD_DECIMAL:
            return GDK_KEY_KP_Decimal;
        case Browser::Key::KEY_NUMPAD_EQUAL:
            return GDK_KEY_KP_Equal;
        case Browser::Key::KEY_NUMPAD_DELETE:
            return GDK_KEY_KP_Delete;
        default:
            // Must be an ASCII keycode
            return (guint)key;
    };
}

static guint modifierToGdkState(Browser::ModifierKeys keys) {
    guint state = 0;
    
    if (keys & Browser::MODIFIER_SHIFT) {
        state |= GDK_SHIFT_MASK;
    }
    if (keys & Browser::MODIFIER_CTRL) {
        state |= GDK_CONTROL_MASK;
    }
    if (keys & Browser::MODIFIER_ALT) {
        state |= GDK_META_MASK;
    }

    return state;
}

static guint modifierToGdkButton(Browser::ModifierKeys keys) {
    if (keys & Browser::MODIFIER_LEFT_MOUSE) {
        return GDK_BUTTON_PRIMARY;
    }
    else if (keys & Browser::MODIFIER_RIGHT_MOUSE) {
        return GDK_BUTTON_SECONDARY;
    }
    else if (keys & Browser::MODIFIER_MIDDLE_MOUSE) {
        return GDK_BUTTON_MIDDLE;
    }

    return 0;
}

// Source/WebKit/UIProcess/Automation/gtk/WebAutomationSessionGtk.cpp
static void doMouseEvent(GdkEventType type, GtkWidget* widget, int x, int y, guint button, guint state) {
    assert(type == GDK_BUTTON_PRESS || type == GDK_BUTTON_RELEASE);

    GdkEvent *event = gdk_event_new(type);
    event->button.window = gtk_widget_get_window(widget);
    g_object_ref(event->button.window);
    event->button.time = GDK_CURRENT_TIME;
    event->button.x = x;
    event->button.y = y;
    event->button.axes = 0;
    event->button.state = state;
    event->button.button = button;
    event->button.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(widget)));
    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(widget), x, y, &xRoot, &yRoot);
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;
    gtk_main_do_event(event);
    //g_free(event);
}

// Source/WebKit/UIProcess/Automation/gtk/WebAutomationSessionGtk.cpp
static void doMotionEvent(GtkWidget* widget, int x, int y, guint state) {
    GdkEvent *event = gdk_event_new(GDK_MOTION_NOTIFY);
    event->motion.window = gtk_widget_get_window(widget);
    g_object_ref(event->motion.window);
    event->motion.time = GDK_CURRENT_TIME;
    event->motion.x = x;
    event->motion.y = y;
    event->motion.axes = 0;
    event->motion.state = state;
    event->motion.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(widget)));
    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(widget), x, y, &xRoot, &yRoot);
    event->motion.x_root = xRoot;
    event->motion.y_root = yRoot;
    gtk_main_do_event(event);
    //g_free(event);
}

static void doFocusInEvent(GtkWidget* widget) {
    GdkEvent* fevent = gdk_event_new(GDK_FOCUS_CHANGE);
    fevent->focus_change.type = GDK_FOCUS_CHANGE;
    fevent->focus_change.in = TRUE;
    fevent->focus_change.window = gtk_widget_get_window(widget);
    if (fevent->focus_change.window != NULL) {
        g_object_ref (fevent->focus_change.window);
    }

    gtk_widget_send_focus_change(widget, fevent);

    //gdk_event_free (fevent);
}

static void doKeyStrokeEvent(GdkEventType type, GtkWidget* widget, guint keyVal, guint state, bool doReleaseAfterPress = false) {
    GdkEvent* event = gdk_event_new(type);
    event->key.keyval = keyVal;

    event->key.time = GDK_CURRENT_TIME;
    event->key.window = gtk_widget_get_window(widget);
    g_object_ref(event->key.window);
    gdk_event_set_device(event, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(widget))));
    event->key.state = state;

    // When synthesizing an event, an invalid hardware_keycode value can cause it to be badly processed by GTK+.
    GdkKeymapKey* keys;
    int keysCount;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keyVal, &keys, &keysCount) && keysCount)
        event->key.hardware_keycode = keys[0].keycode;

    gtk_main_do_event(event);
    if (doReleaseAfterPress) {
        event->key.type = GDK_KEY_RELEASE;
        gtk_main_do_event(event);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Browser class
///////////////////////////////////////////////////////////////////////////////

Browser::Browser()
    : m_private(new BrowserPrivate())
{
    // Do do any initialization in here since this may depend on virtual functions (which are not yet available here)
}

Browser::~Browser() {
    if (m_private->userContentManager) {
        g_object_unref(m_private->userContentManager);
        m_private->userContentManager = nullptr;
    }
    if (m_private->settings) {
        g_object_unref(m_private->settings);
        m_private->settings = nullptr;
    }
    if (m_private->webContext) {
        g_object_unref(m_private->webContext);
        m_private->webContext = nullptr;
    }
    m_private->initialized = false;
}

void Browser::initialize() {
    // Perform one-time initialization if necessary (NOT thread-safe)
    if (!g_Once.initialized) {
        g_Once.initialized = true;

        gtk_init_check(nullptr, nullptr);

        //
        // Forcibly disable glib's runtime giomodule loading, because we don't want ANYTHING system-based to be loaded by glib
        // Unfortunately this would require a custom build of glib, and linkage of this project against that glib.
        //
        // Since this is currently too much work, we just override some environment variables here that glib needs,
        // and load the required modules for TLS/SSL manually (note that these are linked in statically).
        {
            // Override conditions for _g_io_modules_ensure_loaded in glib/gio/giomodule.c
            g_unsetenv("GIO_EXTRA_MODULES");
            
            // Override conditions for get_gio_module_dir in glib/gio/giomodule.c
            g_setenv("GIO_MODULE_DIR", "nonworkingpath", true);
        }
        {
            GIOModule* module = (GIOModule*)g_object_new(G_IO_TYPE_MODULE, NULL);
            g_io_gnomeproxy_load(NULL);
        }
        {
            GIOModule* module = (GIOModule*)g_object_new(G_IO_TYPE_MODULE, NULL);
            g_io_gnutls_load(NULL);
        }

        // Make sure the default im module imquartz is not used on OS X, as this will cause a crash with GtkOffscreenWindow
        g_object_set(gtk_settings_get_default(), "gtk-im-module", "gtk-im-context-simple", nullptr);

        // Store thread id for thread safety checks
        pthread_threadid_np(nullptr, &g_Once.tid);
    }

    // Create WebKitWebContext explicitly
    // This is to avoid webkit_web_context_get_default() which uses an undestroyable GOnce
    m_private->webContext = webkit_web_context_new();

    // Faux single process mode for global context
    webkit_web_context_set_process_model(m_private->webContext, WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS);

    // Settings
    m_private->settings = webkit_settings_new();
    webkit_settings_set_enable_developer_extras(m_private->settings, FALSE);
    webkit_settings_set_enable_webgl(m_private->settings, FALSE);
    webkit_settings_set_enable_media_stream(m_private->settings, FALSE);
    webkit_settings_set_enable_java(m_private->settings, FALSE);
    webkit_settings_set_enable_plugins(m_private->settings, FALSE);
    webkit_settings_set_javascript_can_open_windows_automatically(m_private->settings, FALSE);
    webkit_settings_set_enable_developer_extras(m_private->settings, FALSE);
    webkit_settings_set_javascript_can_access_clipboard(m_private->settings, TRUE);
    webkit_settings_set_enable_write_console_messages_to_stdout(m_private->settings, TRUE);

    // Create WebKitUserContentManager
    m_private->userContentManager = webkit_user_content_manager_new();

    m_private->initialized = true;
}

bool Browser::isInitialized() const {
    return m_private->initialized;
}

std::shared_ptr<Browser::Tab> Browser::createTab() {
    // Create instance and insert into tabs map
    std::shared_ptr<Browser::Tab> tab(new Browser::Tab(m_private->currentTabIndex, m_private.get()));
    m_private->tabs.insert(std::pair<size_t, std::shared_ptr<Browser::Tab>>(m_private->currentTabIndex, tab));
    ++m_private->currentTabIndex;
    return tab;
}

Browser::Tab::Tab(const Browser::Tab::Index tabIndex, BrowserPrivate* parent)
: m_private(new TabPrivate(tabIndex, parent)) {
}

Browser::Tab::~Tab() {
    // Remove from tabs map
    {
        BrowserPrivate::TabMap& tabs = m_private->parent->tabs;
        auto it = tabs.find(m_private->tabIndex);
        assert(it != tabs.end());
        tabs.erase(it);
    }

    // Cleanup
    if (m_private->webView) {
        gtk_widget_destroy(GTK_WIDGET(m_private->webView));
        m_private->webView = nullptr;
    }
    if (m_private->window) {
        gtk_widget_destroy(m_private->window);
        m_private->window = nullptr;
    }
    m_private->initialized = false;
}

void Browser::Tab::initialize(const unsigned int width, const unsigned int height) {
    // Create offscreen window
    m_private->window = gtk_offscreen_window_new();
    gtk_window_set_default_size(GTK_WINDOW(m_private->window), width, height);
    
    // Create WebKitWebView with default global context (GtkWidget)
    // Based on webkit_web_view_new_with_user_content_manager and webkit_web_view_new_with_context
    m_private->webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
        "is-ephemeral", false,
        "web-context", m_private->parent->webContext,
        "user-content-manager", m_private->parent->userContentManager,
        nullptr));
    g_assert(webkit_web_view_get_user_content_manager(m_private->webView) == m_private->parent->userContentManager);
    webkit_web_view_set_settings(m_private->webView, m_private->parent->settings);
    
    // Callbacks
    //g_signal_connect(m_private->webView, "ready-to-show", G_CALLBACK(webViewReadyToShow), this);
    g_signal_connect(m_private->webView, "notify::is-loading", G_CALLBACK(webViewIsLoadingChanged), m_private.get());
    g_signal_connect(m_private->webView, "load-failed", G_CALLBACK(webViewLoadFailed), m_private.get());
    
    // Set transparent background color -- ACHTUNG requires WebKit transparency branch
    //const GdkRGBA backgroundColor = { 0, 0, 0, 0 };
    //webkit_web_view_set_background_color(m_private->webView, &backgroundColor);

    // Set paint callback
    using namespace std::placeholders;
    webkit_web_view_set_paint_callback(m_private->webView, [=](uint8_t *data, float scaling, const GdkPoint &dstPoint, const GdkRectangle &srcSize, const GdkRectangle &srcRect)->void {
        // Callback
        if(m_private->callbackPaint) {
            const Point dstPoint_ = { (unsigned int)dstPoint.x, (unsigned int)dstPoint.y };
            const Rect srcSize_ = { (unsigned int)srcSize.x, (unsigned int)srcSize.y, (unsigned int)srcSize.width, (unsigned int)srcSize.height };
            const Rect srcRect_ = { (unsigned int)srcRect.x, (unsigned int)srcRect.y, (unsigned int)srcRect.width, (unsigned int)srcRect.height };
            m_private->callbackPaint(data, dstPoint_ ,srcSize_, srcRect_);
        }
    });
    
    gtk_container_add(GTK_CONTAINER(m_private->window), GTK_WIDGET(m_private->webView));

    // Show (and implicitly realize) entire widget tree
    // NOTE: This call may crash due to a bug in gtk+3 on OS X, see https://bugzilla.gnome.org/show_bug.cgi?id=667721
    gtk_widget_show_all(m_private->window);
    //gtk_widget_realize(GTK_WIDGET(m_private->webView));
    //gtk_widget_grab_focus(GTK_WIDGET(m_private->webView));

    // Call focus-in event once to enable keyboard focus (and carets and such)
    doFocusInEvent(GTK_WIDGET(m_private->webView));
    
    // Register Denise web extension
    registerWebExtension(m_private->parent, m_private->tabIndex);

    m_private->initialized = true;
}

bool Browser::Tab::isInitialized() const {
    return m_private->initialized;
}

#if 0
void Browser::tick() {
    // Thread safety check (caller thread must be identical for all GTK calls)
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    assert(tid == g_Once.tid);
    
    // ACHTUNG check if necessary
    gtk_main_iteration_do(FALSE);
}
#endif

void Browser::Tab::setSize(const unsigned int width, const unsigned int height) {
    printf("Browser::setSize(%u, %u)\n", width, height);
    assert(isInitialized());
    if (isInitialized()) {
        // Size the window, though this appears to be in vain
        //gtk_window_set_default_size(GTK_WINDOW(m_private->window), width, height);
        //gtk_window_resize(GTK_WINDOW(m_private->window), width, height);

        // Performing a size_allocate on the webView widget seems to trigger the right callback(s)
        GtkAllocation alloc = { 0, 0, (int)width, (int)height };
        gtk_widget_size_allocate(GTK_WIDGET(m_private->webView), &alloc);
    }
}

void Browser::Tab::mouseMove(int x, int y, ModifierKeys modifier) {
    assert(isInitialized());
    if (isInitialized()) {
        doMotionEvent(GTK_WIDGET(m_private->webView), x, y, modifierToGdkState(modifier));
    }
}

void Browser::Tab::mouseDown(int x, int y, ModifierKeys modifier) {
    assert(isInitialized());
    if (isInitialized()) {
        doMouseEvent(
            GDK_BUTTON_PRESS,
            GTK_WIDGET(m_private->webView),
            x,
            y,
            modifierToGdkButton(modifier),
            modifierToGdkState(modifier)
        );
    }
}

void Browser::Tab::mouseUp(int x, int y, ModifierKeys modifier) {
    assert(isInitialized());
    if (isInitialized()) {
        doMouseEvent(
            GDK_BUTTON_RELEASE,
            GTK_WIDGET(m_private->webView),
            x,
            y,
            modifierToGdkButton(modifier),
            modifierToGdkState(modifier)
        );
    }
}

void Browser::Tab::keyPress(const unsigned int key, const ModifierKeys modifierKeys) {
    assert(isInitialized());
    if (isInitialized()) {
        // Immediate press + release
        doKeyStrokeEvent(
            GDK_KEY_PRESS,
            GTK_WIDGET(m_private->webView),
            keyToGdkKey(key),
            modifierToGdkState(modifierKeys),
            true
        );
    }
}

void Browser::Tab::loadURL(const std::string &url) {
    assert(isInitialized());
    if (isInitialized()) {
        webkit_web_view_load_uri(m_private->webView, url.c_str());
    }
}

void Browser::Tab::setCallbackPaint(CallbackPaint fn) {
    m_private->callbackPaint = fn;
}

void Browser::Tab::setCallbackIsLoading(CallbackIsLoading fn) {
    m_private->callbackIsLoading = fn;
}

void Browser::Tab::setCallbackLoadFailed(CallbackLoadFailed fn) {
    m_private->callbackLoadFailed = fn;
}

void Browser::Tab::setCallbackDeniseLoadProduct(CallbackDeniseLoadProduct fn) {
    m_private->callbackDeniseLoadProduct = fn;
}

void Browser::Tab::setCallbackDeniseSetOverlay(CallbackDeniseSetOverlay fn) {
    m_private->callbackDeniseSetOverlay = fn;
}

void Browser::Tab::setCallbackDeniseSetHeader(CallbackDeniseSetHeader fn) {
    m_private->callbackDeniseSetHeader = fn;
}

}
