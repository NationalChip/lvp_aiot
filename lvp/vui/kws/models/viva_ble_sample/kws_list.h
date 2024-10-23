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

int grout_offset_index[] = {0, 39};
LVP_KWS_PARAM g_kws_param_list[] XIP_RODATA_ATTR = {
    {"打开中性光", {31, 5, 107, 9, 189, 116, 187, 97, 58, 150}, 10, 651, 109, 0}, 
    {"打开辅助灯", {31, 5, 107, 9, 57, 135, 189, 136, 31, 49}, 10, 679, 111, 0}, 
    {"关闭辅助灯", {58, 146, 28, 64, 57, 135, 189, 136, 31, 49}, 10, 688, 112, 0}, 
    {"打开灯光", {31, 5, 107, 9, 31, 49, 58, 150}, 8, 807, 101, 0}, 
    {"打开白光", {31, 5, 107, 9, 28, 10, 58, 150}, 8, 788, 107, 0}, 
    {"打开黄光", {31, 5, 107, 9, 59, 151, 58, 150}, 8, 761, 108, 0}, 
    {"开小夜灯", {107, 9, 187, 81, 88, 87, 31, 49}, 8, 777, 110, 0}, 
    {"开辅助灯", {107, 9, 57, 135, 189, 136, 31, 49}, 8, 750, 111, 0}, 
    {"关辅助灯", {58, 146, 57, 135, 189, 136, 31, 49}, 8, 748, 112, 0}, 
    {"最高亮度", {188, 157, 58, 24, 108, 78, 31, 136}, 8, 791, 103, 0}, 
    {"最低亮度", {188, 157, 31, 61, 108, 78, 31, 136}, 8, 784, 104, 0}, 
    {"最小亮度", {188, 157, 187, 81, 108, 78, 31, 136}, 8, 808, 104, 0}, 
    {"最大亮度", {188, 157, 31, 6, 108, 78, 31, 136}, 8, 806, 103, 0}, 
    {"调亮一点", {131, 80, 108, 78, 88, 61, 31, 72}, 8, 797, 103, 0}, 
    {"调暗一点", {131, 80, 7, 17, 88, 61, 31, 72}, 8, 828, 104, 0}, 
    {"小漠小漠", {187, 81, 109, 115, 187, 81, 109, 115}, 8, 771, 100, 1}, 
    {"把灯打开", {28, 5, 31, 49, 31, 5, 107, 9}, 8, 787, 101, 0}, 
    {"我回来了", {168, 166, 59, 155, 108, 10, 108, 32}, 8, 867, 101, 0}, 
    {"我起床了", {168, 166, 127, 63, 30, 151, 108, 32}, 8, 803, 101, 0}, 
    {"关闭灯光", {58, 146, 28, 64, 31, 49, 58, 150}, 8, 773, 102, 0}, 
    {"把灯关闭", {28, 5, 31, 49, 58, 146, 28, 64}, 8, 783, 102, 0}, 
    {"中等亮度", {189, 116, 31, 51, 108, 78, 31, 136}, 8, 764, 105, 0}, 
    {"切换色温", {127, 87, 59, 149, 129, 36, 168, 159}, 8, 720, 106, 0}, 
    {"开中性光", {107, 9, 189, 116, 187, 97, 58, 150}, 8, 724, 109, 0}, 
    {"我出去了", {168, 166, 30, 133, 127, 173, 108, 32}, 8, 843, 113, 0}, 
    {"打开灯", {31, 5, 107, 9, 31, 49}, 6, 909, 101, 0| (4 << 16)}, 
    {"关闭灯", {58, 146, 28, 64, 31, 49}, 6, 864, 102, 0| (4 << 16)}, 
    {"亮一点", {108, 78, 88, 61, 31, 72}, 6, 901, 103, 0}, 
    {"暗一点", {7, 17, 88, 61, 31, 72}, 6, 923, 104, 0}, 
    {"开白光", {107, 9, 28, 10, 58, 150}, 6, 857, 107, 0}, 
    {"开黄光", {107, 9, 59, 151, 58, 150}, 6, 818, 108, 0}, 
    {"变颜色", {28, 73, 88, 71, 129, 36}, 6, 849, 106, 0}, 
    {"调色温", {131, 80, 129, 36, 168, 159}, 6, 812, 106, 0}, 
    {"中性光", {189, 116, 187, 97, 58, 150}, 6, 839, 109, 0}, 
    {"小夜灯", {187, 81, 88, 87, 31, 49}, 6, 864, 110, 0}, 
    {"开灯", {107, 9, 31, 49}, 4, 956, 101, 0}, 
    {"关灯", {58, 146, 31, 49}, 4, 959, 102, 0}, 
    {"白光", {28, 10, 58, 150}, 4, 926, 107, 0}, 
    {"黄光", {59, 151, 58, 150}, 4, 924, 108, 0}, 

};
#endif /* __KWS_LIST_H__ */
