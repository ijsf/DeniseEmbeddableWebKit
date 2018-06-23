#include "Private.h"
#include "Extension.h"

#include <webkit2/webkit-web-extension.h>
#include <JavaScriptCore/JavaScript.h>

#define UNUSED_PARAM(variable) (void)variable

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Denise specific code
///////////////////////////////////////////////////////////////////////////////

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
    
    TabPrivate* tab = (TabPrivate*)JSObjectGetPrivate(object);
    assert(tab != nullptr);

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
            if (valid && tab->callbackDeniseLoadProduct) {
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

    TabPrivate* tab = (TabPrivate*)JSObjectGetPrivate(object);
    assert(tab != nullptr);

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
                    if(!*exception && tab->callbackDeniseSetOverlay) {
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

    TabPrivate* tab = (TabPrivate*)JSObjectGetPrivate(object);
    assert(tab != nullptr);

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
                    if(!*exception && tab->callbackDeniseSetHeader) {
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
void deniseBindJS(JSGlobalContextRef context, TabPrivate* tab) {
    JSObjectRef objGlobal = JSContextGetGlobalObject(context);
    if (objGlobal) {
        // Create DeniseWrapper object as default JS Object
        JSObjectRef objDeniseWrapper = JSObjectMake(context, nullptr, tab);
        // Create function DeniseWrapper.loadProduct
        {
            JSStringRef str = JSStringCreateWithUTF8CString("loadProduct");
            JSObjectSetProperty(context, objDeniseWrapper, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSLoadProduct), kJSPropertyAttributeNone, nullptr);
            JSStringRelease(str);
        }
        // Create function DeniseWrapper.setOverlay
        {
            JSStringRef str = JSStringCreateWithUTF8CString("setOverlay");
            JSObjectSetProperty(context, objDeniseWrapper, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSSetOverlay), kJSPropertyAttributeNone, nullptr);
            JSStringRelease(str);
        }
        // Create function DeniseWrapper.setHeader
        {
            JSStringRef str = JSStringCreateWithUTF8CString("setHeader");
            JSObjectSetProperty(context, objDeniseWrapper, str, JSObjectMakeFunctionWithCallback(context, str, deniseJSSetHeader), kJSPropertyAttributeNone, nullptr);
            JSStringRelease(str);
        }
        // Bind to 'DeniseWrapper' in global object
        {
            JSStringRef str = JSStringCreateWithUTF8CString("DeniseWrapper");
            JSObjectSetProperty(context, objGlobal, str, objDeniseWrapper, kJSPropertyAttributeNone, nullptr);
            JSStringRelease(str);
        }
    }
    else {
        // Unable to get JS global context
        assert(false);
    }
}

// THREAD-BROWSER
void webViewWindowObjectCleared(WebKitScriptWorld *world, WebKitWebPage* page, WebKitFrame* frame, TabPrivate* tab) {
    deniseBindJS(webkit_frame_get_javascript_context_for_script_world(frame, world), tab);
}

// THREAD-UI
extern void registerWebExtension(TabPrivate* tab) {
    webkit_script_world_set_create_callback([tab](WebKitScriptWorld* world)->void {
        // THREAD-BROWSER
        g_signal_connect(world, "window-object-cleared", G_CALLBACK(webViewWindowObjectCleared), tab);
    });
}

};
