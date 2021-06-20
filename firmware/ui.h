#ifndef UI_H_
#define UI_H_

#include <stdint.h>

enum {BTN_PRESSED, ENC_LEFT, ENC_RIGHT, ANIM_NEXT_FRAME, DUMMY_EVENT};

void ui_next(uint32_t event);
void init_ui(void);

#endif // UI_H_
