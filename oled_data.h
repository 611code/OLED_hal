//
// Created by 18681 on 24-10-9.
//

#ifndef OLED_DATA_H
#define OLED_DATA_H

#include <stdint.h>
//UTF-8编码格式给3，GB2312编码格式给2
#define OLED_CHN_CHAR_WIDTH	3

extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F6x8[][6];
#define OLED_8X16 8
#define OLED_6X8 6

/*汉字结构体*/
typedef struct
{
    char Index[OLED_CHN_CHAR_WIDTH + 1];	//汉字索引
    uint8_t Data[32];						//字模数据
} OLED_ChineseCell_t;
extern const OLED_ChineseCell_t OLED_CF16x16[];

#endif //OLED_DIVER_OLED_DATA_H
