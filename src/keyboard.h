#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NULL 0x00
#define KEY_ESC 0x1B
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x09
#define KEY_ENTER '\n'
#define KEY_LCTRL 0x1D
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LALT 0x38
#define KEY_CAPSLOCK 0x3A
#define KEY_F1 0x80
#define KEY_F2 0x81
#define KEY_F3 0x82
#define KEY_F4 0x83
#define KEY_F5 0x84
#define KEY_F6 0x85
#define KEY_F7 0x86
#define KEY_F8 0x87
#define KEY_F9 0x88
#define KEY_F10 0x89
#define KEY_F11 0x8A
#define KEY_F12 0x8B
#define KEY_NUMBERLOCK 0x45
#define KEY_SCROLLLOCK 0x46

#define LEFT_SHIFT_RELEASED 0xAA
#define RIGHT_SHIFT_RELEASED 0xB6

#define KEYBOARD_RELEASE 0x80

#define KEY_IS_PRESSED(_s) (!((_s) & KEYBOARD_RELEASE))
#define KEY_IS_RELEASED(_s) ((_s) & KEYBOARD_RELEASE)

#define LEFT_ARROW 0x4B
#define RIGHT_ARROW 0x4D
#define UP_ARROW 0x48
#define DOWN_ARROW 0x50

void init_keyboard();

#endif
