#include <stdio.h>
#include <lvp_attr.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wmissing-braces"

typedef struct gxdecoder_id {
    char word[30];
    int word_id;
}gxdecoder_id;
gxdecoder_id gxdecoder_data[] XIP_RODATA_ATTR = {
{"<eps>", 0},
{"<UNK>", 1},
{"上", 2},
{"上升", 3},
{"下", 4},
{"下降", 5},
{"停", 6},
{"停停", 7},
{"停止", 8},
{"全", 9},
{"全关", 10},
{"关", 11},
{"关关", 12},
{"关闭", 13},
{"冷", 14},
{"冷风", 15},
{"功", 16},
{"功功", 17},
{"功能", 18},
{"升", 19},
{"升降", 20},
{"吹", 21},
{"吹风", 22},
{"好", 23},
{"小", 24},
{"小好", 25},
{"干", 26},
{"干干", 27},
{"开", 28},
{"开开", 29},
{"打", 30},
{"打开", 31},
{"打打", 32},
{"明", 33},
{"明明", 34},
{"晾", 35},
{"晾晾", 36},
{"晾杆", 37},
{"暂", 38},
{"暂停", 39},
{"暂暂", 40},
{"杆", 41},
{"杆杆", 42},
{"架", 43},
{"架架", 44},
{"止", 45},
{"止止", 46},
{"毒", 47},
{"毒毒", 48},
{"消", 49},
{"消毒", 50},
{"消消", 51},
{"烘", 52},
{"烘干", 53},
{"烘烘", 54},
{"照", 55},
{"照明", 56},
{"照照", 57},
{"能", 58},
{"能能", 59},
{"衣", 60},
{"衣架", 61},
{"衣衣", 62},
{"闭", 63},
{"闭闭", 64},
{"降", 65},
{"风", 66},
{"#0", 67},
{NULL, 0}};

#pragma GCC diagnostic pop

