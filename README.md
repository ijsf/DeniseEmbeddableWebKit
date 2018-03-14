# WebKitGtk for Denise

This repository aims to provide a port of the WebKitGtk project that can be used to embed a lightweight version of WebKit into other applications with the following characteristics:

* Multiplatform, by harnessing gtk+'s characteristics.
* Does not spawn any processes, keeping all functionality within the process that embeds WebKit's WebView.
* Does not create any windows, provides callbacks for offscreen rendering.
* Excludes as much functionality not essential for embedded browsers.

Currently, only OS X is supported.

# OS X

Building WebKitGtk on OS X requires a number of prerequisities and some bug patches to dependencies that may have been installed globally or other oddities.

## Prerequisities

### icu

Installing `icu4c` using Homebrew is currently not possible, so this has to be done manually. 

Download the latest `icu4c` from http://site.icu-project.org/download/60#TOC-ICU4C-Download and extract.

	cd source
	./configure
	make install

### gettext

Install `gettext` using Homebrew:

	brew install gettext

The `gettext` package should now be available at `/usr/local/opt/gettext/lib`.

In case you run into compilation problems with `-lintl` in webgtk, symlink `libintl.a` to `/usr/local/lib`:

	ln -s /usr/local/opt/gettext/lib/libintl.a /usr/local/lib

### gtk+3

Unfortunately, gtk+3 contains critical bugs related to offscreen windows that need to be patched manually.
Since gtk+3 is installed globally through Homebrew, we have to patch the Homebrew formula.

	HOMEBREW_EDITOR=nano brew edit gtk+3

In the editor, add the following patch on the first level:

```
	patch :p1, <<~EOS
	  From 2c474afd063ae0ce3e0bb7fea452480042bb2b64 Mon Sep 17 00:00:00 2001
	  From: Philip Chimento <philip.chimento@gmail.com>
	  Date: Sun, 21 May 2017 20:40:40 -0700
	  Subject: [PATCH] quartz: Fix crash when realizing GtkOffscreenWindow

	  GtkOffscreenWindow doesn't have a NSView or NSWindow, so return NULL if
	  passed one of those.

	  https://bugzilla.gnome.org/show_bug.cgi?id=667721
	  ---
	   gtk/gtkdnd-quartz.c | 6 +++++-
	   1 file changed, 5 insertions(+), 1 deletion(-)

	  diff --git a/gtk/gtkdnd-quartz.c b/gtk/gtkdnd-quartz.c
	  index 6198986f6d..e047f06abf 100644
	  --- a/gtk/gtkdnd-quartz.c
	  +++ b/gtk/gtkdnd-quartz.c
	  @@ -36,6 +36,7 @@
	   #include "gtkimageprivate.h"
	   #include "gtkinvisible.h"
	   #include "gtkmain.h"
	  +#include "gtkoffscreenwindow.h"
	   #include "deprecated/gtkstock.h"
	   #include "gtkwindow.h"
	   #include "gtkintl.h"
	  @@ -356,7 +357,10 @@ get_toplevel_nswindow (GtkWidget *widget)
	   {
	     GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
	     GdkWindow *window = gtk_widget_get_window (toplevel);
	  -  
	  +
	  +  if (GTK_IS_OFFSCREEN_WINDOW (toplevel))
	  +    return NULL;
	  +
	     if (gtk_widget_is_toplevel (toplevel) && window)
	       return [gdk_quartz_window_get_nsview (window) window];
	     else
	  -- 
	EOS
```

Add the following configure arguments for static linking and to disable unnecessary features:

    --disable-doc-cross-references --disable-cloudprint --disable-modules --disable-papi --disable-cups

Save and close, and make sure to rebuild the `gtk+3` package:

	brew reinstall --build-from-source gtk+3

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
