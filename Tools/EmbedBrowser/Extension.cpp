#include "Private.h"
#include "Extension.h"

#include <webkit2/webkit-web-extension.h>
#include <JavaScriptCore/JavaScript.h>

#define UNUSED_PARAM(variable) (void)variable

// Global hack, assume there is always a browser
WebKitEmbed::BrowserPrivate* g_browser = nullptr;

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Denise specific code
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<TabPrivate> getTabFromObject(JSObjectRef object) {
    // Retrieve TabPrivate instance in a safe way (handles already deleted tabs gracefully)
    assert(g_browser != nullptr);
    const Browser::Tab::Index tabIndex = (const Browser::Tab::Index)JSObjectGetPrivate(object);
    if (g_browser->tabs.find(tabIndex) != g_browser->tabs.end()) {
        std::shared_ptr<Browser::Tab> tab = g_browser->tabs.find(tabIndex)->second;
        if (tab) {
            return tab->m_private;
        }
    }
    return nullptr;
}

enum DeniseError {
    ERROR_NONE = 0,
    ERROR_INVALID_PARAMETERS
};

/*
    interface ErrorObject {
      ErrorCode code,
      string message
    }
*/
// THREAD-BROWSER
JSObjectRef deniseMakeErrorObject(JSContextRef context, const DeniseError error, const std::string message) {
    JSObjectRef obj = JSObjectMake(context, nullptr, nullptr);

    JSStringRef strCode = JSStringCreateWithUTF8CString("code");
    JSObjectSetProperty(context, obj, strCode, JSValueMakeNumber(context, (int)error), kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strCode);

    JSStringRef strMessage = JSStringCreateWithUTF8CString("message");
    JSStringRef strErrorMessage = JSStringCreateWithUTF8CString(message.c_str());
    JSObjectSetProperty(context, obj, strMessage, JSValueMakeString(context, strErrorMessage), kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strErrorMessage);
    JSStringRelease(strMessage);
    
    return obj;
}

// THREAD-BROWSER
JSObjectRef deniseMakeErrorObject(JSContextRef context, const DeniseError error, const JSValueRef message) {
    JSObjectRef obj = JSObjectMake(context, nullptr, nullptr);

    JSStringRef strCode = JSStringCreateWithUTF8CString("code");
    JSObjectSetProperty(context, obj, strCode, JSValueMakeNumber(context, (int)error), kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strCode);

    JSStringRef strMessage = JSStringCreateWithUTF8CString("message");
    JSObjectSetProperty(context, obj, strMessage, message, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strMessage);
    
    return obj;
}

// THREAD-BROWSER
std::string JSStringToStdString(JSStringRef jsString) {
    size_t nMaxSize = JSStringGetMaximumUTF8CStringSize(jsString);
    char* buf = new char[nMaxSize];
    size_t nSize = JSStringGetUTF8CString(jsString, buf, nMaxSize);
    std::string str = std::string (buf, nSize - 1);
    delete [] buf;
    return str;
}

// THREAD-BROWSER
JSValueRef deniseJSLoadProduct(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);
    
    std::shared_ptr<TabPrivate> tab = getTabFromObject(object);

    /* DeniseWrapper.loadProduct(payload: ProductPayload, callback: function(err: ErrorCode)) */
    if (argumentCount == 2) {
        // Get parameters
        JSObjectRef objPayload = JSValueToObject(context, arguments[0], exception);
        if (!*exception) {
            Denise::Wrapper::ProductType productType;
            std::string productId;
            std::string productLoadData;
            
            bool valid = true;
            // productPayload.id
            if (valid) {
                JSStringRef jsstrKey = JSStringCreateWithUTF8CString("id");
                JSValueRef jsValueRef = JSObjectGetProperty(context, objPayload, jsstrKey, exception);
                if (!*exception && JSValueIsString(context, jsValueRef)) {
                    JSStringRef jsstrValue = JSValueToStringCopy(context, jsValueRef, exception);
                    if (!*exception && jsstrValue) {
                        productId = JSStringToStdString(jsstrValue);
                    }
                    else {
                        valid = false;
                    }
                    JSStringRelease(jsstrValue);
                }
                else {
                    valid = false;
                }
                if (!valid) {
                    JSStringRef message = JSStringCreateWithUTF8CString("TypeError: productPayload.id not a string");
                    *exception = JSValueMakeString(context, message);
                    JSStringRelease(message);
                }
                JSStringRelease(jsstrKey);
            }
            // productPayload.type
            if (valid) {
                JSStringRef jsstrKey = JSStringCreateWithUTF8CString("type");
                JSValueRef jsValueRef = JSObjectGetProperty(context, objPayload, jsstrKey, exception);
                if (!*exception && JSValueIsNumber(context, jsValueRef)) {
                    int value = JSValueToNumber(context, jsValueRef, exception);
                    if (!*exception) {
                        productType = (Denise::Wrapper::ProductType)value;
                    }
                    else {
                        valid = false;
                    }
                }
                else {
                    valid = false;
                }
                if (!valid) {
                    JSStringRef message = JSStringCreateWithUTF8CString("TypeError: productPayload.type not a number");
                    *exception = JSValueMakeString(context, message);
                    JSStringRelease(message);
                }
                JSStringRelease(jsstrKey);
            }
            // productPayload.loadData
            if (valid) {
                JSStringRef jsstrKey = JSStringCreateWithUTF8CString("loadData");
                JSValueRef jsValueRef = JSObjectGetProperty(context, objPayload, jsstrKey, exception);
                if (!*exception && JSValueIsString(context, jsValueRef)) {
                    JSStringRef jsstrValue = JSValueToStringCopy(context, jsValueRef, exception);
                    if (!*exception && jsstrValue) {
                        productLoadData = JSStringToStdString(jsstrValue);
                    }
                    else {
                        valid = false;
                    }
                    JSStringRelease(jsstrValue);
                }
                else {
                    valid = false;
                }
                if (!valid) {
                    JSStringRef message = JSStringCreateWithUTF8CString("TypeError: productPayload.loadData not a string");
                    *exception = JSValueMakeString(context, message);
                    JSStringRelease(message);
                }
                JSStringRelease(jsstrKey);
            }
            // Callback if parameters were valid
            if (valid && tab != nullptr && tab->callbackDeniseLoadProduct) {
                tab->callbackDeniseLoadProduct(productType, productId, productLoadData);
            }
        }
    }
    else {
        JSStringRef message = JSStringCreateWithUTF8CString("TypeError: function requires 2 arguments");
        *exception = JSValueMakeString(context, message);
        JSStringRelease(message);
    }

    return JSValueMakeUndefined(context);
}

