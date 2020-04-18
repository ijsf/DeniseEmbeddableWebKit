# Embeddable WebKit

This is a now unmaintained repository representing a whole lot of work creating an embeddable (as library) WebKit browser based on WebKitGTK.

Since the prototype this code was originally meant for is no longer necessary, this repository is now available as open source in case anyone finds a use for it. A lot of work has been done patching WebKitGTK as can be seen in the commits.

---

This repository aims to provide a port of the WebKitGtk project that can be used to embed a lightweight version of WebKit into other applications with the following characteristics:

* Multiplatform, by harnessing gtk+'s characteristics.
* Does not spawn any processes, keeping all functionality within the process that embeds WebKit's WebView.
* Does not create any windows, provides callbacks for offscreen rendering.
* Excludes as much functionality not essential for embedded browsers.

Currently, only OS X is supported.

# OS X

Building WebKitGtk on OS X requires a number of prerequisities and some bug patches to dependencies that may have been installed globally or other oddities.

## Prerequisities to run

### Pitfalls

The following errors may occur when running an application using the embedded browser on a vanilla macOS system:

```
Fontconfig error: Cannot load default config file

(<unknown>:194): GdkPixbuf-WARNING **: 15:54:37.180: Cannot open pixbuf loader module file '/usr/local/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache': No such file or directory

This likely means that your installation is broken.
Try running the command
gdk-pixbuf-query-loaders > /usr/local/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
to make things work again for the time being.
```

To fix this, unfortunately a number of things need to be installed on the system, see below.

### Font-config

The bundled GTK relies on font-config being present on the system.

```
brew install font-config
fc-cache
```

### GTK+3

The bundled GTK relies on some GSettings files / schemes being present in the system.

```
brew install gtk+3
```

### Self-signed certificates

These do not work. The daemon will crash (and restart) with a `g_object_unref` error.

## Prerequisities to compile

### XCode

The XCode command line utilities _must_ be installed. Not doing this will lead to very obscure errors due to the `/usr/local/include` header path and `/usr/local/lib` library path not being found.

Make sure the following has been executed on the build system:

    xcode-select --install

### icu

Installing `icu4c` using Homebrew is currently not possible, so this has to be done manually. 

Download the latest `icu4c` Linux source archive from http://site.icu-project.org/download/61#TOC-ICU4C-Download and extract.

	cd source
	./configure
	make install

### libsoup

Install:

    brew install libsoup

### gettext

Install `gettext` using Homebrew:

	brew install gettext

The `gettext` package should now be available at `/usr/local/opt/gettext/lib`.

In case you run into compilation problems with `-lintl` in webgtk, symlink `libintl.a` to `/usr/local/lib`:

	ln -s /usr/local/opt/gettext/lib/libintl.a /usr/local/lib

### glib-networking

Unfortunately, glib-networking through Homebrew does not normally contain static modules and relies on a dynamic loading system that we do not want to use.
We thus have to patch the Homebrew formula:

```
	HOMEBREW_EDITOR=nano brew edit glib-networking
```

Add the following configure arguments for static linking:

```
                      "-Dstatic_modules=true",
```

Save and close, and make sure to rebuild the package:

```
	brew reinstall --build-from-source glib-networking
```

The static libraries are expected to be available at `/usr/local/lib/gio/modules`.

### gtk+3

Unfortunately, gtk+3 contains critical bugs related to offscreen windows that need to be patched manually.
Since gtk+3 is installed globally through Homebrew, we have to patch the Homebrew formula.

	HOMEBREW_EDITOR=nano brew edit gtk+3

The patches below assume the following version:

```
    url "https://download.gnome.org/sources/gtk+/3.24/gtk+-3.24.2.tar.xz"
```

Add the following bugfix patch (tracked at `https://gitlab.gnome.org/GNOME/gtk/issues/784`):

