#pragma once

#include <memory>
#include <string>

namespace WebKitEmbed
{
  class Browser
  {
  public:
    enum ModifierKeys {
      MODIFIER_NONE = 0,
      MODIFIER_SHIFT = 1,
      MODIFIER_CTRL = 2,
      MODIFIER_ALT = 4,
      MODIFIER_COMMAND = 8,
      MODIFIER_LEFT_MOUSE = 16,
      MODIFIER_RIGHT_MOUSE = 32,
      MODIFIER_MIDDLE_MOUSE = 64
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
    
    void loadURL(const std::string &url);

    typedef std::function<void(uint8_t *, const Point&, const Rect&, const Rect&)> PaintCallback;
    void setPaintCallback(PaintCallback fn);
    
  private:
    std::unique_ptr<class BrowserPrivate> m_private;
  };
}
