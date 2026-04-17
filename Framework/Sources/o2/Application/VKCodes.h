#pragma once

#if defined PLATFORM_WINDOWS

#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B

#define VK_OEM_3          0xC0
#define VK_OEM_MINUS      0xBD
#define VK_OEM_PLUS       0xBB

#define VK_OEM_4          0xDB
#define VK_OEM_6          0xDD

#define VK_OEM_1          0xBA
#define VK_OEM_7          0xDE
#define VK_OEM_5          0xDC

#define VK_OEM_COMMA      0xBC
#define VK_OEM_PERIOD     0xBE
#define VK_OEM_2          0xBF

#define VK_ESCAPE         0x1B

#define VK_SCROLL         0x91
#define VK_PAUSE          0x13

#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_CAPITAL        0x14
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_RETURN         0x0D
#define VK_SPACE          0x20

#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E

#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28

#define VK_NUMLOCK        0x90
#define VK_CLEAR          0x0C

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69

#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_RETURN_NUMPAD  0x0D

#define VK_0 '0'
#define VK_1 '1'
#define VK_2 '2'
#define VK_3 '3'
#define VK_4 '4'
#define VK_5 '5'
#define VK_6 '6'
#define VK_7 '7'
#define VK_8 '8'
#define VK_9 '9'
#define VK_A 'A'
#define VK_B 'B'
#define VK_C 'C'
#define VK_D 'D'
#define VK_E 'E'
#define VK_F 'F'
#define VK_G 'G'
#define VK_H 'H'
#define VK_I 'I'
#define VK_J 'J'
#define VK_K 'K'
#define VK_L 'L'
#define VK_M 'M'
#define VK_N 'N'
#define VK_O 'O'
#define VK_P 'P'
#define VK_Q 'Q'
#define VK_R 'R'
#define VK_S 'S'
#define VK_T 'T'
#define VK_U 'U'
#define VK_V 'V'
#define VK_W 'W'
#define VK_X 'X'
#define VK_Y 'Y'
#define VK_Z 'Z'

#elif defined PLATFORM_LINUX

#include <X11/keysym.h>

#define VK_BACK           XK_BackSpace
#define VK_TAB            XK_Tab
#define VK_RETURN         XK_Return
#define VK_SHIFT          XK_Shift_L
#define VK_CONTROL        XK_Control_L
#define VK_MENU           XK_Alt_L
#define VK_CAPITAL        XK_Caps_Lock
#define VK_ESCAPE         XK_Escape
#define VK_SPACE          XK_space
#define VK_END            XK_End
#define VK_PRIOR          XK_Prior
#define VK_NEXT           XK_Next
#define VK_HOME           XK_Home
#define VK_INSERT         XK_Insert
#define VK_DELETE         XK_Delete
#define VK_LEFT           XK_Left
#define VK_UP             XK_Up
#define VK_RIGHT          XK_Right
#define VK_DOWN           XK_Down
#define VK_NUMLOCK        XK_Num_Lock
#define VK_CLEAR          XK_Clear
#define VK_LWIN           XK_Super_L
#define VK_RWIN           XK_Super_R
#define VK_F1             XK_F1
#define VK_F2             XK_F2
#define VK_F3             XK_F3
#define VK_F4             XK_F4
#define VK_F5             XK_F5
#define VK_F6             XK_F6
#define VK_F7             XK_F7
#define VK_F8             XK_F8
#define VK_F9             XK_F9
#define VK_F10            XK_F10
#define VK_F11            XK_F11
#define VK_F12            XK_F12
#define VK_OEM_3          XK_quoteleft
#define VK_OEM_MINUS      XK_minus
#define VK_OEM_PLUS       XK_plus
#define VK_OEM_4          XK_bracketleft
#define VK_OEM_6          XK_bracketright
#define VK_OEM_1          XK_semicolon
#define VK_OEM_7          XK_quoteright
#define VK_OEM_5          XK_backslash
#define VK_OEM_COMMA      XK_comma
#define VK_OEM_PERIOD     XK_period
#define VK_OEM_2          XK_slash
#define VK_SCROLL         XK_Scroll_Lock
#define VK_PAUSE          XK_Pause
#define VK_NUMPAD0        XK_KP_0
#define VK_NUMPAD1        XK_KP_1
#define VK_NUMPAD2        XK_KP_2
#define VK_NUMPAD3        XK_KP_3
#define VK_NUMPAD4        XK_KP_4
#define VK_NUMPAD5        XK_KP_5
#define VK_NUMPAD6        XK_KP_6
#define VK_NUMPAD7        XK_KP_7
#define VK_NUMPAD8        XK_KP_8
#define VK_NUMPAD9        XK_KP_9
#define VK_MULTIPLY       XK_KP_Multiply
#define VK_ADD            XK_KP_Add
#define VK_SEPARATOR      XK_KP_Separator
#define VK_SUBTRACT       XK_KP_Subtract
#define VK_DECIMAL        XK_KP_Decimal
#define VK_DIVIDE         XK_KP_Divide
#define VK_RETURN_NUMPAD  XK_KP_Enter
#define VK_0              XK_0
#define VK_1              XK_1
#define VK_2              XK_2
#define VK_3              XK_3
#define VK_4              XK_4
#define VK_5              XK_5
#define VK_6              XK_6
#define VK_7              XK_7
#define VK_8              XK_8
#define VK_9              XK_9
#define VK_A              XK_a
#define VK_B              XK_b
#define VK_C              XK_c
#define VK_D              XK_d
#define VK_E              XK_e
#define VK_F              XK_f
#define VK_G              XK_g
#define VK_H              XK_h
#define VK_I              XK_i
#define VK_J              XK_j
#define VK_K              XK_k
#define VK_L              XK_l
#define VK_M              XK_m
#define VK_N              XK_n
#define VK_O              XK_o
#define VK_P              XK_p
#define VK_Q              XK_q
#define VK_R              XK_r
#define VK_S              XK_s
#define VK_T              XK_t
#define VK_U              XK_u
#define VK_V              XK_v
#define VK_W              XK_w
#define VK_X              XK_x
#define VK_Y              XK_y
#define VK_Z              XK_z

