/* Grus
* Copyright (C) 1991-2024 Nationalchip Co., Ltd
*
* kws_list.h: a Circular Queue using array
*
*/

#ifndef __KWS_LIST_H__
#define __KWS_LIST_H__

#include <lvp_param.h>
#include <lvp_attr.h>

#define ENABLE_NEW_GROUPING_METHOD

int grout_offset_index[] = {0, 32};
LVP_KWS_PARAM g_kws_param_list[] XIP_RODATA_ATTR = {
    {"温度十六度", {168, 159, 31, 136, 130, 62, 108, 105, 31, 136}, 10, 719, 123, 0}, 
    {"温度十七度", {168, 159, 31, 136, 130, 62, 127, 61, 31, 136}, 10, 737, 124, 0}, 
    {"温度十八度", {168, 159, 31, 136, 130, 62, 28, 3, 31, 136}, 10, 704, 125, 0}, 
    {"温度十九度", {168, 159, 31, 136, 130, 62, 106, 104, 31, 136}, 10, 727, 126, 0}, 
    {"打开一档", {31, 5, 107, 9, 88, 61, 31, 22}, 8, 811, 105, 0}, 
    {"打开二档", {31, 5, 107, 9, 37, 56, 31, 22}, 8, 781, 106, 0}, 
    {"打开三档", {31, 5, 107, 9, 129, 14, 31, 22}, 8, 783, 107, 0}, 
    {"温水模式", {168, 159, 130, 156, 109, 113, 130, 64}, 8, 755, 117, 0}, 
    {"沸水模式", {57, 42, 130, 156, 109, 113, 130, 64}, 8, 749, 118, 0}, 
    {"小芯小芯", {187, 81, 187, 89, 187, 81, 187, 89}, 8, 831, 100, 1}, 
    {"晾杆上升", {108, 78, 58, 14, 130, 22, 130, 49}, 8, 761, 101, 0}, 
    {"衣架上升", {88, 61, 106, 69, 130, 22, 130, 49}, 8, 794, 101, 0}, 
    {"晾杆下降", {108, 78, 58, 14, 187, 69, 106, 78}, 8, 753, 102, 0}, 
    {"衣架下降", {88, 61, 106, 69, 187, 69, 106, 78}, 8, 787, 102, 0}, 
    {"停止升降", {131, 95, 189, 63, 130, 49, 106, 78}, 8, 762, 103, 0}, 
    {"暂停升降", {188, 17, 131, 95, 130, 49, 106, 78}, 8, 737, 103, 0}, 
    {"打开风扇", {31, 5, 107, 9, 57, 49, 130, 17}, 8, 783, 104, 0}, 
    {"关闭风扇", {58, 146, 28, 64, 57, 49, 130, 17}, 8, 779, 108, 0}, 
    {"播放音乐", {28, 112, 57, 22, 88, 89, 186, 181}, 8, 778, 109, 0}, 
    {"增大音量", {188, 49, 31, 6, 88, 89, 108, 78}, 8, 780, 112, 0}, 
    {"减小音量", {106, 72, 187, 81, 88, 89, 108, 78}, 8, 796, 113, 0}, 
    {"停止播放", {131, 95, 189, 63, 28, 112, 57, 22}, 8, 784, 114, 0}, 
    {"水壶加热", {130, 156, 59, 134, 106, 66, 128, 36}, 8, 749, 115, 0}, 
    {"停止加热", {131, 95, 189, 63, 106, 66, 128, 36}, 8, 768, 116, 0}, 
    {"泡茶模式", {126, 27, 30, 4, 109, 113, 130, 64}, 8, 732, 119, 0}, 
    {"功能全关", {58, 116, 110, 50, 127, 175, 58, 146}, 8, 742, 120, 0}, 
    {"打开雾化", {31, 5, 107, 9, 168, 136, 59, 141}, 8, 782, 121, 0}, 
    {"开启雾化", {107, 9, 127, 63, 168, 136, 59, 141}, 8, 767, 121, 0}, 
    {"关闭雾化", {58, 146, 28, 64, 168, 136, 59, 141}, 8, 775, 122, 0}, 
    {"取消雾化", {127, 172, 187, 79, 168, 136, 59, 141}, 8, 758, 122, 0}, 
    {"上一首", {130, 22, 88, 61, 130, 124}, 6, 915, 110, 0}, 
    {"下一首", {187, 69, 88, 61, 130, 124}, 6, 905, 111, 0}, 

};
#endif /* __KWS_LIST_H__ */
