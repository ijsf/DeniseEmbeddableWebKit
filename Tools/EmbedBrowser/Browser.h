#pragma once

#include <memory>
#include <string>
#include <functional>

/// DENISE BEGIN
namespace Denise
{
    namespace Internal
    {
        /**
        * Wrapper
        *
        * Interface class for calls to the Wrapper.
        */
        class Wrapper {
        public:
            #include "WrapperConstants.h"
   
            struct ProductPayload {
                std::string id;
                PluginType type;
                std::string loadData;
            };
       
            virtual void loadProduct(const ProductPayload payload) = 0;
            virtual void setOverlay(const bool visible) = 0;
            virtual void setHeader(const bool visible) = 0;
   
        protected:
            virtual ~Wrapper() {};
        };

        /**
        * Store
        *
        * Interface class for calls to the Store.
        */
        class Store {
        public:
            virtual bool setStore(const bool visible) = 0;
   
        protected:
            virtual ~Store() {};
        };
    }
}
/// DENISE END

namespace WebKitEmbed
{
    class Browser
    {
    public:
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

        Browser();
        virtual ~Browser();

        void initialize(int width, int height);
        bool isInitialized() const;

        void tick();

        void setSize(int width, int height);

        void mouseMove(int x, int y, ModifierKeys modifier);
        void mouseDown(int x, int y, ModifierKeys modifier);
        void mouseUp(int x, int y, ModifierKeys modifier);

        void keyPress(const unsigned int key, const ModifierKeys modifierKeys);

        void loadURL(const std::string &url);

        typedef std::function<void(uint8_t *, const Point&, const Rect&, const Rect&)> PaintCallback;
        void setPaintCallback(PaintCallback fn);

        typedef std::function<void(const bool)> IsLoadingCallback;
        void setIsLoadingCallback(IsLoadingCallback fn);

        // bool loadFailed(const LoadEvent loadEvent, const std::string URI, const Error error)
        // Return true to stop other handlers from being invoked, false to propagate the event further
        typedef std::function<bool(const LoadEvent loadEvent, const std::string, const Error)> LoadFailedCallback;
        void setLoadFailedCallback(LoadFailedCallback fn);

        /// DENISE BEGIN
        void deniseSetWrapperInterface(Denise::Internal::Wrapper* interface);
        /// DENISE END
    
    private:
        std::unique_ptr<class BrowserPrivate> m_private;
    };
}
