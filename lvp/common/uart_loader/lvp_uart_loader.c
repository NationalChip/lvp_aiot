#include <string.h>

#include <driver/gx_delay.h>
#include <driver/gx_gpio.h>
#include <driver/gx_irq.h>
#include <driver/gx_padmux.h>
#include <driver/gx_rtc.h>
#include <driver/gx_snpu.h>
#include <driver/gx_timer.h>
#include <driver/gx_uart.h>
#include <lvp_audio_in.h>

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
#include <kws_uart_report.h>
#endif

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
#include <lvp_uart_tts_player.h>
#endif

#include "lvp_uart_loader.h"

#define LOG_TAG "[UART_LOADER]"

#ifdef CONFIG_ENABLE_PRINT_SEND_RECV_INFO
    #define DEBUG_PRINT(fmt, args...) printf(fmt, ## args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define LOADER_UART             (0)             // 串口号
#define LOADER_UART_BAUDRATE    (9600)          // 波特率
#define LOADER_BASE_ADDR        (0x1FFF06C0)    // 固件的加载基地址
#define BLOCK_OFFSET            (2048)          // 固件每次发送块大小
#define POWER_GPIO_8303         (1)             // 控制 8303 core 电源引脚
#define HANDSHAKE_TIME_MS       (40)            // 握手间隔时间
#define RECV_TIMEOUT_MS         (60)            // 消息等待超时时间
#define HANDSHAKE_SUCCESS       "cmd>>:"        // 握手成功接收到的数据

// static unsigned char response[50] = {0};

// 带超时的读取函数
static int _read_with_timeout(int port, unsigned char *buffer, size_t size, int timeout_ms)
{
    return gx_uart_read_non_block(port, buffer, size, timeout_ms);
}

// 写入数据
static int _write_to_serial_port(int uart_port, const char *data, size_t size)
{
    int w_len = gx_uart_write(uart_port, (const unsigned char *)data, size);
    if (w_len != size) {
        printf(LOG_TAG "write error\n");
    }
    return w_len;
}

// 发送握手信号
static int _send_handshake(int uart_port)
{
    const char *handshake = "UXTDWU\n";
    DEBUG_PRINT("[Handshake] Send: %s", handshake);
    return _write_to_serial_port(uart_port, handshake, strlen(handshake));
}

// 读取设备版本
static int _read_revision(int uart_port)
{
    const char *cmd = "rdrev+\n";
    DEBUG_PRINT("[Read Revision] Send: %s", cmd);
    return _write_to_serial_port(uart_port, cmd, strlen(cmd));
}

// 设置UART运行模式
static int _uart_run(int uart_port, unsigned int base_addr)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "uartrun 2 %08X \n", base_addr);
    DEBUG_PRINT("[UART Run] Send: %s", cmd);
    return _write_to_serial_port(uart_port, cmd, strlen(cmd));
}

// 发送固件数据块
static int _send_firmware_block(int port, const char *data, size_t size, unsigned int block_offset, unsigned int base_addr)
{
    unsigned char response[50] = {0};
    char cmd[64] = {0};

    snprintf(cmd, sizeof(cmd), "uartbin %u 0 %03x %08X\n", block_offset, (unsigned int)size,
             base_addr + block_offset * (unsigned int)BLOCK_OFFSET);
    DEBUG_PRINT("[Send Firmware Block-%d] Send: %s", block_offset, cmd);

    if (_write_to_serial_port(port, cmd, strlen(cmd)) < 0) {
        return -1;
    }

    // 读取设备响应
    int n = _read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS);
    if (n > 0) {
        response[n] = '\0';
        DEBUG_PRINT("[Send Firmware Block-%d] Received: %s\n", block_offset, response);
    } else if (n == 0) {
        printf("[Send Firmware Block-%d] Read timeout\n", block_offset);
        return -1;  // 超时退出
    } else {
        printf("[Send Firmware Block-%d] Read error\n", block_offset);
        return -1;  // 读取错误退出
    }

    return _write_to_serial_port(port, data, size);
}

// 发送校验和
static int _send_checksum(int port, unsigned int checksum)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%08X\n", checksum);
    DEBUG_PRINT("[Send Checksum] send: %s", cmd);
    return _write_to_serial_port(port, cmd, strlen(cmd));
}

