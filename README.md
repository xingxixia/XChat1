# XChat1

XChat1 是一个基于 Qt6 Widgets 的桌面 AI 角色陪伴项目。它以 Galgame 式“立绘 + 对话框”为主要交互形式，目标是做一个轻量、可自定义、适合陪伴聊天和辅助学习代码的桌面角色。

本项目基于 [ZcChat2](https://github.com/Zao-chen/ZcChat2) 进行二次开发，原项目作者为 Zao-chen，原项目采用 GPL-3.0 license。本仓库不是原作者官方版本。

## 项目方向

XChat1 更关注一个简单直接的桌面陪伴体验：

- 桌面上显示角色立绘
- 通过对话框和角色聊天
- 支持配置大模型 API
- 支持角色立绘、基础语音合成和动画插件
- 优化聊天框显示、输入体验和文本样式

相比做成复杂工具平台，本项目目前更偏向“Galgame 风格桌面 AI 角色”。

## 本版本主要修改

本仓库在 ZcChat2 基础上做了以下二次开发：

- 启动时默认只显示立绘，不自动弹出聊天框
- 修正设置窗口作为聊天框子窗口时可能出现的越界、置顶和关闭问题
- 修正 API 配置刷新逻辑，根据当前服务商重新读取 API Key 和模型配置
- 将聊天框中的用户输入和角色回复显示拆分，避免 Enter 误发送角色回复
- 支持回复态直接打字切回输入框
- 支持 Esc 隐藏聊天框
- 修正历史回滚时文本不可见或显示错位的问题
- 新增文本设置页，支持字体、字号和文本框主题
- 增加浅色/深色透明文本框和 Galgame 风格文本框
- 调整立绘缩放和动画显示逻辑，减少抖动和边缘异常
- 新增一个测试用 Q 版角色立绘

详细修改记录见：

- [DEV_LOG_2026-05-15.md](DEV_LOG_2026-05-15.md)
- [HANDOFF_2026-05-15_DIALOG_SPLIT.md](HANDOFF_2026-05-15_DIALOG_SPLIT.md)

## AI 辅助开发与素材说明

本项目二次开发过程由 AI 辅助完成，包括代码阅读、问题定位、修改建议、日志整理和部分代码实现。

新增测试角色立绘素材由 ChatGPT Image 2 生成，并作为测试素材放入默认角色目录。

详细说明见：

- [AI_DISCLOSURE.md](AI_DISCLOSURE.md)

## 构建环境

当前本地验证环境：

- Windows
- Qt 6.8.3 msvc2022_64
- Visual Studio 2022 Community
- CMake
- Release 构建

参考构建命令：

```powershell
cd E:\File\AIuseing\xai\ZcChat2
& cmd.exe /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build .\build-release --config Release'
```

说明：由于项目自带第三方 DLL 主要为 Release 版本，本地开发建议优先使用 Release 构建。

## 运行配置

程序运行时会读取用户文档目录下的配置：

```text
Documents/ZcChat2
```

默认配置资源位于：

```text
res/default_config/ZcChat2
```

请不要把个人运行时配置中的 API Key 提交到仓库。

## 许可

本项目基于 GPL-3.0 开源项目 ZcChat2 二次开发，因此继续遵守 GPL-3.0 license。

保留内容：

- 原项目 LICENSE
- 原项目来源说明
- 第三方库和原项目资源的既有许可关系

本仓库不声明为 ZcChat2 官方版本。