#elif defined PLATFORM_MAC || defined PLATFORM_IOS || defined PLATFORM_WASM || defined PLATFORM_ANDROID

#define VK_F1             -122
#define VK_F2             -120
#define VK_F3             -99
#define VK_F4             -118
#define VK_F5             -96
#define VK_F6             -97
#define VK_F7             -98
#define VK_F8             -100
#define VK_F9             -101
#define VK_F10            -109
#define VK_F11            -103
#define VK_F12            -111

#define VK_OEM_3          -50
#define VK_OEM_MINUS      -27
#define VK_OEM_PLUS       -24

#define VK_OEM_4          -33
#define VK_OEM_6          -30

#define VK_OEM_1          -41
#define VK_OEM_7          -39
#define VK_OEM_5          -42

#define VK_OEM_COMMA      -43
#define VK_OEM_PERIOD     -47
#define VK_OEM_2          -44

#define VK_ESCAPE         -53

#define VK_BACK           -51
#define VK_TAB            -48
#define VK_SHIFT          -56
#define VK_CONTROL        -59
#define VK_MENU           -58
#define VK_RETURN         -36
#define VK_SPACE          -49

#define VK_COMMAND        -60
#define VK_SELECT         -110
#define VK_SNAPSHOT       -105

#define VK_PRIOR          -116
#define VK_NEXT           -121
#define VK_END            -119
#define VK_HOME           -115
#define VK_DELETE         -117

#define VK_LEFT           -123
#define VK_UP             -126
#define VK_RIGHT          -124
#define VK_DOWN           -125

#define VK_NUMLOCK        -71
#define VK_CLEAR          -87

#define VK_NUMPAD0        -82
#define VK_NUMPAD1        -83
#define VK_NUMPAD2        -84
#define VK_NUMPAD3        -85
#define VK_NUMPAD4        -86
#define VK_NUMPAD5        -87
#define VK_NUMPAD6        -88
#define VK_NUMPAD7        -89
#define VK_NUMPAD8        -91
#define VK_NUMPAD9        -92

#define VK_MULTIPLY       -67
#define VK_ADD            -69
#define VK_SUBTRACT       -78
#define VK_DECIMAL        -65
#define VK_DIVIDE         -75
#define VK_RETURN_NUMPAD  -76

#define VK_0 48
#define VK_1 49
#define VK_2 50
#define VK_3 51
#define VK_4 52
#define VK_5 53
#define VK_6 54
#define VK_7 55
#define VK_8 56
#define VK_9 57
#define VK_A 'A'
#define VK_B 'B'    
#define VK_C 'C'
#define VK_D 'D'
#define VK_E 'E'
#define VK_F 'F'
#define VK_G 'G'
#define VK_H 'H'
#define VK_I 'I'
#define VK_J 'J'
#define VK_K 'K'
#define VK_L 'L'
#define VK_M 'M'
#define VK_N 'N'
#define VK_O 'O'
#define VK_P 'P'
#define VK_Q 'Q'
#define VK_R 'R'
#define VK_S 'S'
#define VK_T 'T'
#define VK_U 'U'
#define VK_V 'V'
#define VK_W 'W'
#define VK_X 'X'
#define VK_Y 'Y'
#define VK_Z 'Z'

#endif

// Cross-platform virtual key codes
#define VK_CTRL_CMD       -1000  // Ctrl on Windows/Linux, Cmd on Mac
