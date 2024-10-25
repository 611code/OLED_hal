//
// Created by 18681 on 24-10-8.
//
#include "oled.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
uint8_t OLED_GRAM[OLED_PAGE][OLED_COLUMN];


static void OLED_Send(uint8_t *data, uint8_t len)
{
    HAL_I2C_Master_Transmit(&OLED_hi2c, OLED_ADDRESS, data, len, HAL_MAX_DELAY);
}
static void OLED_SendCmd(uint8_t cmd)
{
    static uint8_t sendBuffer[2] = {0};
    sendBuffer[1] = cmd;
    OLED_Send(sendBuffer, 2);
}
static void OLED_SetCursor(uint8_t Page, uint8_t X)
{
    OLED_SendCmd(0xB0 | Page);					//设置页位置
    OLED_SendCmd(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
    OLED_SendCmd(0x00 | (X & 0x0F));			//设置X位置低4位
}
static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;	//结果默认为1
    while (Y --)			//累乘Y次
    {
        Result *= X;		//每次把X累乘到结果上
    }
    return Result;
}



void OLED_Init()
{
    OLED_SendCmd(0xAE);
    OLED_SendCmd(0xD5);
    OLED_SendCmd(0x80);
    OLED_SendCmd(0xA8);
    OLED_SendCmd(0x3F);
    OLED_SendCmd(0xD3);
    OLED_SendCmd(0x00);
    OLED_SendCmd(0x40);
    OLED_SendCmd(0xA1);
    OLED_SendCmd(0xC8);
    OLED_SendCmd(0xDA);
    OLED_SendCmd(0x12);
    OLED_SendCmd(0x81);
    OLED_SendCmd(0xCF);
    OLED_SendCmd(0xD9);
    OLED_SendCmd(0xF1);
    OLED_SendCmd(0xDB);
    OLED_SendCmd(0x30);
    OLED_SendCmd(0xA4);
    OLED_SendCmd(0xA6);
    OLED_SendCmd(0x8D);
    OLED_SendCmd(0x14);
    OLED_SendCmd(0xAF);
    OLED_Clear();
    OLED_Update();
}

void OLED_Update(void)
{
    static uint8_t sendBuffer[OLED_COLUMN + 1];
    sendBuffer[0] = 0x40;
    for (uint8_t i = 0; i < OLED_PAGE; i++)
    {
        OLED_SendCmd(0xB0 + i); // 设置页地址
#ifdef sh1106
        OLED_SendCmd(0x00+2);     // 设置列地址低4位
#elifdef ssd1306
        OLED_SendCmd(0x00);     // 设置列地址低4位
#endif
        OLED_SendCmd(0x10);     // 设置列地址高4位
        memcpy(sendBuffer + 1, OLED_GRAM[i], OLED_COLUMN);
        OLED_Send(sendBuffer, OLED_COLUMN + 1);
    }
}
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t j;
    int16_t Page, Page1;

    /*负数坐标在计算页地址时需要加一个偏移*/
    /*(Y + Height - 1) / 8 + 1的目的是(Y + Height) / 8并向上取整*/
    Page = Y / 8;
    Page1 = (Y + Height - 1) / 8 + 1;
    if (Y < 0)
    {
        Page -= 1;
        Page1 -= 1;
    }
    /*遍历指定区域涉及的相关页*/
    for (j = Page; j < Page1; j ++)
    {
        if (X >= 0 && X <= 127 && j >= 0 && j <= 7)        //超出屏幕的内容不显示
        {
            /*设置光标位置为相关页的指定列*/
            OLED_SetCursor(j, X);
            /*连续写入Width个数据，将显存数组的数据写入到OLED硬件*/
            OLED_Send(&OLED_GRAM[j][X], Width);
        }
    }
}
void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;

    for (j = Y; j < Y + Height; j ++)        //遍历指定页
    {
        for (i = X; i < X + Width; i ++)    //遍历指定列
        {
            if (i >= 0 && i <= 127 && j >=0 && j <= 63)     //超出屏幕的内容不显示
            {
                OLED_GRAM[j / 8][i] &= ~(0x01 << (j % 8));//将显存数组指定数据清零
            }
        }
    }
}
void OLED_Reverse(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j ++)				//遍历8页
    {
        for (i = 0; i < 128; i ++)			//遍历128列
        {
            OLED_GRAM[j][i] ^= 0xFF;	//将显存数组数据全部取反
        }
    }
}
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;

    for (j = Y; j < Y + Height; j ++)		//遍历指定页
    {
        for (i = X; i < X + Width; i ++)	//遍历指定列
        {
            if (i >= 0 && i <= 127 && j >=0 && j <= 63)			//超出屏幕的内容不显示
            {
                OLED_GRAM[j / 8][i] ^= 0x01 << (j % 8);
            }
        }
    }
}


