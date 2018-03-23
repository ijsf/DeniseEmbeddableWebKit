#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>

#include <pthread.h>
#include <gdk/gdk.h>

#include "Browser.h"
#include "Extension.h"

#define UNUSED_PARAM(variable) (void)variable

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

class WebKitGlobalData {
public:
    WebKitGlobalData()
        : initialized(false)
    {}

    bool initialized;
    WebKitSettings *settings;
    uint64_t tid;
};
static WebKitGlobalData g_WebKitData;

///////////////////////////////////////////////////////////////////////////////
// GTK callbacks
///////////////////////////////////////////////////////////////////////////////

static void webViewReadyToShow(WebKitWebView *webView, class Browser *browser)
{
    UNUSED_PARAM(browser);

    WebKitWindowProperties *windowProperties = webkit_web_view_get_window_properties(webView);

    GdkRectangle geometry;
    webkit_window_properties_get_geometry(windowProperties, &geometry);

    printf("webViewReadyToShow (%u,%u,%u,%u)\n", geometry.x, geometry.y, geometry.width, geometry.height);
}

/* Based on Tools/TestWebKitAPI/glib/WebKitGLib/WebViewTest.cpp */
#include <JavaScriptCore/JSRetainPtr.h>
static char* jsValueToCString(JSGlobalContextRef context, JSValueRef value)
{
    g_assert(value);
    g_assert(JSValueIsString(context, value));

    JSRetainPtr<JSStringRef> stringValue(Adopt, JSValueToStringCopy(context, value, 0));
    g_assert(stringValue);

    size_t cStringLength = JSStringGetMaximumUTF8CStringSize(stringValue.get());
    char* cString = static_cast<char*>(g_malloc(cStringLength));
    JSStringGetUTF8CString(stringValue.get(), cString, cStringLength);
    return cString;
}
char* javascriptResultToCString(WebKitJavascriptResult* javascriptResult)
{
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

static guint modifierToGdkState(Browser::ModifierKeys keys)
{
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

static guint modifierToGdkButton(Browser::ModifierKeys keys)
{
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
static void doMouseEvent(GdkEventType type, GtkWidget* widget, int x, int y, guint button, guint state)
{
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
static void doMotionEvent(GtkWidget* widget, int x, int y, guint state)
{
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

static void doKeyStrokeEvent(GdkEventType type, GtkWidget* widget, guint keyVal, guint state, bool doReleaseAfterPress = false)
{
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

// Private structure, shielded off from Browser.h interface
class BrowserPrivate
{
public:
    BrowserPrivate() : initialized(false) {}

    bool initialized;
    Browser::PaintCallback paintCallback;
    
    // Only valid after initialize()
    WebKitUserContentManager *userContentManager;
    WebKitWebView *webView;
    GtkWidget *window;
    
    /// DENISE BEGIN
    Denise::Internal::Wrapper* m_deniseInterfaceWrapper;
    /// DENISE END
};

Browser::Browser()
    : m_private(new BrowserPrivate())
{
    // Do do any initialization in here since this may depend on virtual functions (which are not yet available here)
}

Browser::~Browser()
{
    // Reset the paint callback
    webkit_web_view_set_paint_callback(m_private->webView, WebKitWebViewPaintCallback());
    
    // Destroy (GtkWidget)m_private->webView
    // Destroy m_private->userContentManager?
    // Destroy (GtkWidget)m_private->window
}

void Browser::initialize(int width, int height)
{
    // Perform one-time initialization if necessary (NOT thread-safe)
    if (!g_WebKitData.initialized) {
        g_WebKitData.initialized = true;

        gtk_init(nullptr, nullptr);   // THREADCHECK
        
        // Make sure the default im module imquartz is not used on OS X, as this will cause a crash with GtkOffscreenWindow
        g_object_set(gtk_settings_get_default(), "gtk-im-module", "gtk-im-context-simple", nullptr);

        WebKitWebContext *globalContext = webkit_web_context_get_default();

        // Store thread id for thread safety checks
        pthread_threadid_np(nullptr, &g_WebKitData.tid);

        // Settings
        g_WebKitData.settings = webkit_settings_new();
        webkit_settings_set_enable_developer_extras(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_webgl(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_media_stream(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_java(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_plugins(g_WebKitData.settings, FALSE);
        webkit_settings_set_javascript_can_open_windows_automatically(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_developer_extras(g_WebKitData.settings, FALSE);
        webkit_settings_set_javascript_can_access_clipboard(g_WebKitData.settings, TRUE);
        webkit_settings_set_enable_write_console_messages_to_stdout(g_WebKitData.settings, TRUE);

        // Faux single process mode for global context
        webkit_web_context_set_process_model(globalContext, WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS);
    }

    // Create offscreen window
    GtkWidget *window = gtk_offscreen_window_new();
    m_private->window = window;
    gtk_window_set_default_size(GTK_WINDOW(m_private->window), width, height);

    // Create WebKitUserContentManager
    m_private->userContentManager = webkit_user_content_manager_new();

    // Create WebKitWebView with default global context (GtkWidget)
    m_private->webView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(m_private->userContentManager));
    g_assert(webkit_web_view_get_user_content_manager(m_private->webView) == m_private->userContentManager);
    webkit_web_view_set_settings(m_private->webView, g_WebKitData.settings);
    g_signal_connect(m_private->webView, "ready-to-show", G_CALLBACK(webViewReadyToShow), this);
    
    // Set transparent background color -- ACHTUNG requires WebKit transparency branch
    //const GdkRGBA backgroundColor = { 0, 0, 0, 0 };
    //webkit_web_view_set_background_color(m_private->webView, &backgroundColor);

    // Set paint callback
    using namespace std::placeholders;
    webkit_web_view_set_paint_callback(m_private->webView, [=](uint8_t *data, float scaling, const GdkPoint &dstPoint, const GdkRectangle &srcSize, const GdkRectangle &srcRect)->void {
        // Callback
        if(m_private->paintCallback) {
            const Point dstPoint_ = { (unsigned int)dstPoint.x, (unsigned int)dstPoint.y };
            const Rect srcSize_ = { (unsigned int)srcSize.x, (unsigned int)srcSize.y, (unsigned int)srcSize.width, (unsigned int)srcSize.height };
            const Rect srcRect_ = { (unsigned int)srcRect.x, (unsigned int)srcRect.y, (unsigned int)srcRect.width, (unsigned int)srcRect.height };
            m_private->paintCallback(data, dstPoint_ ,srcSize_, srcRect_);
        }
    });
    
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(m_private->webView));

    // Show (and implicitly realize) entire widget tree
    // NOTE: This call may crash due to a bug in gtk+3 on OS X, see https://bugzilla.gnome.org/show_bug.cgi?id=667721
    gtk_widget_show_all(window);
    
    /// DENISE BEGIN
    registerWebExtension(this);
    /// DENISE END

    m_private->initialized = true;
}

void Browser::tick()
{
    // Thread safety check (caller thread must be identical for all GTK calls)
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    assert(tid == g_WebKitData.tid);
    
    // ACHTUNG check if necessary
    gtk_main_iteration_do(FALSE);
}

bool Browser::isInitialized() const
{
    return m_private->initialized;
}

void Browser::setSize(int width, int height)
{
    assert(isInitialized());
    
    gtk_window_set_default_size(GTK_WINDOW(m_private->window), width, height);
}

void Browser::mouseMove(int x, int y, ModifierKeys modifier)
{
    doMotionEvent(GTK_WIDGET(m_private->webView), x, y, modifierToGdkState(modifier));
}

void Browser::mouseDown(int x, int y, ModifierKeys modifier)
{
    doMouseEvent(
        GDK_BUTTON_PRESS,
        GTK_WIDGET(m_private->webView),
        x,
        y,
        modifierToGdkButton(modifier),
        modifierToGdkState(modifier)
    );
}

void Browser::mouseUp(int x, int y, ModifierKeys modifier)
{
    doMouseEvent(
        GDK_BUTTON_RELEASE,
        GTK_WIDGET(m_private->webView),
        x,
        y,
        modifierToGdkButton(modifier),
        modifierToGdkState(modifier)
    );
}

void Browser::keyPress(const unsigned int key, const ModifierKeys modifierKeys)
{
    // Immediate press + release
    doKeyStrokeEvent(
        GDK_KEY_PRESS,
        GTK_WIDGET(m_private->webView),
        keyToGdkKey(key),
        modifierToGdkState(modifierKeys),
        true
    );
}

void Browser::loadURL(const std::string &url)
{
    webkit_web_view_load_uri(m_private->webView, url.c_str());
}

void Browser::setPaintCallback(PaintCallback fn)
{
    m_private->paintCallback = fn;
}

// ACHTUNG: Static hack to get the Denise::Internal interfaces to Extension.cpp,
// as there is currently no easy way to get to the Browser instance in which these interfaces are stored as member vars.
#include "Extension.h"
Denise::Internal::Wrapper* g_deniseInterfaceWrapper;
    
void Browser::deniseSetWrapperInterface(Denise::Internal::Wrapper* interface)
{
    m_private->m_deniseInterfaceWrapper = g_deniseInterfaceWrapper = interface;
}

}
