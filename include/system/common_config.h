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


#define CONFIG_FONT_COLOR        0x6495ED
#define CONFIG_BACKGROUND_COLOR  DEEP_BROWN_COLOR

#define ICON_PATH "/etc/digitalpic/icons"

#if 1
#define DEBUG_PRINTF(...)
#else
#define DEBUG_PRINTF printf
#endif

#endif