void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;
    /*将图像所在区域清空*/
    OLED_ClearArea(X, Y, Width, Height);
    /*遍历指定图像涉及的相关页*/
    /*(Height - 1) / 8 + 1的目的是Height / 8并向上取整*/
    for (j = 0; j < (Height - 1) / 8 + 1; j ++)
    {
        /*遍历指定图像涉及的相关列*/
        for (i = 0; i < Width; i ++)
        {
            if (X + i >= 0 && X + i <= 127)        //超出屏幕的内容不显示
            {
                /*负数坐标在计算页地址和移位时需要加一个偏移*/
                Page = Y / 8;
                Shift = Y % 8;
                if (Y < 0)
                {
                    Page -= 1;
                    Shift += 8;
                }
                if (Page + j >= 0 && Page + j <= 7)        //超出屏幕的内容不显示
                {
                    /*显示图像在当前页的内容*/
                    // 往左移动shift,得到低位
                    OLED_GRAM[Page + j][X + i] |= Image[j * Width + i] << (Shift);
                }
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7)        //超出屏幕的内容不显示
                {
                    /*显示图像在下一页的内容*/
                    // 往右移动(8-shift),得到剩下的高位
                    OLED_GRAM[Page + j + 1][X + i] |= Image[j * Width + i] >> (8 - Shift);
                }
            }
        }
    }
}
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
    if (FontSize == OLED_8X16)		//字体为宽8像素，高16像素
    {
        /*将ASCII字模库OLED_F8x16的指定数据以8*16的图像格式显示*/
        OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
    }
    else if(FontSize == OLED_6X8)	//字体为宽6像素，高8像素
    {
        /*将ASCII字模库OLED_F6x8的指定数据以6*8的图像格式显示*/
        OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
    }
}
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)		//遍历字符串的每个字符
    {
        /*调用OLED_ShowChar函数，依次显示每个字符*/
        OLED_ShowChar(X + i * FontSize, Y, String[i], FontSize);
    }
}
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)		//遍历数字的每一位
    {
        /*调用OLED_ShowChar函数，依次显示每个数字*/
        /*Number / OLED_Pow(10, Length - i - 1) % 10 可以十进制提取数字的每一位*/
        /*+ '0' 可将数字转换为字符格式*/
        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    uint32_t Number1;

    if (Number >= 0)						//数字大于等于0
    {
        OLED_ShowChar(X, Y, '+', FontSize);	//显示+号
        Number1 = Number;					//Number1直接等于Number
    }
    else									//数字小于0
    {
        OLED_ShowChar(X, Y, '-', FontSize);	//显示-号
        Number1 = -Number;					//Number1等于Number取负
    }

    for (i = 0; i < Length; i++)			//遍历数字的每一位
    {
        /*调用OLED_ShowChar函数，依次显示每个数字*/
        /*Number1 / OLED_Pow(10, Length - i - 1) % 10 可以十进制提取数字的每一位*/
        /*+ '0' 可将数字转换为字符格式*/
        OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)		//遍历数字的每一位
    {
        /*以十六进制提取数字的每一位*/
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;

        if (SingleNumber < 10)			//单个数字小于10
        {
            /*调用OLED_ShowChar函数，显示此数字*/
            /*+ '0' 可将数字转换为字符格式*/
            OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
        }
        else							//单个数字大于10
        {
            /*调用OLED_ShowChar函数，显示此数字*/
            /*+ 'A' 可将数字转换为从A开始的十六进制字符*/
            OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
        }
    }
}
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)		//遍历数字的每一位
    {
        /*调用OLED_ShowChar函数，依次显示每个数字*/
        /*Number / OLED_Pow(2, Length - i - 1) % 2 可以二进制提取数字的每一位*/
        /*+ '0' 可将数字转换为字符格式*/
        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
    }
}
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
    uint32_t PowNum, IntNum, FraNum;

    if (Number >= 0)						//数字大于等于0
    {
        OLED_ShowChar(X, Y, '+', FontSize);	//显示+号
    }
    else									//数字小于0
    {
        OLED_ShowChar(X, Y, '-', FontSize);	//显示-号
        Number = -Number;					//Number取负
    }

    /*提取整数部分和小数部分*/
    IntNum = Number;						//直接赋值给整型变量，提取整数
    Number -= IntNum;						//将Number的整数减掉，防止之后将小数乘到整数时因数过大造成错误
    PowNum = OLED_Pow(10, FraLength);		//根据指定小数的位数，确定乘数
    FraNum = round(Number * PowNum);		//将小数乘到整数，同时四舍五入，避免显示误差
    IntNum += FraNum / PowNum;				//若四舍五入造成了进位，则需要再加给整数

    /*显示整数部分*/
    OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);

    /*显示小数点*/
    OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);

    /*显示小数部分*/
    OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}
