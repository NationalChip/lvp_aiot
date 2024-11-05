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
        * 小芯小芯
    * 指令词：
        * 晾杆上升|衣架上升
        * 晾杆下降|衣架下降
        * 停止升降|暂停升降
        * 打开风扇
        * 打开一档
        * 打开二档
        * 打开三档
        * 关闭风扇
        * 播放音乐
        * 上一首
        * 下一首
        * 增大音量
        * 减小音量
        * 停止播放
        * 水壶加热
        * 停止加热
        * 温水模式
        * 沸水模式
        * 泡茶模式
        * 功能全关
        * 打开雾化|开启雾化
        * 关闭雾化|取消雾化
        * 温度十六度
        * 温度十七度
        * 温度十八度
        * 温度十九度

## 固件烧录：
* 请阅读：[串口升级](https://nationalchip.gitlab.io/ai_audio_docs/software/lvp/SDK%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/SDK%E5%BF%AB%E9%80%9F%E5%85%A5%E9%97%A8/%E4%B8%B2%E5%8F%A3%E5%8D%87%E7%BA%A7/)