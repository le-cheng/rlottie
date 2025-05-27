# rlottie Qt渲染后端示例程序

本项目已经完全移植到Qt平台，并创建了多个示例程序来展示Qt渲染后端的功能。

## 🎯 **示例程序列表**

### 1. **qtplayer** - 文件夹浏览播放器 ✅
**功能**: 
- 文件夹选择和JSON文件列表显示
- 点击文件名即时播放动画
- 左右分栏界面，支持多文件批量预览
- 完整的播放控制功能

**特点**:
- 使用Qt渲染后端进行真正的矢量渲染
- 智能文件扫描 (*.json, *.lottie)
- 即点即播，无需手动控制

**使用方法**:
```bash
./qtplayer
```

### 2. **qtperformance** - 性能测试工具 ✅
**功能**:
- 可配置的多动画同时渲染测试
- 实时FPS监控和统计
- 支持1-100个动画实例的性能测试
- 详细的性能报告输出

**特点**:
- 网格式布局显示多个动画
- 实时统计平均FPS和总帧数
- 控制台输出详细测试结果

**使用方法**:
```bash
./qtperformance
```

### 3. **qtviewer** - 文件查看器 ✅
**功能**:
- 自动扫描resource文件夹中的Lottie文件
- 文件列表选择和播放控制
- 实时帧信息显示
- 自适应缩放显示

**特点**:
- 保持动画宽高比的智能缩放
- 文件信息和播放状态显示
- 简洁的播放控制界面

**使用方法**:
```bash
./qtviewer
```

### 4. **qtdemo** - 动态属性演示 ✅
**功能**:
- 展示rlottie的动态属性修改功能
- 多种变换效果的同时演示
- 网格布局对比显示

**特点**:
- 填充颜色动态变化
- 描边属性实时调整
- 变换矩阵动画效果

**使用方法**:
```bash
./qtdemo
```

### 5. **qtdemo_marker** - 标记功能演示 ✅
**功能**:
- 演示Lottie动画标记功能
- 支持从标记开始播放
- 标记到标记的片段播放

**特点**:
- 正确处理marker std::tuple格式
- 支持多种播放模式
- 标记信息显示

**使用方法**:
```bash
./qtdemo_marker
```

## 🛠 **编译和运行**

### 快速启动脚本:
```bash
./run_qt_examples.sh
```

### 编译所有Qt示例程序:
```bash
cd build
make qtplayer qtperformance qtviewer qtdemo qtdemo_marker -j4
```

### 单独编译某个程序:
```bash
make qtplayer          # 编译播放器
make qtperformance     # 编译性能测试
make qtviewer          # 编译查看器
make qtdemo            # 编译动态属性演示
make qtdemo_marker     # 编译标记功能演示
```

### 运行程序:
```bash
cd build/example
LD_LIBRARY_PATH=/path/to/qt/lib:$LD_LIBRARY_PATH ./qtplayer
```

## 📁 **文件结构**

```
example/
├── CMakeLists.txt         # 构建配置文件 ✅
├── qtplayer.cpp           # 文件夹浏览播放器 ✅
├── qtperformance.cpp      # 性能测试工具 ✅
├── qtviewer.cpp           # 文件查看器 ✅
├── qtdemo.cpp             # 动态属性演示 ✅
├── qtdemo_marker.cpp      # 标记功能演示 ✅
├── lottieperf.cpp         # 原始性能测试工具 ✅
├── lottie2gif.cpp         # GIF转换工具 ✅
├── run_qt_examples.sh     # 自动化编译运行脚本 ✅
└── resource/              # 示例文件目录
    └── test.json          # 测试动画文件 ✅
```

## 🎨 **技术特点**

### **Qt渲染后端优势**:
1. **真正的矢量渲染**: 直接使用QPainterPath，无需光栅化
2. **GPU加速**: 利用Qt的硬件加速能力
3. **高质量输出**: 支持抗锯齿和平滑变换
4. **跨平台兼容**: 利用Qt的跨平台特性
5. **内存效率**: 智能的缓冲区管理和清理

### **实现亮点**:
- **VPath到QPainterPath的直接转换**
- **多线程移除**: 简化为单线程操作，适合嵌入式环境
- **内存清理机制**: 自动清理脏数据，确保渲染质量
- **动态渲染后端切换**: 支持运行时选择渲染方式
- **正确的Marker处理**: 使用std::get访问tuple元素

## 🔧 **配置说明**

### CMakeLists.txt配置:
```cmake
# 启用Qt渲染后端
set(LOTTIE_QT ON)

# 示例文件路径
set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource/")
add_definitions(-DDEMO_DIR="${DEMO_DIR}")

# 自动MOC配置
set_target_properties(qtdemo PROPERTIES AUTOMOC ON)
```

### 全局渲染后端配置:
```cpp
// 在main函数中配置
rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

// 或为单个动画配置
animation->setRenderBackend(rlottie::RenderBackend::Qt);
```

## 📊 **性能测试结果**

使用qtperformance工具可以获得详细的性能报告:

```
=== 性能测试结果 ===
动画数量: 25
测试时长: 10.5 秒
总渲染帧数: 6250
平均FPS: 595.2
每个动画平均FPS: 23.8
```

## 🚀 **扩展和定制**

### 添加新的示例程序:
1. 创建新的 `.cpp` 文件
2. 在 `CMakeLists.txt` 中添加编译目标
3. 确保包含必要的Qt头文件和rlottie API

### 自定义动画文件:
1. 将 `.json` 或 `.lottie` 文件放入 `resource/` 目录
2. 重新运行程序，文件会自动被扫描和加载

## 🔧 **故障排除**

### 常见编译问题及解决方案:

1. **DEMO_DIR未定义**:
   - 确保CMakeLists.txt中正确定义了`DEMO_DIR`
   - 检查`add_definitions(-DDEMO_DIR="${DEMO_DIR}")`是否存在

2. **MOC文件未找到**:
   - 确保设置了`AUTOMOC ON`属性
   - 检查`Q_OBJECT`宏是否正确放置

3. **Marker访问错误**:
   - 使用`std::get<0>(marker)`访问名称
   - 使用`std::get<1>(marker)`访问开始帧
   - 使用`std::get<2>(marker)`访问结束帧

4. **Qt库链接问题**:
   - 确保设置正确的`LD_LIBRARY_PATH`
   - 检查Qt5组件是否正确链接

### 编译状态: ✅ 全部成功
- qtplayer: ✅ 编译成功
- qtperformance: ✅ 编译成功  
- qtviewer: ✅ 编译成功
- qtdemo: ✅ 编译成功
- qtdemo_marker: ✅ 编译成功

这些示例程序展示了rlottie Qt渲染后端的强大功能，为开发者提供了完整的参考实现和测试工具。 