// 计算校验和
static unsigned int _calculate_checksum(const char *data, size_t size)
{
    unsigned int checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        checksum += (unsigned char)data[i];
    }
    return checksum;
}

// 发送固件数据
static LOADER_STATUS _send_firmware(int port, unsigned char *bin_data, unsigned int bin_data_size, unsigned int base_addr)
{
    unsigned char response[50] = {0};
    size_t bytesRead = 0;
    unsigned int block_offset = 0;

    while ((block_offset * BLOCK_OFFSET) < bin_data_size) {
        if ((block_offset + 1) * BLOCK_OFFSET > bin_data_size)
            bytesRead = bin_data_size - block_offset * BLOCK_OFFSET;
        else
            bytesRead = BLOCK_OFFSET;

        const char *buffer = (const char *)(bin_data + block_offset * BLOCK_OFFSET);
        int send_block_len = _send_firmware_block(port, buffer, bytesRead, block_offset, base_addr);
        if (send_block_len < 0) {
            return LOADER_ERROR_UARTBIN;
        }
        DEBUG_PRINT("[Send Firmware Block-%d] Send bin data successful, send len: %d\n", block_offset, send_block_len);

        int response_len = _read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS);
        if (response_len > 0) {
            response[response_len] = '\0';
            DEBUG_PRINT("[Send Firmware Block-%d]  Received: %s\n", block_offset, response);
        } else if (response_len == 0) {
            printf("[Send Firmware Block-%d]  Read timeout\n", block_offset);
            return LOADER_ERROR_UARTBIN;  // 超时退出
        } else {
            printf("[Send Firmware Block-%d]  Read error\n", block_offset);
            return LOADER_ERROR_UARTBIN;  // 读取错误退出
        }

        unsigned int checksum = _calculate_checksum(buffer, bytesRead);
        DEBUG_PRINT("[Send Firmware Block-%d]  mcu calculate checksum: %d(0x%x)\n", block_offset, checksum, checksum);
        if (_send_checksum(port, checksum) < 0) {
            return LOADER_ERROR_CHECKSUM;
        }

        // 读取设备响应
        response_len = 0;
        response_len = _read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS);
        if (response_len > 0) {
            response[response_len] = '\0';
            DEBUG_PRINT("[Send Firmware Block-%d]  Received: %s\n", block_offset, response);
        } else if (response_len == 0) {
            printf("[Send Firmware Block-%d]  Read timeout\n", block_offset);
            return LOADER_ERROR_UARTBIN;  // 超时退出
        } else {
            printf("[Send Firmware Block-%d]  Read error\n", block_offset);
            return LOADER_ERROR_UARTBIN;  // 读取错误退出
        }

        block_offset++;
    }

    return LOADER_SUCCESS;
}

#ifdef CONFIG_ENABLE_RECV_8303_UART_DATA
static int _recv_8303_callback(int port, int length, void *priv)
{
    if (gx_uart_can_read(port) == 1) {
        if (_read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS) > 0) {
            printf("[await] Received: %s", response);
        }
    }

    return 0;
}
#endif