```
    patch :p0, <<~EOS
      --- gdk/quartz/gdkwindow-quartz.c.old	2016-12-30 17:55:56.000000000 +0300
      +++ gdk/quartz/gdkwindow-quartz.c	2017-03-08 18:05:13.000000000 +0300
      @@ -1079,6 +1079,15 @@
         GDK_QUARTZ_RELEASE_POOL;
       }
 
      +static GdkWindow *
      +_gdk_quartz_window_get_effective_impl_transient_for(GdkWindow *window)
      +{
      +   window = (GDK_WINDOW_IMPL_QUARTZ (window->impl))->transient_for;
      +   while (window && !GDK_IS_WINDOW_IMPL_QUARTZ (window->impl))
      +     window = window->parent;
      +   return window;
      +}
      +
       /* Temporarily unsets the parent window, if the window is a
        * transient. 
        */
      @@ -1086,6 +1095,7 @@
       _gdk_quartz_window_detach_from_parent (GdkWindow *window)
       {
         GdkWindowImplQuartz *impl;
      +  GdkWindow *transient_for;
 
         g_return_if_fail (GDK_IS_WINDOW (window));
 
      @@ -1093,11 +1103,12 @@
   
         g_return_if_fail (impl->toplevel != NULL);
 
      -  if (impl->transient_for && !GDK_WINDOW_DESTROYED (impl->transient_for))
      +  transient_for = _gdk_quartz_window_get_effective_impl_transient_for (window);
      +  if (transient_for && !GDK_WINDOW_DESTROYED (transient_for))
           {
             GdkWindowImplQuartz *parent_impl;
 
      -      parent_impl = GDK_WINDOW_IMPL_QUARTZ (impl->transient_for->impl);
      +      parent_impl = GDK_WINDOW_IMPL_QUARTZ (transient_for->impl);
             [parent_impl->toplevel removeChildWindow:impl->toplevel];
             clear_toplevel_order ();
           }
      @@ -1108,6 +1119,7 @@
       _gdk_quartz_window_attach_to_parent (GdkWindow *window)
       {
         GdkWindowImplQuartz *impl;
      +  GdkWindow *transient_for;
 
         g_return_if_fail (GDK_IS_WINDOW (window));
 
      @@ -1115,11 +1127,12 @@
   
         g_return_if_fail (impl->toplevel != NULL);
 
      -  if (impl->transient_for && !GDK_WINDOW_DESTROYED (impl->transient_for))
      +  transient_for = _gdk_quartz_window_get_effective_impl_transient_for (window);
      +  if (transient_for && !GDK_WINDOW_DESTROYED (transient_for))
           {
             GdkWindowImplQuartz *parent_impl;
 
      -      parent_impl = GDK_WINDOW_IMPL_QUARTZ (impl->transient_for->impl);
      +      parent_impl = GDK_WINDOW_IMPL_QUARTZ (transient_for->impl);
             [parent_impl->toplevel addChildWindow:impl->toplevel ordered:NSWindowAbove];
             clear_toplevel_order ();
           }
    EOS
```

Add the following bugfix patch (tracked at `https://gitlab.gnome.org/GNOME/gtk/issues/986`):

```
    patch :p1, <<~EOS
      diff -ruN gtk+-3.22.26.old/gdk/gdkwindow.c gtk+-3.22.26.new/gdk/gdkwindow.c
      --- gtk+-3.22.26.old/gdk/gdkwindow.c	2017-10-26 12:23:40.000000000 +0200
      +++ gtk+-3.22.26.new/gdk/gdkwindow.c	2017-11-20 01:46:52.000000000 +0100
      @@ -8531,6 +8531,26 @@

         pointer_info = _gdk_display_get_pointer_info (display, device);

      +  //HACK: workaround for a bug in the Quartz driver which causes i.e. menus to hang
      +#if defined(__APPLE__)
      +  /* It seems the Quartz driver is sometimes not handling crossing events
      +     correctly (i.e. after menu popups, tooltips appeared), which causes
      +     toplevel_under_pointer to point to an old toplevel window or even NULL,
      +     and finally the respective relevent child widget under the mouse pointer
      +     could not be resolved due to this, which caused i.e. menus to ignore all
      +     mouse events. The following is a harsh way to ensure toplevel_under_pointer
      +     is always pointing to the window under the current pointer location by
      +     updating this whenever pointer info is accessed.
      +  */
      +  if (pointer_info) {
      +    if (pointer_info->toplevel_under_pointer)
      +      g_object_unref (pointer_info->toplevel_under_pointer);
      +    GdkWindow* w =
      +      _gdk_device_window_at_position (device, NULL, NULL, NULL, TRUE);
      +    pointer_info->toplevel_under_pointer = w ? g_object_ref (w) : NULL;
      +  }
      +#endif
      +
         if (event_window == pointer_info->toplevel_under_pointer)
           pointer_window =
             _gdk_window_find_descendant_at (event_window,
    EOS
```

Add the following hack patch:

