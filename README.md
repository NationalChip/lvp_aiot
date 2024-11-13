# lvp_aiot
* lvp_aiot 概述：
    * **lvp_aiot** 全称为 Lower-Power Voice Process AIOT， 是专门为了低功耗、可配置离线语音识别而研发的语音信号处理框架，适用于 **GX8002D** 芯片，与 **viva(https://github.com/NationalChip/viva)** 配套使用，开发者不需要自己训练模型，常规的应用也不需要额外开发，利用**viva**可以轻松实现"0代码"开发。
    * 如果开发者需要部署自己训练的模型，可以使用如下sdk：
        * **lvp_kws(https://github.com/NationalChip/lvp_kws)** 是专门为了低功耗可穿戴设备和语音遥控器等应用而研发的语音信号处理框架，适用于 **GX8002A/GX8002B** 芯片，开发者可以部署自己训练的模型。
* GX8002 是一款专为低功耗领域设计的 **超低功耗 AI 神经网络芯片**，适用于低功耗可穿戴设备和语音遥控器等应用。该芯片具有体积小、功耗低、成本低等显著优势。它集成了杭州国芯微自主研发的第二代神经网络处理器 gxNPU V200，支持 **TensorFlow** 和 **Pytorch** 框架，以及自研的硬件 VAD（语音活动检测），显著降低了功耗。在实际测试场景中，VAD 待机功耗可低至 70uW，运行功耗约为 0.6mW，芯片的平均功耗约为 300uW。
    * [GX8002芯片数据手册](https://nationalchip.gitlab.io/ai_audio_docs/hardware/%E8%8A%AF%E7%89%87%E6%95%B0%E6%8D%AE%E6%89%8B%E5%86%8C/GX8002%E8%8A%AF%E7%89%87%E6%95%B0%E6%8D%AE%E6%89%8B%E5%86%8C/)

## 开发板介绍
* 请阅读：[GX8002_DEV开发板介绍](https://nationalchip.gitlab.io/ai_audio_docs/hardware/%E5%BC%80%E5%8F%91%E6%9D%BF%E7%A1%AC%E4%BB%B6%E5%8F%82%E8%80%83%E8%AE%BE%E8%AE%A1/GX8002/GX8002_DEV%E5%BC%80%E5%8F%91%E6%9D%BF/)，在此页面您可以下载开发板 **硬件规格资料** 和 **硬件设计资料**。

## 快速入门
* 请阅读：[搭建开发环境](https://nationalchip.gitlab.io/ai_audio_docs/software/lvp/SDK%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/SDK%E5%BF%AB%E9%80%9F%E5%85%A5%E9%97%A8/%E6%90%AD%E5%BB%BA%E5%BC%80%E5%8F%91%E7%8E%AF%E5%A2%83/#1-sdk/) 完成编译环境的安装。
* 默认示例编译，默认示例的指令词清单见下文的[默认示例的指令词列表](#默认示例的指令词列表)
    ``` shell
    cp configs/general_asr/grus_gx8002d_general_asr_v103_v055.config .config
    make defconfig
    make clean; make
    ```
* 请阅读 [串口升级](https://nationalchip.gitlab.io/ai_audio_docs/software/lvp/SDK%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/SDK%E5%BF%AB%E9%80%9F%E5%85%A5%E9%97%A8/%E4%B8%B2%E5%8F%A3%E5%8D%87%E7%BA%A7/) 以了解如何将 output/mcu_nor.bin 文件烧录到我们的开发板 (Grus_Dev_V1.4)。
* 请阅读 [viva使用指南](https://github.com/NationalChip/viva/blob/main/README.md) 以了解如何使用 viva 零代码开发lvp_aiot.

## 默认示例的指令词列表
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