LOADER_STATUS LvpUartLoader(unsigned char *bin_data, unsigned int bin_data_size, unsigned long handshake_timeout_s)
{
    if (bin_data == NULL || bin_data_size <= 0 || handshake_timeout_s <= 0)
        return LOADER_ERROR_PARAM;

    LvpAudioInSuspend();
    while (gx_snpu_get_state() == GX_SNPU_BUSY);
    gx_snpu_pause();
    unsigned int irq_state = gx_lock_irq_save();
    gx_rtc_init();

    int port = LOADER_UART;
    int baudrate = LOADER_UART_BAUDRATE;
    unsigned int base_addr = LOADER_BASE_ADDR;
    int response_len = 0;
    LOADER_STATUS status = LOADER_SUCCESS;
    unsigned char response[50] = {0};

    if (gx_uart_exit(port) != 0) {
        printf(LOG_TAG "loader uart init fail!\n");
        status = LOADER_ERROR_HANDSHAKE;
        goto error_return;
    }

    if (gx_uart_init(port, baudrate) != 0) {
        printf(LOG_TAG "loader uart init fail!\n");
        status = LOADER_ERROR_HANDSHAKE;
        goto error_return;
    }

    padmux_set(POWER_GPIO_8303, POWER_GPIO_8303 == 2 ? 0 : 1);
    gx_gpio_set_direction(POWER_GPIO_8303, GX_GPIO_DIRECTION_OUTPUT);
    // 8303 power off
    gx_gpio_set_level(POWER_GPIO_8303, GX_GPIO_LEVEL_HIGH);
    gx_mdelay(20);

    DEBUG_PRINT("[Handshake]\n");
    unsigned long start_time, current_time;
    gx_rtc_get_tick(&start_time);

    do {
        if (_send_handshake(port) < 0) {
            status = LOADER_ERROR_HANDSHAKE;
            goto error_return;
        }
        // 8303 power on
        gx_gpio_set_level(POWER_GPIO_8303, GX_GPIO_LEVEL_LOW);
        // 根据抓包握手每 40ms 一次
        gx_mdelay(HANDSHAKE_TIME_MS);

        if (gx_uart_can_read(port) == 1) {
            response_len = _read_with_timeout(port, response, sizeof(response) - 1, HANDSHAKE_TIME_MS);
            if ((response_len > 2) && strstr((const char *)response, HANDSHAKE_SUCCESS)) {
                response[response_len] = '\0';
                DEBUG_PRINT("[Handshake] Received: %s\n", response);
            } else {
                printf("[Handshake] Read error Restart 8303\n");
                response_len = 0;
                // 8303 power off
                gx_gpio_set_level(POWER_GPIO_8303, GX_GPIO_LEVEL_HIGH);
                gx_mdelay(HANDSHAKE_TIME_MS);
            }
        }

        gx_rtc_get_tick(&current_time);
        if ((current_time - start_time) >= handshake_timeout_s) {
            printf("[Handshake] Timeout\n");
            status = LOADER_ERROR_HANDSHAKE;
            goto error_return;
        }
    } while (response_len == 0);

    if (gx_uart_init(port, 115200) != 0) {
        printf(LOG_TAG "loader uart switching baud rate fail!\n");
        status = LOADER_ERROR_HANDSHAKE;
        goto error_return;
    }

    DEBUG_PRINT("[Read Revision]\n");
    if (_read_revision(port) < 0) {
        status = LOADER_ERROR_RDREV;
        goto error_return;
    }
    response_len = _read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS);
    if (response_len > 0) {
        response[response_len] = '\0';
        DEBUG_PRINT("[Read Revision] Received: %s\n", response);
    } else if (response_len == 0) {
        printf("[Read Revision] Read timeout, [%d]\n", response_len);
    } else {
        printf("[Read Revision] Read error, [%d]\n", response_len);
        status = LOADER_ERROR_RDREV;
        goto error_return;
    }

    DEBUG_PRINT("[UART Run]\n");
    if (_uart_run(port, base_addr) < 0) {
        status = LOADER_ERROR_UARTRUN;
        goto error_return;
    }
    response_len = _read_with_timeout(port, response, sizeof(response) - 1, RECV_TIMEOUT_MS);
    if (response_len > 0) {
        response[response_len] = '\0';
        DEBUG_PRINT("[UART Run] Received: %s\n", response);
    } else if (response_len == 0) {
        printf("[UART Run] Read timeout\n");
        status = LOADER_ERROR_UARTRUN;
        goto error_return;
    } else {
        printf("[UART Run] Read error\n");
        status = LOADER_ERROR_UARTRUN;
        goto error_return;
    }

    DEBUG_PRINT("[Send Firmware]\n");
    status = _send_firmware(port, bin_data, bin_data_size, base_addr);
    if (status != LOADER_SUCCESS) {
        goto error_return;
    }

    // 清空当前串口里残留的数据，防止在使用串口时进行后续初始化
    gx_uart_flush_recv_fifo(port);

    DEBUG_PRINT(LOG_TAG "Firmware sent successfully\n");
    status = LOADER_SUCCESS;

#ifdef CONFIG_ENABLE_RECV_8303_UART_DATA
    gx_uart_start_async_recv(port, _recv_8303_callback, NULL);
#endif

error_return:
#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
    if (port == CONFIG_KWS_UART_REPORT_UART)
        LvpKwsUartReportInit();
#endif
#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
    if (port == CONFIG_REPLY_PLAYER_UART)
        LvpUartTtsPlayerInit();
#endif
    gx_unlock_irq_restore(irq_state);
    LvpAudioInResume();
    gx_snpu_resume();

    return status;
}