// THREAD-BROWSER
JSValueRef deniseJSSetOverlay(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    std::shared_ptr<TabPrivate> tab = getTabFromObject(object);

    // Error related parameters
    JSObjectRef objCallback;
    
    /* DeniseWrapper.setOverlay({ visible: boolean }, callback: function(err: ErrorCode)) */
    if (argumentCount == 2) {
        // Get callback
        {
            JSObjectRef _callback = JSValueToObject(context, arguments[1], exception);
            if (!*exception) {
                if(!JSObjectIsFunction(context, _callback)) {
                    JSStringRef strError = JSStringCreateWithUTF8CString("TypeError: callback is not a function");
                    *exception = JSValueMakeString(context, strError);
                    JSStringRelease(strError);
                }
            }
            if (!*exception) {
                // Assume valid callback function object, so store into objCallback
                objCallback = _callback;
            }
        }
        if (!*exception) {
            // Get parameters
            JSObjectRef objParams = JSValueToObject(context, arguments[0], exception);
            if (!*exception) {
                // params.visible
                {
                    JSStringRef jsstrVisible = JSStringCreateWithUTF8CString("visible");
                    const bool visible = JSValueToBoolean(context, JSObjectGetProperty(context, objParams, jsstrVisible, exception));
                    if(!*exception && tab != nullptr && tab->callbackDeniseSetOverlay) {
                        tab->callbackDeniseSetOverlay(visible);
                    }
                    JSStringRelease(jsstrVisible);
                }
            }
        }
    }
    else {
        JSStringRef strError = JSStringCreateWithUTF8CString("TypeError: function requires 2 arguments");
        *exception = JSValueMakeString(context, strError);
        JSStringRelease(strError);
    }

    // Fall-through code path for all cases
    
    // Check for exception, in which case we call the callback, if it is defined at this point
    if (*exception && objCallback) {
        // Assume ERROR_INVALID_PARAMETERS as code for any errors occurring above,
        // use the string contained inside exception as message.
        
        /* function(err) */
        JSValueRef args[] = { deniseMakeErrorObject(context, DeniseError::ERROR_INVALID_PARAMETERS, *exception) };
        JSObjectCallAsFunction(context, objCallback, nullptr, 1, args, nullptr);
    }
    
    return JSValueMakeUndefined(context);
}

