#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

#define DEBUG_EMERG   "<0>"   
#define DEBUG_ALERT   "<1>"   
#define DEBUG_CRIT    "<2>"   
#define DEBUG_ERR     "<3>"   
#define DEBUG_WARNING "<4>"   
#define DEBUG_NOTICE  "<5>"   
#define DEBUG_INFO    "<6>"   
#define DEBUG_DEBUG   "<7>"

#define DEBUG_DEFAULT 4

#define BRONZES_COLOR    0xA62AA2
#define DEEP_BROWN_COLOR 0x5A4033
#define PALE_TURQUOISE_2 0xAEEEEE
#define PALE_TURQUOISE_3 0x96CDCD
#define PALE_TURQUOISE_4 0x668B8B
#define CADET_BLUE_1     0x98F5FF
#define CADET_BLUE_2     0x8EE5EE
#define CADET_BLUE_3     0x7AC5CD
#define LIGHT_GRAY       0xD3D3D3
#define MISTYROSE        0xFFE4E1
#define TAN4             0x8B5A2B
#define BURLYWOOD4       0x8B7355
#define DARKSLATEGRAY    0x2F4F4F

#define CONFIG_FONT_COLOR        0x6495ED
#define CONFIG_BACKGROUND_COLOR  DEEP_BROWN_COLOR
#define CONFIG_MUSIC_BG_COLOR    DARKSLATEGRAY
#define CONFIG_PROGRESS_BG_COLOR PALE_TURQUOISE_4
#define CONFIG_PROGRESS_COLOR    CADET_BLUE_2

#define ICON_PATH "/etc/digitalpic/icons"

#if 1
#define DEBUG_PRINTF(...)
#else
#define DEBUG_PRINTF printf
#endif

#endif
