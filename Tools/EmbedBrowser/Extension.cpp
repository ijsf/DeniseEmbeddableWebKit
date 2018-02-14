#include <webkit2/webkit-web-extension.h>
#include <JavaScriptCore/JavaScript.h>

#include "Extension.h"

#define UNUSED_PARAM(variable) (void)variable

namespace WebKitEmbed
{

///////////////////////////////////////////////////////////////////////////////
// Denise specific code
///////////////////////////////////////////////////////////////////////////////

// We use static functions for now for convenience, but a better option would be to define all of this inside the Browser class
Browser::SetHeaderCallback deniseSetHeaderCallback;

/*
    interface ErrorObject {
      ErrorCode code,
      string message
    }
*/
JSObjectRef deniseMakeErrorObject(JSContextRef context, const Browser::DeniseError error, const std::string message) {
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

JSObjectRef deniseMakeErrorObject(JSContextRef context, const Browser::DeniseError error, const JSValueRef message) {
    JSObjectRef obj = JSObjectMake(context, nullptr, nullptr);

    JSStringRef strCode = JSStringCreateWithUTF8CString("code");
    JSObjectSetProperty(context, obj, strCode, JSValueMakeNumber(context, (int)error), kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strCode);

    JSStringRef strMessage = JSStringCreateWithUTF8CString("message");
    JSObjectSetProperty(context, obj, strMessage, message, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(strMessage);
    
    return obj;
}

JSValueRef deniseJSLoadProduct(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);
    
    /* DeniseWrapper.loadProduct(payload: ProductPayload, callback: function(err: ErrorCode)) */
    if (argumentCount == 2) {
        // STUB
    }
    else {
        JSStringRef message = JSStringCreateWithUTF8CString("TypeError: function requires 2 arguments");
        *exception = JSValueMakeString(context, message);
        JSStringRelease(message);
    }

    return JSValueMakeUndefined(context);
}

JSValueRef deniseJSSetHeader(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

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
                    JSStringRef strVisible = JSStringCreateWithUTF8CString("visible");
                    const bool visible = JSValueToBoolean(context, JSObjectGetProperty(context, objParams, strVisible, exception));
                    if(!*exception && deniseSetHeaderCallback) {
                        deniseSetHeaderCallback(visible);
                    }
                    JSStringRelease(strVisible);
                }
                // ...
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
        JSValueRef args[] = { deniseMakeErrorObject(context, Browser::DeniseError::ERROR_INVALID_PARAMETERS, *exception) };
        JSObjectCallAsFunction(context, objCallback, nullptr, 1, args, nullptr);
    }
    
    return JSValueMakeUndefined(context);
}

void deniseBindJS(JSGlobalContextRef context) {
    JSObjectRef objGlobal = JSContextGetGlobalObject(context);
    if (objGlobal) {
        // Create DeniseWrapper object as default JS Object
        JSObjectRef objDeniseWrapper = JSObjectMake(context, nullptr, nullptr);
        // Create function DeniseWrapper.loadProduct
        {
            JSStringRef strLoadProduct = JSStringCreateWithUTF8CString("loadProduct");
            JSObjectSetProperty(context, objDeniseWrapper, strLoadProduct, JSObjectMakeFunctionWithCallback(context, strLoadProduct, deniseJSLoadProduct), kJSPropertyAttributeNone, nullptr);
            JSStringRelease(strLoadProduct);
        }
        // Create function DeniseWrapper.setHeader
        {
            JSStringRef strSetHeader = JSStringCreateWithUTF8CString("setHeader");
            JSObjectSetProperty(context, objDeniseWrapper, strSetHeader, JSObjectMakeFunctionWithCallback(context, strSetHeader, deniseJSSetHeader), kJSPropertyAttributeNone, nullptr);
            JSStringRelease(strSetHeader);
        }
        // Bind to 'DeniseWrapper' in global object
        JSStringRef strDeniseWrapper = JSStringCreateWithUTF8CString("DeniseWrapper");
        JSObjectSetProperty(context, objGlobal, strDeniseWrapper, objDeniseWrapper, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(strDeniseWrapper);
    }
    else {
        // Unable to get JS global context
        assert(false);
    }
}

void webViewWindowObjectCleared(WebKitScriptWorld *world, WebKitWebPage *page, WebKitFrame *frame, Browser *browser) {
    deniseBindJS(webkit_frame_get_javascript_context_for_script_world(frame, world));
}

extern void registerWebExtension(Browser *browser) {
    g_signal_connect(webkit_script_world_get_default(), "window-object-cleared", G_CALLBACK(webViewWindowObjectCleared), browser);
}

};
