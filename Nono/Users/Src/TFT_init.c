#include "TFT_init.h"
#include "tim.h"
#include "spi.h"
#include "TFT.h"
#include "cmsis_os.h"
/*PWM控制屏幕亮度
level 0-100
*/
void LCD_BLK(uint8_t level)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, level);
}

/*片选成功
用spi传输8Bit数据
*/
void LCD_WR8(uint8_t data)
{
    LCD_CS_Clr();
    HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
    LCD_CS_Set();
}

/*
传输8位数据
*/
void LCD_WR_DATA8(uint8_t dat)
{

    LCD_WR8(dat);
}

/*
传输16位数据
*/
void LCD_WR_DATA16(uint16_t dat)
{
    LCD_WR8(dat >> 8);
    LCD_WR8(dat);
}

/*
传输指令
*/
void LCD_WR_REG(uint8_t dat)
{
    LCD_DC_Clr();
    LCD_WR8(dat);
    LCD_DC_Set();
}

/*
设置坐标
*/
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WR_REG(0x2a);  // 选择列地址设置命令
    LCD_WR_DATA16(x1); // 写入起始列地址 x1
    LCD_WR_DATA16(x2); // 写入结束列地址 x2
    LCD_WR_REG(0x2b);  // 选择行地址设置命令
    LCD_WR_DATA16(y1); // 写入起始行地址 y1
    LCD_WR_DATA16(y2); // 写入结束行地址 y2
    LCD_WR_REG(0x2c);  // 选择内存写入命令
}

void LCD_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // 打开背光控制PWM输出

    LCD_RES_Clr(); // 复位
    HAL_Delay(100);
    LCD_RES_Set();
    HAL_Delay(100);

    LCD_BLK(100);

    LCD_WR_REG(0xFE); // 寄存器使能
    LCD_WR_REG(0xEF);

    LCD_WR_REG(0x84); // 删掉则是镜像模式
    LCD_WR_DATA8(0x40);

    LCD_WR_REG(0xB6); // 屏幕功能配置
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x20);

    LCD_WR_REG(0x36); // 作用是扫描模式
    if (USE_HORIZONTAL == 0)
        LCD_WR_DATA8(0x08);
    else if (USE_HORIZONTAL == 1)
        LCD_WR_DATA8(0xC8);
    else if (USE_HORIZONTAL == 2)
        LCD_WR_DATA8(0x68);
    else
        LCD_WR_DATA8(0xA8);

    LCD_WR_REG(0x3A); // 颜色模式选择，确定每个像素由几bits组成
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xC3); // 电源控制
    LCD_WR_DATA8(0x13);
    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x13);
    LCD_WR_REG(0xC9);
    LCD_WR_DATA8(0x22);

    LCD_WR_REG(0xF0); // 伽马设置
    LCD_WR_DATA8(0x45);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x2A);

    LCD_WR_REG(0xF1); // 伽马设置
    LCD_WR_DATA8(0x43);
    LCD_WR_DATA8(0x70);
    LCD_WR_DATA8(0x72);
    LCD_WR_DATA8(0x36);
    LCD_WR_DATA8(0x37);
    LCD_WR_DATA8(0x6F);

    LCD_WR_REG(0xF2); // 伽马设置
    LCD_WR_DATA8(0x45);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x2A);

    LCD_WR_REG(0xF3); // 伽马设置
    LCD_WR_DATA8(0x43);
    LCD_WR_DATA8(0x70);
    LCD_WR_DATA8(0x72);
    LCD_WR_DATA8(0x36);
    LCD_WR_DATA8(0x37);
    LCD_WR_DATA8(0x6F);

    LCD_WR_REG(0xE8); // 帧率
    LCD_WR_DATA8(0x34);

    LCD_WR_REG(0x66);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0xCD);
    LCD_WR_DATA8(0x67);
    LCD_WR_DATA8(0x45);
    LCD_WR_DATA8(0x45);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x00);

    LCD_WR_REG(0x67);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x54);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x32);
    LCD_WR_DATA8(0x98);

    LCD_WR_REG(0x34); // 剪切效果线打开0x35，关闭是0x34

    LCD_WR_REG(0x21); // 颜色反转  20是关闭,21是打开

    LCD_WR_REG(0x11); // 退出休眠模式，10是睡眠模式

    LCD_WR_REG(0x29); // 开始显示
		HAL_Delay(100);
    LCD_Fill(0, 0, 240, 240, BLACK);
}

