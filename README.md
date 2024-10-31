# lvp_aiot
低功耗、可配置离线语音识别SDK，配套viva(https://github.com/NationalChip/viva) 可以0代码开发lvp_aiot。

## SDK工具链安装
* 请阅读：[SDK工具链安装](https://nationalchip.gitlab.io/ai_audio_docs/software/lvp/SDK%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/SDK%E5%BF%AB%E9%80%9F%E5%85%A5%E9%97%A8/%E6%90%AD%E5%BB%BA%E5%BC%80%E5%8F%91%E7%8E%AF%E5%A2%83/#1-sdk)

## 默认示例
``` shell
cp configs/general_asr/grus_gx8002d_general_asr_v103_v055.config .config
make defconfig
make clean;make
```
* 默认指令词列表：
    * 唤醒词：
        * 小号小号|小一小一
    * 指令词：
        * 关闭照明|照明关闭
        * 晾杆上升|衣架上升
        * 晾杆下降|衣架下降
        * 停止升降|暂停升降
        * 打开消毒|消毒打开
        * 关闭消毒|消毒关闭
        * 打开吹风
        * 打开冷风
        * 关闭吹风
        * 关闭冷风
        * 打开烘干|烘干打开
        * 关闭烘干|烘干关闭
        * 衣架全关
        * 功能全关
        * 音量增大

## 固件烧录：
* 请阅读：[串口升级](https://nationalchip.gitlab.io/ai_audio_docs/software/lvp/SDK%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/SDK%E5%BF%AB%E9%80%9F%E5%85%A5%E9%97%A8/%E4%B8%B2%E5%8F%A3%E5%8D%87%E7%BA%A7/)