// THREAD-BROWSER
JSValueRef deniseJSSetHeader(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    std::shared_ptr<TabPrivate> tab = getTabFromObject(object);

    // Error related parameters
    JSObjectRef objCallback;
    
    /* DeniseWrapper.setHeader({ visible: boolean }, callback: function(err: ErrorCode)) */
    if (argumentCount == 2) {
        // Get callback
        {
            JSObjectRef _callback = JSValueToObject(context, arguments[1], exception);
            if (!*exception) {
                if(!JSObjectIsFunction(context, _callback)) {
                    JSStringRef strError = JSStringCreateWithUTF8CString("TypeError: callback is not a function");
                    *exception = JSValueMakeString(context, strError);
                    JSStringRelease(strError);
                }
            }
            if (!*exception) {
                // Assume valid callback function object, so store into objCallback
                objCallback = _callback;
            }
        }
        if (!*exception) {
            // Get parameters
            JSObjectRef objParams = JSValueToObject(context, arguments[0], exception);
            if (!*exception) {
                // params.visible
                {
                    JSStringRef jsstrVisible = JSStringCreateWithUTF8CString("visible");
                    const bool visible = JSValueToBoolean(context, JSObjectGetProperty(context, objParams, jsstrVisible, exception));
                    if(!*exception && tab != nullptr && tab->callbackDeniseSetHeader) {
                        tab->callbackDeniseSetHeader(visible);
                    }
                    JSStringRelease(jsstrVisible);
                }
            }
        }
    }
    else {
        JSStringRef strError = JSStringCreateWithUTF8CString("TypeError: function requires 2 arguments");
        *exception = JSValueMakeString(context, strError);
        JSStringRelease(strError);
    }

    // Fall-through code path for all cases
    
    // Check for exception, in which case we call the callback, if it is defined at this point
    if (*exception && objCallback) {
        // Assume ERROR_INVALID_PARAMETERS as code for any errors occurring above,
        // use the string contained inside exception as message.
        
        /* function(err) */
        JSValueRef args[] = { deniseMakeErrorObject(context, DeniseError::ERROR_INVALID_PARAMETERS, *exception) };
        JSObjectCallAsFunction(context, objCallback, nullptr, 1, args, nullptr);
    }
    
    return JSValueMakeUndefined(context);
}

// THREAD-BROWSER
JSValueRef deniseAppJSNotificationSet(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    std::shared_ptr<TabPrivate> tab = getTabFromObject(object);

    /* DeniseApp.notificationSet() */
    tab->callbackDeniseAppNotificationSet();
    
    return JSValueMakeUndefined(context);
}

// THREAD-BROWSER
JSValueRef deniseAppJSNotificationReset(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    std::shared_ptr<TabPrivate> tab = getTabFromObject(object);

    /* DeniseApp.notificationReset() */
    tab->callbackDeniseAppNotificationReset();
    
    return JSValueMakeUndefined(context);
}

// THREAD-BROWSER
void deniseBindJS(JSGlobalContextRef context, const Browser::Tab::Index tabIndex) {
    JSObjectRef objGlobal = JSContextGetGlobalObject(context);
    if (objGlobal) {
        // DeniseWrapper
        {
            // Create object as default JS Object
            // (tabIndex encoded into pointer, which should be fine)
            JSObjectRef obj = JSObjectMake(context, nullptr, (void*)tabIndex);
            
            // DeniseWrapper.loadProduct
            {
                JSStringRef str = JSStringCreateWithUTF8CString("loadProduct");
                JSObjectSetProperty(context, obj, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSLoadProduct), kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }
            // DeniseWrapper.setOverlay
            {
                JSStringRef str = JSStringCreateWithUTF8CString("setOverlay");
                JSObjectSetProperty(context, obj, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSSetOverlay), kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }
            // DeniseWrapper.setHeader
            {
                JSStringRef str = JSStringCreateWithUTF8CString("setHeader");
                JSObjectSetProperty(context, obj, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSSetHeader), kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }

            // Bind to new object in global object
            {
                JSStringRef str = JSStringCreateWithUTF8CString("DeniseWrapper");
                JSObjectSetProperty(context, objGlobal, str, obj, kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }
        }
        // DeniseApp
        {
            // Create object as default JS Object
            // (tabIndex encoded into pointer, which should be fine)
            JSObjectRef obj = JSObjectMake(context, nullptr, (void*)tabIndex);
            
            // DeniseApp.notificationSet()
            {
                JSStringRef str = JSStringCreateWithUTF8CString("notificationSet");
                JSObjectSetProperty(context, obj, str, JSObjectMakeFunctionWithCallback(context, str, deniseAppJSNotificationSet), kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }
            // DeniseApp.notificationReset()
            {
                JSStringRef str = JSStringCreateWithUTF8CString("notificationReset");
                JSObjectSetProperty(context, obj, str, JSObjectMakeFunctionWithCallback(context, str, deniseAppJSNotificationReset), kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }

            // Bind to new object in global object
            {
                JSStringRef str = JSStringCreateWithUTF8CString("DeniseApp");
                JSObjectSetProperty(context, objGlobal, str, obj, kJSPropertyAttributeNone, nullptr);
                JSStringRelease(str);
            }
        }
    }
    else {
        // Unable to get JS global context
        assert(false);
    }
}

// THREAD-BROWSER
void webViewWindowObjectCleared(WebKitScriptWorld *world, WebKitWebPage* page, WebKitFrame* frame, const Browser::Tab::Index tabIndex) {
    deniseBindJS(webkit_frame_get_javascript_context_for_script_world(frame, world), tabIndex);
}

// THREAD-UI
extern void registerWebExtension(BrowserPrivate* browser, const Browser::Tab::Index tabIndex) {
    // Global hack
    assert(g_browser == nullptr || g_browser == browser);
    g_browser = browser;
    
    webkit_script_world_set_create_callback([tabIndex](WebKitScriptWorld* world)->void {
        // THREAD-BROWSER
        // (tabIndex encoded into pointer, which should be fine)
        g_signal_connect(world, "window-object-cleared", G_CALLBACK(webViewWindowObjectCleared), (void*)tabIndex);
    });
}

};
