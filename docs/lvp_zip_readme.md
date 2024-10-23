## 1.output 文件夹说明

```shell
$ tree output/
output/
├── bootx                                       # Linux 烧录工具
├── download.sh                                 # Linux 烧录脚本
├── GX8003A_module_User's Guide.pdf             # 模组使用说明文档
├── ... 省略亿行
├── mcu_nor.bin                                 # 工程编译出的固件
└── README.md                                   # 说明文档

```

---

## 2.模组说明

见此文件夹下的PDF文档：GX8003A_module_User's Guide.pdf

## 3.烧录

### 3.1.Linux

在 output 文件夹内使用 download.sh 脚本即可，命令如下
```shell
./download.sh 0 -r 1000000
```

### 3.2.Windows

- 烧录说明见链接[https://nationalchip.gitlab.io/ai_audio_docs/software/tools/NCDownloader/]

---

## 4.版本信息
commit id:
