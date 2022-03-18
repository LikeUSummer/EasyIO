## 编译构建

### 环境依赖

- linux 系统

- gcc 工具链

- openssl 开发库

### 用 gn 生成 ninja 编译描述文件

仓内已经预置了 gn 和 ninja 工具（linux 64位版），直接执行如下命令：

```shell
./build/gn gen -C out
```

### 用 ninja 编译所需的应用/测试模块

为了演示 EasyIO 框架的用法，在 usage 目录下给出了一些示例应用程序，对应的可执行模块名请查看根目录下的 BUILD.gn 文件，例如编译其中的 HTTP 多线程下载器`http_downloader`，执行以下命令：

```shell
 ./build/ninja -C out http_downloader
```

如果需要构建 usage 目录下的所有演示程序，则执行：

```shell
./build/ninja -C out
```

构建出的可执行文件都在 out 目录中。
