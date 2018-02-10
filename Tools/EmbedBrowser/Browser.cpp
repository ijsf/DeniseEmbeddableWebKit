#include <webkit2/webkit2.h>
#include <pthread.h>
#include <gdk/gdk.h>

#include "Browser.h"

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

static void webViewReadyToShow(WebKitWebView *webView, class Browser *data)
{
    WebKitWindowProperties *windowProperties = webkit_web_view_get_window_properties(webView);

    GdkRectangle geometry;
    webkit_window_properties_get_geometry(windowProperties, &geometry);

    printf("webViewReadyToShow (%u,%u,%u,%u)\n", geometry.x, geometry.y, geometry.width, geometry.height);
}

///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

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
    WebKitWebView *webView;
    GtkWidget *window;
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
        
    printf("Browser::~Browser !!!\n");
    // Destroy (GtkWidget)m_private->webView
    // Destroy (GtkWidget)m_private->window
}

void Browser::initialize(int width, int height)
{
    printf("Browser::initialize\n");
    
    // Perform one-time initialization if necessary (NOT thread-safe)
    if (!g_WebKitData.initialized) {
        g_WebKitData.initialized = true;

        gtk_init(NULL, NULL);   // THREADCHECK
        
        // Make sure the default im module imquartz is not used on OS X, as this will cause a crash with GtkOffscreenWindow
        g_object_set(gtk_settings_get_default(), "gtk-im-module", "gtk-im-context-simple", NULL);

        WebKitWebContext *globalContext = webkit_web_context_get_default();

        // Store thread id for thread safety checks
        pthread_threadid_np(NULL, &g_WebKitData.tid);

        // Settings
        g_WebKitData.settings = webkit_settings_new();
        webkit_settings_set_enable_developer_extras(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_webgl(g_WebKitData.settings, FALSE);
        webkit_settings_set_enable_media_stream(g_WebKitData.settings, FALSE);

        // Faux single process mode for global context
        webkit_web_context_set_process_model(globalContext, WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS);
    }
    printf("Browser::initialize 1\n");

    // Create offscreen window
    GtkWidget *window = gtk_offscreen_window_new();
    m_private->window = window;
    gtk_window_set_default_size(GTK_WINDOW(m_private->window), width, height);
    printf("Browser::initialize 2\n");

    // Create WebkitWebView with default global context (GtkWidget)
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    m_private->webView = webView;
    webkit_web_view_set_settings(webView, g_WebKitData.settings);
    g_signal_connect(webView, "ready-to-show", G_CALLBACK(webViewReadyToShow), this);
    printf("Browser::initialize 3\n");
    
    // Set transparent background color -- ACHTUNG doesnt work
    const GdkRGBA backgroundColor = { 0, 0, 0, 0 };
    webkit_web_view_set_background_color(webView, &backgroundColor);

    // Set paint callback
    using namespace std::placeholders;
    webkit_web_view_set_paint_callback(webView, [=](uint8_t *data, float scaling, const GdkPoint &dstPoint, const GdkRectangle &srcSize, const GdkRectangle &srcRect)->void {
        // Callback
        if(m_private->paintCallback) {
            const Point dstPoint_ = { (unsigned int)dstPoint.x, (unsigned int)dstPoint.y };
            const Rect srcSize_ = { (unsigned int)srcSize.x, (unsigned int)srcSize.y, (unsigned int)srcSize.width, (unsigned int)srcSize.height };
            const Rect srcRect_ = { (unsigned int)srcRect.x, (unsigned int)srcRect.y, (unsigned int)srcRect.width, (unsigned int)srcRect.height };
            m_private->paintCallback(data, dstPoint_ ,srcSize_, srcRect_);
        }
    });
    printf("Browser::initialize 4\n");
    
    // ACHTUNG
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webView));

    // Show (and implicitly realize) entire widget tree
    // NOTE: This call may crash due to a bug in gtk+3 on OS X, see https://bugzilla.gnome.org/show_bug.cgi?id=667721
    gtk_widget_show_all(window);

    m_private->initialized = true;
    printf("Browser::initialize done\n");
}

void Browser::tick()
{
    // Thread safety check (caller thread must be identical for all GTK calls)
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    assert(tid == g_WebKitData.tid);
    
    // ACHTUNG check if necessary
    // THREADCHECK
    gtk_main_iteration_do(FALSE);
}

bool Browser::isInitialized() const
{
    return m_private->initialized;
}

void Browser::setSize(int width, int height)
{
    assert(isInitialized());
    
    printf("Browser::setSize\n");
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

void Browser::loadURL(const std::string &url)
{
    webkit_web_view_load_uri(m_private->webView, url.c_str());
}

void Browser::setPaintCallback(PaintCallback fn)
{
    m_private->paintCallback = fn;
}

}
