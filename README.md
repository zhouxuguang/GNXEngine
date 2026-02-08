# GNXEngine<br>
轻量级游戏引擎<br>
主要介绍：<br>
（1）	使用C/C++开发、CMAKE构建系统、跨平台，目前支持Mac和Windows；<br>
（2）	跨平台RHI，图形API兼容Metal和Vulkan，并预留了其它图形API的接入能力；<br>
（3）	自主构建基础设施，例如多线程、线程池、时间、日期、日志、字符串常用功能等；<br>
（4）	自主开发3D数学库，例如向量、矩阵、四元数等；<br>
（5）	使用Assimp导入静态网格和蒙皮网格以及动画资源，并修复了相关的bug；<br>
（6）	纹理加载：支持PNG/JPEG/TGA/KTX1等格式的导入；<br>
（7）	网格使用PBR/IBL光照模型进行渲染；<br>
（8）	动画系统：动画姿态插值、CPU蒙皮和GPU蒙皮、并优化了插值后姿态从局部到全局变换的转换；<br>
（9）	Shader前端使用HLSL：通过DXCompiler转换为Spirv，再通过Spirv-Cross转换为各个后端图形API的shader语言；<br>
（10）	使用传统的Entity-Component模式进行上层业务构造；<br>
<br>
编译方法：在根目录运行cmake命令即可，没有任何依赖，第三方库都已放在仓库统一管理。<br>

