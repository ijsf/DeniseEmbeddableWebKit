#pragma once

#include <memory>
#include <string>
#include <functional>

namespace Denise
{
    namespace Wrapper
    {
        // Constants
        #include "WrapperConstants.h"
    }
}

namespace WebKitEmbed
{
    /**
     * Browser
     */
    class Browser
    {
    public:
        // Constants
        #include "BrowserConstants.h"

        // Mirror of GError
        struct Error {
            int code;
            std::string message;
        };
        
        struct Point {
            unsigned int x;
            unsigned int y;
        };

        struct Rect {
            unsigned int x;
            unsigned int y;
            unsigned int width;
            unsigned int height;
        };

        /**
        * Tab
        */
        class Tab {
        public:
            typedef size_t Index;
            
        protected:
            friend class Browser;
            Tab(const Index tabIndex, class BrowserPrivate* parent);
   
        public:
            ~Tab();

            void initialize(const unsigned int width, const unsigned int height);
            bool isInitialized() const;

            void setSize(const unsigned int width, const unsigned int height);

            void mouseMove(int x, int y, ModifierKeys modifier);
            void mouseDown(int x, int y, ModifierKeys modifier);
            void mouseUp(int x, int y, ModifierKeys modifier);

            void keyPress(const unsigned int key, const ModifierKeys modifierKeys);

            void loadURL(const std::string& url);

            /*** Browser callbacks ***/
            typedef std::function<void(uint8_t *, const Point&, const Rect&, const Rect&)> CallbackPaint;
            typedef std::function<void(const bool)> CallbackIsLoading;
            // bool loadFailed(const LoadEvent loadEvent, const std::string URI, const Error error)
            // Return true to stop other handlers from being invoked, false to propagate the event further
            typedef std::function<bool(const LoadEvent loadEvent, const std::string, const Error)> CallbackLoadFailed;

            void setCallbackPaint(CallbackPaint fn);
            void setCallbackIsLoading(CallbackIsLoading fn);
            void setCallbackLoadFailed(CallbackLoadFailed fn);

            /*** Denise callbacks ***/
            typedef std::function<void(const Denise::Wrapper::ProductType, const std::string, const std::string)> CallbackDeniseLoadProduct;
            typedef std::function<void(const bool)> CallbackDeniseSetOverlay;
            typedef std::function<void(const bool)> CallbackDeniseSetHeader;

            void setCallbackDeniseLoadProduct(CallbackDeniseLoadProduct fn);
            void setCallbackDeniseSetOverlay(CallbackDeniseSetOverlay fn);
            void setCallbackDeniseSetHeader(CallbackDeniseSetHeader fn);

            // pImpl
            std::shared_ptr<class TabPrivate> m_private;
        };

    public:
        Browser();
        virtual ~Browser();

        void initialize();
        bool isInitialized() const;
        std::shared_ptr<Tab> createTab();

        // pImpl
        std::unique_ptr<class BrowserPrivate> m_private;
    };
}