void OLED_ShowChinese(int16_t X, int16_t Y, char *Chinese)
{
    uint8_t pChinese = 0;
    uint8_t pIndex;
    uint8_t i;
    char SingleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};

    for (i = 0; Chinese[i] != '\0'; i ++)		//遍历汉字串
    {
        SingleChinese[pChinese] = Chinese[i];	//提取汉字串数据到单个汉字数组
        pChinese ++;							//计次自增

        /*当提取次数到达OLED_CHN_CHAR_WIDTH时，即代表提取到了一个完整的汉字*/
        if (pChinese >= OLED_CHN_CHAR_WIDTH)
        {
            pChinese = 0;		//计次归零

            /*遍历整个汉字字模库，寻找匹配的汉字*/
            /*如果找到最后一个汉字（定义为空字符串），则表示汉字未在字模库定义，停止寻找*/
            for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex ++)
            {
                /*找到匹配的汉字*/
                if (strcmp(OLED_CF16x16[pIndex].Index, SingleChinese) == 0)
                {
                    break;		//跳出循环，此时pIndex的值为指定汉字的索引
                }
            }

            /*将汉字字模库OLED_CF16x16的指定数据以16*16的图像格式显示*/
            OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * 16, Y, 16, 16, OLED_CF16x16[pIndex].Data);
        }
    }
}
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
    char String[256];						//定义字符数组
    va_list arg;							//定义可变参数列表数据类型的变量arg
    va_start(arg, format);					//从format开始，接收参数列表到arg变量
    vsprintf(String, format, arg);			//使用vsprintf打印格式化字符串和参数列表到字符数组中
    va_end(arg);							//结束变量arg
    OLED_ShowString(X, Y, String, FontSize);//OLED显示字符数组（字符串）
}



static uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
    int16_t PointAngle;
    PointAngle = atan2(Y, X) / 3.14 * 180;	//计算指定点的弧度，并转换为角度表示
    if (StartAngle < EndAngle)	//起始角度小于终止角度的情况
    {
        /*如果指定角度在起始终止角度之间，则判定指定点在指定角度*/
        if (PointAngle >= StartAngle && PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    else			//起始角度大于于终止角度的情况
    {
        /*如果指定角度大于起始角度或者小于终止角度，则判定指定点在指定角度*/
        if (PointAngle >= StartAngle || PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    return 0;		//不满足以上条件，则判断判定指定点不在指定角度
}
static uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
    int16_t i, j, c = 0;
    /*此算法由W. Randolph Franklin提出*/
    /*参考链接：https://wrfranklin.org/Research/Short_Notes/pnpoly.html*/
    for (i = 0, j = nvert - 1; i < nvert; j = i++)
    {
        if (((verty[i] > testy) != (verty[j] > testy)) &&
            (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
        {
            c = !c;
        }
    }
    return c;
}
static uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
    if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//超出屏幕的内容不读取
    {
        /*判断指定位置的数据*/
        if (OLED_GRAM[Y / 8][X] & 0x01 << (Y % 8))
        {
            return 1;	//为1，返回1
        }
    }

    return 0;		//否则，返回0
}
void OLED_DrawPoint(int16_t X, int16_t Y)
{
    if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//超出屏幕的内容不显示
    {
        /*将显存数组指定位置的一个Bit数据置1*/
        OLED_GRAM[Y / 8][X] |= 0x01 << (Y % 8);
    }
}
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    int16_t x, y, dx, dy, d, incrE, incrNE, temp;
    int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
    uint8_t yflag = 0, xyflag = 0;

    if (y0 == y1)		//横线单独处理
    {
        /*0号点X坐标大于1号点X坐标，则交换两点X坐标*/
        if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}

        /*遍历X坐标*/
        for (x = x0; x <= x1; x ++)
        {
            OLED_DrawPoint(x, y0);	//依次画点
        }
    }
    else if (x0 == x1)	//竖线单独处理
    {
        /*0号点Y坐标大于1号点Y坐标，则交换两点Y坐标*/
        if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}

        /*遍历Y坐标*/
        for (y = y0; y <= y1; y ++)
        {
            OLED_DrawPoint(x0, y);	//依次画点
        }
    }
    else				//斜线
    {
        /*使用Bresenham算法画直线，可以避免耗时的浮点运算，效率更高*/
        /*参考文档：https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
        /*参考教程：https://www.bilibili.com/video/BV1364y1d7Lo*/

        if (x0 > x1)	//0号点X坐标大于1号点X坐标
        {
            /*交换两点坐标*/
            /*交换后不影响画线，但是画线方向由第一、二、三、四象限变为第一、四象限*/
            temp = x0; x0 = x1; x1 = temp;
            temp = y0; y0 = y1; y1 = temp;
        }

        if (y0 > y1)	//0号点Y坐标大于1号点Y坐标
        {
            /*将Y坐标取负*/
            /*取负后影响画线，但是画线方向由第一、四象限变为第一象限*/
            y0 = -y0;
            y1 = -y1;

            /*置标志位yflag，记住当前变换，在后续实际画线时，再将坐标换回来*/
            yflag = 1;
        }

        if (y1 - y0 > x1 - x0)	//画线斜率大于1
        {
            /*将X坐标与Y坐标互换*/
            /*互换后影响画线，但是画线方向由第一象限0~90度范围变为第一象限0~45度范围*/
            temp = x0; x0 = y0; y0 = temp;
            temp = x1; x1 = y1; y1 = temp;

            /*置标志位xyflag，记住当前变换，在后续实际画线时，再将坐标换回来*/
            xyflag = 1;
        }

        /*以下为Bresenham算法画直线*/
        /*算法要求，画线方向必须为第一象限0~45度范围*/
        dx = x1 - x0;
        dy = y1 - y0;
        incrE = 2 * dy;
        incrNE = 2 * (dy - dx);
        d = 2 * dy - dx;
        x = x0;
        y = y0;

        /*画起始点，同时判断标志位，将坐标换回来*/
        if (yflag && xyflag){OLED_DrawPoint(y, -x);}
        else if (yflag)		{OLED_DrawPoint(x, -y);}
        else if (xyflag)	{OLED_DrawPoint(y, x);}
        else				{OLED_DrawPoint(x, y);}

        while (x < x1)		//遍历X轴的每个点
        {
            x ++;
            if (d < 0)		//下一个点在当前点东方
            {
                d += incrE;
            }
            else			//下一个点在当前点东北方
            {
                y ++;
                d += incrNE;
            }

            /*画每一个点，同时判断标志位，将坐标换回来*/
            if (yflag && xyflag){OLED_DrawPoint(y, -x);}
            else if (yflag)		{OLED_DrawPoint(x, -y);}
            else if (xyflag)	{OLED_DrawPoint(y, x);}
            else				{OLED_DrawPoint(x, y);}
        }
    }
}
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    int16_t i, j;
    if (!IsFilled)		//指定矩形不填充
    {
        /*遍历上下X坐标，画矩形上下两条线*/
        for (i = X; i < X + Width; i ++)
        {
            OLED_DrawPoint(i, Y);
            OLED_DrawPoint(i, Y + Height - 1);
        }
        /*遍历左右Y坐标，画矩形左右两条线*/
        for (i = Y; i < Y + Height; i ++)
        {
            OLED_DrawPoint(X, i);
            OLED_DrawPoint(X + Width - 1, i);
        }
    }
    else				//指定矩形填充
    {
        /*遍历X坐标*/
        for (i = X; i < X + Width; i ++)
        {
            /*遍历Y坐标*/
            for (j = Y; j < Y + Height; j ++)
            {
                /*在指定区域画点，填充满矩形*/
                OLED_DrawPoint(i, j);
            }
        }
    }
}
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
    int16_t i, j;
    int16_t vx[] = {X0, X1, X2};
    int16_t vy[] = {Y0, Y1, Y2};

    if (!IsFilled)			//指定三角形不填充
    {
        /*调用画线函数，将三个点用直线连接*/
        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X0, Y0, X2, Y2);
        OLED_DrawLine(X1, Y1, X2, Y2);
    }
    else					//指定三角形填充
    {
        /*找到三个点最小的X、Y坐标*/
        if (X1 < minx) {minx = X1;}
        if (X2 < minx) {minx = X2;}
        if (Y1 < miny) {miny = Y1;}
        if (Y2 < miny) {miny = Y2;}

        /*找到三个点最大的X、Y坐标*/
        if (X1 > maxx) {maxx = X1;}
        if (X2 > maxx) {maxx = X2;}
        if (Y1 > maxy) {maxy = Y1;}
        if (Y2 > maxy) {maxy = Y2;}

        /*最小最大坐标之间的矩形为可能需要填充的区域*/
        /*遍历此区域中所有的点*/
        /*遍历X坐标*/
        for (i = minx; i <= maxx; i ++)
        {
            /*遍历Y坐标*/
            for (j = miny; j <= maxy; j ++)
            {
                /*调用OLED_pnpoly，判断指定点是否在指定三角形之中*/
                /*如果在，则画点，如果不在，则不做处理*/
                if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j);}
            }
        }
    }
}
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
    int16_t x, y, d, j;

    /*使用Bresenham算法画圆，可以避免耗时的浮点运算，效率更高*/
    /*参考文档：https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
    /*参考教程：https://www.bilibili.com/video/BV1VM4y1u7wJ*/

    d = 1 - Radius;
    x = 0;
    y = Radius;

    /*画每个八分之一圆弧的起始点*/
    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X + y, Y + x);
    OLED_DrawPoint(X - y, Y - x);

    if (IsFilled)		//指定圆填充
    {
        /*遍历起始点Y坐标*/
        for (j = -y; j < y; j ++)
        {
            /*在指定区域画点，填充部分圆*/
            OLED_DrawPoint(X, Y + j);
        }
    }

    while (x < y)		//遍历X轴的每个点
    {
        x ++;
        if (d < 0)		//下一个点在当前点东方
        {
            d += 2 * x + 1;
        }
        else			//下一个点在当前点东南方
        {
            y --;
            d += 2 * (x - y) + 1;
        }

        /*画每个八分之一圆弧的点*/
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X + y, Y + x);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - y, Y - x);
        OLED_DrawPoint(X + x, Y - y);
        OLED_DrawPoint(X + y, Y - x);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X - y, Y + x);

        if (IsFilled)	//指定圆填充
        {
            /*遍历中间部分*/
            for (j = -y; j < y; j ++)
            {
                /*在指定区域画点，填充部分圆*/
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }

            /*遍历两侧部分*/
            for (j = -x; j < x; j ++)
            {
                /*在指定区域画点，填充部分圆*/
                OLED_DrawPoint(X - y, Y + j);
                OLED_DrawPoint(X + y, Y + j);
            }
        }
    }
}
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    int16_t x, y, j;
    int16_t a = A, b = B;
    float d1, d2;

    /*使用Bresenham算法画椭圆，可以避免部分耗时的浮点运算，效率更高*/
    /*参考链接：https://blog.csdn.net/myf_666/article/details/128167392*/

    x = 0;
    y = b;
    d1 = b * b + a * a * (-b + 0.5);

    if (IsFilled)	//指定椭圆填充
    {
        /*遍历起始点Y坐标*/
        for (j = -y; j < y; j ++)
        {
            /*在指定区域画点，填充部分椭圆*/
            OLED_DrawPoint(X, Y + j);
            OLED_DrawPoint(X, Y + j);
        }
    }

    /*画椭圆弧的起始点*/
    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X - x, Y + y);
    OLED_DrawPoint(X + x, Y - y);

    /*画椭圆中间部分*/
    while (b * b * (x + 1) < a * a * (y - 0.5))
    {
        if (d1 <= 0)		//下一个点在当前点东方
        {
            d1 += b * b * (2 * x + 3);
        }
        else				//下一个点在当前点东南方
        {
            d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
            y --;
        }
        x ++;

        if (IsFilled)	//指定椭圆填充
        {
            /*遍历中间部分*/
            for (j = -y; j < y; j ++)
            {
                /*在指定区域画点，填充部分椭圆*/
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }

        /*画椭圆中间部分圆弧*/
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }

    /*画椭圆两侧部分*/
    d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;

    while (y > 0)
    {
        if (d2 <= 0)		//下一个点在当前点东方
        {
            d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
            x ++;

        }
        else				//下一个点在当前点东南方
        {
            d2 += a * a * (-2 * y + 3);
        }
        y --;

        if (IsFilled)	//指定椭圆填充
        {
            /*遍历两侧部分*/
            for (j = -y; j < y; j ++)
            {
                /*在指定区域画点，填充部分椭圆*/
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }

        /*画椭圆两侧部分圆弧*/
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }
}
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    int16_t x, y, d, j;

    /*此函数借用Bresenham算法画圆的方法*/

    d = 1 - Radius;
    x = 0;
    y = Radius;

    /*在画圆的每个点时，判断指定点是否在指定角度内，在，则画点，不在，则不做处理*/
    if (OLED_IsInAngle(x, y, StartAngle, EndAngle))	{OLED_DrawPoint(X + x, Y + y);}
    if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
    if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
    if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}

    if (IsFilled)	//指定圆弧填充
    {
        /*遍历起始点Y坐标*/
        for (j = -y; j < y; j ++)
        {
            /*在填充圆的每个点时，判断指定点是否在指定角度内，在，则画点，不在，则不做处理*/
            if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j);}
        }
    }

    while (x < y)		//遍历X轴的每个点
    {
        x ++;
        if (d < 0)		//下一个点在当前点东方
        {
            d += 2 * x + 1;
        }
        else			//下一个点在当前点东南方
        {
            y --;
            d += 2 * (x - y) + 1;
        }

        /*在画圆的每个点时，判断指定点是否在指定角度内，在，则画点，不在，则不做处理*/
        if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y);}
        if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
        if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
        if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
        if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y - y);}
        if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x);}
        if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y);}
        if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x);}

        if (IsFilled)	//指定圆弧填充
        {
            /*遍历中间部分*/
            for (j = -y; j < y; j ++)
            {
                /*在填充圆的每个点时，判断指定点是否在指定角度内，在，则画点，不在，则不做处理*/
                if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j);}
                if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j);}
            }

            /*遍历两侧部分*/
            for (j = -x; j < x; j ++)
            {
                /*在填充圆的每个点时，判断指定点是否在指定角度内，在，则画点，不在，则不做处理*/
                if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j);}
                if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j);}
            }
        }
    }
}