```
    patch :p1, <<~EOS
      --- gtk+-3.22.30-original/gdk/quartz/gdkeventloop-quartz.c	2016-10-22 06:14:29.000000000 +0200
      +++ gtk+-3.22.30/gdk/quartz/gdkeventloop-quartz.c	2018-06-22 15:52:21.000000000 +0200
      @@ -641,8 +641,8 @@
          */
         if (current_loop_level == 0 && g_main_depth() == 0 && getting_events == 0)
           {
      -      if (autorelease_pool)
      -        [autorelease_pool drain];
      +//      if (autorelease_pool)
      +//        [autorelease_pool drain];
 
             autorelease_pool = [[NSAutoreleasePool alloc] init];
           }
    EOS
```

Add the following patch to disable the Dock icon:

```
    patch :p0, <<~EOS
      --- gdk/quartz/gdkdisplay-quartz.c	2019-01-04 01:45:55.000000000 +0100
      +++ gdk/quartz/gdkdisplay-quartz.c	2019-01-04 01:46:11.000000000 +0100
      @@ -473,11 +473,4 @@
                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                         0, NULL, NULL, NULL,
                         G_TYPE_NONE, 0, NULL);
      -
      -  ProcessSerialNumber psn = { 0, kCurrentProcess };
      -
      -  /* Make the current process a foreground application, i.e. an app
      -   * with a user interface, in case we're not running from a .app bundle
      -   */
      -  TransformProcessType (&psn, kProcessTransformToForegroundApplication);
       }
    EOS
```

Add the following configure arguments for static linking and to disable unnecessary features:

    --disable-doc-cross-references --disable-cloudprint --disable-modules --disable-papi --disable-cups

Save and close, and make sure to rebuild the package:

	brew reinstall --build-from-source gtk+3

### gdk-pixbuf

This library compiles with dynamic loader modules by default. These should be built-in instead.

    HOMEBREW_EDITOR=nano brew edit gtk+3

Add this flag to the build configuration options:

    -Dbuiltin_loaders=all

### Fixups

The CMake files of the `WebCore` target are currently a bit messed up and do not properly include the `gio-unix` and `gettext` module include paths. As a workaround, copy the relevant header files into the `glib` include path:

```
cp /usr/local/include/gio-unix-2.0/gio/* /usr/local/include/glib-2.0/gio/
cp /usr/local/opt/gettext/include/* /usr/local/include/glib-2.0/
```

## Building

Make sure to add `gettext` to `PATH` before calling cmake:

	export PATH="/usr/local/opt/gettext/bin:$PATH"

Enter the WebKitGtk directory and create a `build` directory:

	mkdir build
	cd build

From this `build` directory, use `cmake` to configure:

	cmake -DPORT=GTK -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_ICONDATABASE=OFF -DENABLE_WEB_CRYPTO=OFF -DENABLE_WEBDRIVER=OFF -DENABLE_INTROSPECTION=OFF -DENABLE_X11_TARGET=OFF -DENABLE_QUARTZ_TARGET=ON -DENABLE_TOOLS=ON -DENABLE_MINIBROWSER=ON -DENABLE_NETSCAPE_PLUGIN_API=OFF -DENABLE_PLUGIN_PROCESS_GTK2=OFF -DENABLE_VIDEO=OFF -DENABLE_WEB_AUDIO=OFF -DENABLE_CREDENTIAL_STORAGE=OFF -DENABLE_GEOLOCATION=OFF -DENABLE_OPENGL=OFF -DENABLE_GRAPHICS_CONTEXT_3D=OFF -DUSE_LIBNOTIFY=OFF -DUSE_LIBHYPHEN=OFF -DCMAKE_SHARED_LINKER_FLAGS=-L/usr/local/opt/gettext/lib -DUSE_UPOWER=OFF -DUSE_WOFF2=OFF -DUSE_LIBSECRET=OFF -DENABLE_SPELLCHECK=OFF -DENABLE_SAMPLING_PROFILER=OFF -DCMAKE_CXX_FLAGS=-I/usr/local/include ..

Any custom configuration defines can optionally be added, e.g. `-DLOG_DISABLED=0`, `-DCMAKE_BUILD_TYPE=RelWithDebInfo`, `-DCMAKE_BUILD_TYPE=Debug`.

Initiate a build of WebKitGtk:

	make -j8

Note that this may take a while (e.g. several hours) and will require several GiB of storage space. The `-j8` specifies the number of parallel builds to perform and may be changed for better or worse.

### Fast building

The embeddable browsing API is contained in the `Tools/EmbedBrowser` code. In case you make any modifications to this source, invoking the above `make` (all) command will take a while and you may want to avoid this by using the fast build option, e.g.:

	cd build/Tools/EmbedBrowser
	make EmbedBrowser/fast

## Testing

### MiniBrowser

In order to run the included `MiniBrowser`:

	cd build
	./bin/MiniBrowser
