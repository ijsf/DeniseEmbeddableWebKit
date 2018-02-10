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
    
    enum Key {
        KEY_SPACE,
        KEY_ESCAPE,
        KEY_RETURN,
        KEY_TAB,
        KEY_DELETE,
        KEY_BACKSPACE,
        KEY_INSERT,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_HOME,
        KEY_END,

        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,

        KEY_NUMPAD_0,
        KEY_NUMPAD_1,
        KEY_NUMPAD_2,
        KEY_NUMPAD_3,
        KEY_NUMPAD_4,
        KEY_NUMPAD_5,
        KEY_NUMPAD_6,
        KEY_NUMPAD_7,
        KEY_NUMPAD_8,
        KEY_NUMPAD_9,

        KEY_NUMPAD_ADD,
        KEY_NUMPAD_SUBTRACT,
        KEY_NUMPAD_MULTIPLY,
        KEY_NUMPAD_DIVIDE,
        KEY_NUMPAD_SEPARATOR,
        KEY_NUMPAD_DECIMAL,
        KEY_NUMPAD_EQUAL,
        KEY_NUMPAD_DELETE
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
    
  private:
    std::unique_ptr<class BrowserPrivate> m_private;
  };
}
