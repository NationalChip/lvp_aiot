/* LVP
 * Copyright (C) 2001-2024 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 *tts_player.h
 *
 */

#ifndef __UART_LOADER_H__
#define __UART_LOADER_H__

typedef enum {
    LOADER_SUCCESS,          ///< 操作成功
    LOADER_ERROR_PARAM,      ///< 参数异常
    LOADER_ERROR_HANDSHAKE,  ///< 握手失败
    LOADER_ERROR_RDREV,      ///< 读取设备版本失败
    LOADER_ERROR_UARTRUN,    ///< 设置UART运行模式失败
    LOADER_ERROR_UARTBIN,    ///< 发送固件数据块失败
    LOADER_ERROR_CHECKSUM,   ///< 校验和发送失败
} LOADER_STATUS;

/**
 * @brief 通过UART加载8303固件
 *
 * 该函数通过UART接口将固件数据加载到8303设备中。加载过程包括握手、读取设备版本、
 * 设置UART运行模式、发送固件数据块以及校验和发送。函数在加载过程中会进行错误处理，
 * 并在发生错误时返回相应的错误状态。
 *
 * @param bin_data: 指向8303固件数据的指针
 * @param bin_data_size: 固件数据的长度（字节数）
 * @param handshake_timeout_s: 握手超时时间，单位为秒。如果在此时间内未成功握手，函数将退出并返回错误状态
 *
 * @retval LOADER_STATUS 类型值，表示操作的结果
 *         - LOADER_SUCCESS: 操作成功
 *         - LOADER_ERROR_HANDSHAKE: 握手失败
 *         - LOADER_ERROR_RDREV: 读取设备版本失败
 *         - LOADER_ERROR_UARTRUN: 设置UART运行模式失败
 *         - LOADER_ERROR_UARTBIN: 发送固件数据块失败
 *         - LOADER_ERROR_CHECKSUM: 校验和发送失败
 */
LOADER_STATUS LvpUartLoader(unsigned char *bin_data, unsigned int bin_data_size, unsigned long handshake_timeout_s);

#endif /* __UART_LOADER_H__ */