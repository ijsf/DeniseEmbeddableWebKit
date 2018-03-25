/*
 * Copyright (C) 2013 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitScriptWorld.h"

#include "WebKitScriptWorldPrivate.h"
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/glib/WTFGType.h>

using namespace WebKit;
using namespace WebCore;

enum {
    WINDOW_OBJECT_CLEARED,

    LAST_SIGNAL
};

typedef HashMap<InjectedBundleScriptWorld*, WebKitScriptWorld*> ScriptWorldMap;

static ScriptWorldMap& scriptWorlds()
{
    static NeverDestroyed<ScriptWorldMap> map;
    return map;
}

struct _WebKitScriptWorldPrivate {
    ~_WebKitScriptWorldPrivate()
    {
        ASSERT(scriptWorlds().contains(scriptWorld.get()));
        scriptWorlds().remove(scriptWorld.get());
    }

    RefPtr<InjectedBundleScriptWorld> scriptWorld;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitScriptWorld, webkit_script_world, G_TYPE_OBJECT)

static void webkit_script_world_class_init(WebKitScriptWorldClass* klass)
{
    /**
     * WebKitScriptWorld::window-object-cleared:
     * @world: the #WebKitScriptWorld on which the signal is emitted
     * @page: a #WebKitWebPage
     * @frame: the #WebKitFrame  to which @world belongs
     *
     * Emitted when the JavaScript window object in a #WebKitScriptWorld has been
     * cleared. This is the preferred place to set custom properties on the window
     * object using the JavaScriptCore API. You can get the window object of @frame
     * from the JavaScript execution context of @world that is returned by
     * webkit_frame_get_javascript_context_for_script_world().
     *
     * Since: 2.2
     */
    signals[WINDOW_OBJECT_CLEARED] = g_signal_new(
        "window-object-cleared",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST,
        0, nullptr, nullptr,
        g_cclosure_marshal_generic,
        G_TYPE_NONE, 2,
        WEBKIT_TYPE_WEB_PAGE,
        WEBKIT_TYPE_FRAME);
}

/**
 * Mechanism to replace thread unsafe webkit_script_world_get_default and webkit_script_world_new.
 *
 * Using the original functions will cause a serious threading issue because webkit_script_world_get_default
 * would immediately create a JS context (including VM, run loop, etc.) in the calling thread,
 * while this should be left for the thread that is creating the webpage.
 *
 * Since webkit_script_world_get_default is normally called from the main thread (or any other unrelated thread),
 * this will cause potential deadlocks as the main thread will now be owning and taking care of a JS run loop
 * and its associated locking behaviour.
 *
 * As webkitScriptWorldGet (and subsequently webkitScriptWorldWindowObjectCleared) is called by the proper web page thread,
 * we just mimic the g_once behaviour of webkit_script_world_get_default and use a callback.
 *
 * NOTE: webkit_script_world_set_create_callback should only be called once.
 */

static std::function<void(WebKitScriptWorld *)> scriptWorldCreateCallback;
static std::atomic<int> scriptWorldCreated(0);

static WebKitScriptWorld* webkitScriptWorldCreate(Ref<InjectedBundleScriptWorld>&& scriptWorld)
{
    WebKitScriptWorld* world = WEBKIT_SCRIPT_WORLD(g_object_new(WEBKIT_TYPE_SCRIPT_WORLD, NULL));
    world->priv->scriptWorld = WTFMove(scriptWorld);

    ASSERT(!scriptWorlds().contains(world->priv->scriptWorld.get()));
    scriptWorlds().add(world->priv->scriptWorld.get(), world);

    return world;
}

void webkit_script_world_set_create_callback(std::function<void(WebKitScriptWorld *)> callback)
{
    scriptWorldCreateCallback = callback;
}

WebKitScriptWorld* webkitScriptWorldGet(InjectedBundleScriptWorld* scriptWorld)
{
    // Mimic'ed behaviour: if this is the first time this function is called, assume a scriptWorld was just created
    if (scriptWorldCreated.fetch_or(1) == 0) {
        // First time called, so create default script world
        webkitScriptWorldCreate(InjectedBundleScriptWorld::normalWorld());
        
        // Call the relevant callback
        scriptWorldCreateCallback(scriptWorlds().get(scriptWorld));
    }
    
    return scriptWorlds().get(scriptWorld);
}

InjectedBundleScriptWorld* webkitScriptWorldGetInjectedBundleScriptWorld(WebKitScriptWorld* world)
{
    return world->priv->scriptWorld.get();
}

void webkitScriptWorldWindowObjectCleared(WebKitScriptWorld* world, WebKitWebPage* page, WebKitFrame* frame)
{
    g_signal_emit(world, signals[WINDOW_OBJECT_CLEARED], 0, page, frame);
}
