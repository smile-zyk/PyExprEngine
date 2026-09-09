# Postprocess 多实例 + JSON-RPC 2.0

> 状态：待 review，未实现

## 1. 架构

```
demo (host)
  LauncherWidget
    └─ PostprocessManager            ← 主程序调用的库
         └─ PostprocessInstance × N
              ├─ QProcess            启动子进程
              ├─ Transport           local socket
              └─ JsonRpcPeer         发请求 / 收响应
                    │
                    │  JSON-RPC 2.0
                    ▼
postprocess (子进程) × N
  PostprocessWidget                  ← 原 demo_widget
    ▲
    └─ PostprocessRpcService
         ├─ Transport             与 host 侧配对
         └─ JsonRpcPeer             注册方法 / 收请求
```

两个可执行文件：

| 目标 | 说明 |
|---|---|
| `postprocess` | 后处理程序，可独立运行 / RPC 子进程 / MCP server |
| `demo` | 主程序，只负责启动和管理 |

## 2. 决策

| 项 | 结论 |
|---|---|
| 传输 | `Transport` 抽象（消息级），**local socket** |
| 分帧 | 换行分隔 JSON（`QJsonDocument::Compact` + `\n`），仅流传输用 |
| JSON 库 | Qt（`QJsonDocument`）；core 继续用 boost::json，两者不碰面 |
| 实例标识 | `QUuid`，`Launch()` 返回；失败为 null `QUuid()` |
| 方法命名 | `module.verb`，如 `dataset.add` / `window.raise` |
| host 侧 API | 只有 `Call(id, method, params, cb)`，**不做语法糖** |
| 方法元数据 | `MethodSpec` **必做**（MCP `tools/list` 需要） |

## 3. 分层

```
methods/*           dataset.* / window.* / ...        业务
    ↓
JsonRpcPeer         注册方法 / 匹配响应 / 错误码        协议语义
    ↓
Transport           Send(QJsonObject) / MessageReceived  消息收发
    ├─ StreamTransport   QIODevice + 换行分帧（基类）
    │    └─ LocalSocketTransport
    └─ HttpTransport     MCP Streamable HTTP（无分帧）
```

`Transport` 是**消息级**接口，不暴露 `QIODevice` —— MCP 的 HTTP 传输没有持久
双向字节流，暴露 `QIODevice` 会把它排除在外。

```cpp
class Transport : public QObject {
    Q_OBJECT
public:
    virtual bool Start(QString *error) = 0;
    virtual void Close() = 0;
    virtual void Send(const QJsonObject &message) = 0;
    virtual bool isConnected() const = 0;
    virtual QString Describe() const = 0;      // 日志用

Q_SIGNALS:
    void Connected();
    void MessageReceived(const QJsonObject &message);
    void Disconnected();      // 归一化"对端没了"
    void Error(const QString &text);
};
```

`Disconnected()` 把各传输不同的死亡检测归一化：

| 传输 | 对端死亡信号 |
|---|---|
| local socket | `QLocalSocket::disconnected` |

## 4. 传输实现

| 传输 | host 侧 | child 侧 | 命令行 |
|---|---|---|---|
| `LocalSocketTransport` | `QLocalServer::listen` | `QLocalSocket::connectToServer` | `--channel <name>` |

选 local socket 而非子进程 stdin/stdout：

- `QLocalSocket` 是真正的顺序设备，`readyRead` 正常，不需要 reader thread
  （`QFile(stdin)` 非顺序设备，无 `readyRead`，且 `bytesAvailable()` 对管道
  返回垃圾值会让 `readAll()` 阻塞 GUI 线程）
- `disconnected` 是可靠的"对端没了"信号（不像 `QProcess::readChannelFinished`
  在 Windows 上每次读到 0 字节都误报，会把活着的连接拆掉）
- 子进程 stdout 完全空闲，Python 插件 `print()` 不会污染协议流

代价：建连多几十行（生成管道名 → argv 传给子进程 → host `listen` →
子进程 `connect`）。

**可测试性**：`LoopbackTransport`（一对 `QLocalSocket`）可不启进程单测整个协议层。

## 5. 文件

```
demo/
├── postprocess_widget.{h,cc}      原 demo_widget，改名 + 程序化 API
├── postprocess_main.cc            postprocess 入口（独立 / RPC / MCP）
├── launcher_widget.{h,cc}         主程序 UI
├── launcher_main.cc               demo 入口
│
├── rpc/                           协议层（两进程共用）
│   ├── json_rpc_message.{h,cc}    编解码、错误码
│   ├── json_rpc_peer.{h,cc}       对称 peer：注册方法 + 匹配响应
│   ├── params.h                   Params + JsonTraits<T>
│   ├── method_spec.h              MethodSpec（MCP tools/list 需要）
│   ├── transport.h                Transport 接口
│   ├── stream_transport.{h,cc}    QIODevice + 换行分帧（基类）
│   ├── local_socket_transport.{h,cc}
│   ├── http_transport.{h,cc}      MCP Streamable HTTP
│   ├── postprocess_rpc_service.{h,cc}
│   ├── mcp_service.{h,cc}         MCP 适配层
│   └── methods/
│       ├── window_methods.{h,cc}      window.*
│       ├── equation_methods.{h,cc}    equation.* + expression.*
│       ├── dataset_methods.{h,cc}     dataset.*
│       └── project_methods.{h,cc}     project.*
│
└── host/                          主程序调用的库
    ├── launch_config.{h,cc}
    ├── postprocess_instance.{h,cc}
    └── postprocess_manager.{h,cc}
```

## 6. 协议

### 方法

| 方法 | 参数 | 返回 |
|---|---|---|
| `system.ping` | — | `{pong, pid}` |
| `system.list_methods` | — | `{methods[], groups{}}` |
| `system.shutdown` | — | `{ok}` |
| `window.raise` | — | `{ok}` |
| `window.set_status` | `{text}` | `{ok}` |
| `window.set_title` | `{title}` | `{ok}` |
| `project.get_state` | — | `{title, datasets[], default_dataset, equations[]}` |
| `project.load` | `{path}` | `{ok, datasets, default_dataset, equations}` |
| `equation.add` | `{name, content, tag?, redefine?}` | `{ok, id, name}` |
| `expression.add` | `{content, tag?}` | `{ok, id}` |
| `dataset.add` | `{name, path, format?, make_default?}` | `{ok, datasets}` |
| `dataset.remove` | `{name}` | `{ok, datasets}` |
| `dataset.set_default` | `{name}` | `{ok, default_dataset}` |

`?` = 可选。

### 通知（child → host）

| 方法 | 参数 | 时机 |
|---|---|---|
| `ready` | `{title, pid}` | 窗口 show 后 |
| `status_changed` | `{text}` | 状态栏变化 |

生命周期级，不加模块前缀。host 提供的方法用 `host.` 前缀。

### 错误码

标准 JSON-RPC 2.0：`-32700` / `-32600` / `-32601` / `-32602` / `-32603`。
应用层错误 `-32000`。

### 消息分类

看 `method` 还是 `result`/`error`，**不看 `id`**（response 也有 id）：

```
有 "method"         → Request（有 id）或 Notification（无 id）
有 "result"/"error" → Response
```

Notification = 没有 `id` 字段的 Request，接收方不回响应。

## 7. 扩展

加方法 = 在对应模块加一行 `RegisterMethod`。host 侧无需改动。

```cpp
peer.RegisterMethod(MethodSpec{
    "dataset.add",
    "Load a dataset file into the REL environment",
    {
        {"name",         ParamType::kString, true,  "",     "dataset name"},
        {"path",         ParamType::kString, true,  "",     "dataset file path"},
        {"format",       ParamType::kString, false, "hdf5", "hdf5 | touchstone"},
        {"make_default", ParamType::kBool,   false, "false","also make it default"},
    },
    [this](rpc::Params p) {
        const QString name   = p.Require<QString>("name");
        const QString format = p.Optional<QString>("format", "hdf5");
        // ...
    }
});
```

- `Require<T>` 缺失/类型不符自动抛 `kInvalidParams`
- 新类型 = 特化 `JsonTraits<T>`
- 新模块 = 新 `methods/xxx_methods.{h,cc}` + service 一行调用
- 分发是扁平 map 查找，前缀只是命名约定
- `MethodSpec` 必填：`tools/list` 要靠它生成 JSON Schema

## 8. 启动参数

`LaunchConfig` 是 `demo_project.json` 的超集，写成临时文件用 `--config` 传：

```json
{
  "version": 1,
  "title": "LNA run",
  "project": "demo/demo_project.json",
  "datasets": [{ "name": "LNA", "format": "hdf5", "path": "../3rd/REL/case/LNA.h5" }],
  "default_dataset": "LNA",
  "equations": [{ "name": "gain", "content": "LNA.amplifier.HB1.HB.Gain", "tag": "Equation" }],
  "expressions": [{ "content": "LNA.amplifier.HB1.HB.Pout", "tag": "Watch" }]
}
```

```
postprocess                                              独立模式
postprocess --rpc --channel <name> [--config <file>]      RPC 子进程
postprocess --mcp --channel <name>                        MCP server
```

## 9. 生命周期

启动：host 生成 uuid + 管道名 `xequation.postprocess.<uuid>` → `listen()` →
写 config → `start()`（带 `--channel <name>`）→
子进程 `connectToServer()` → 建 peer → 子进程发 `ready` →
host flush 排队命令 → `Launch()` 返回 uuid。

停止：`shutdown` → 3s → `terminate()` → 2s → `kill()`。析构直接跳到 terminate/kill。

孤儿防护：host 死 → 子进程 `disconnected` → quit；子进程崩 → host fail 所有 pending。

## 10. Qt 事件循环集成

### 线程模型

**所有对象建在 GUI 线程，零锁。**

```
manager / instance / transport / peer  →  GUI 线程
    → QProcess / QSocket 信号也在 GUI 线程发射
    → handler 和 callback 都在 GUI 线程跑
```

子进程侧尤其重要：handler 直接操作 `PostprocessWidget`，必须在 GUI 线程。

约束（写进代码注释）：**这些对象必须活在有事件循环的线程上，
所有回调在该线程执行。**

### 各侧的信号源

| 侧 | 设备 | 驱动方式 |
|---|---|---|
| host | `QProcess` | `readyRead` / `finished`，事件循环驱动 |
| host | `QLocalSocket` | `readyRead` / `disconnected` |
| child | `QFile(stdin)` | ❌ **无 `readyRead`**，需 reader thread |
| child | `QLocalSocket` | `readyRead` / `disconnected` |

### stdin reader thread

`QFile` 不是顺序设备（`isSequential() == false`），Qt 不发射 `readyRead`。

| 方案 | 可移植性 | 延迟 |
|---|---|---|
| `QSocketNotifier` | ❌ Windows 只支持 socket，不支持管道 | 0 |
| `QTimer` 轮询 | ✅ | 10ms 级 |
| **读线程 + 信号** | ✅ | 0 |

用**读线程**：阻塞读 stdin，读到一行 `emit LineReceived(bytes)`，
GUI 线程通过队列连接（默认）收到。无轮询延迟，跨平台一致。

这是传输的**实现细节**，不污染 `Transport` 接口。

### 异步结果：回调 + 信号

Qt 5.15 没有 `QPromise`（Qt 6 才有），`QFuture` 只能配 `QtConcurrent`，
所以异步结果靠回调。两者都提供：

```cpp
// 1. std::function 回调（默认）
mgr->Call(id, "dataset.add", params, [](bool ok, auto r, auto e) { ... });

// 2. Qt 信号（给偏好 signal/slot 的宿主）
connect(mgr, &PostprocessManager::ResponseReceived,
        this, [](const QUuid &id, const QString &method,
                 const QJsonValue &result, const QString &error) { ... });
```

信号是免费的 —— 回调触发时顺带 `emit` 一次。

### 同步调用

可提供 `CallSync()`（`QEventLoop` + `QTimer` 超时），但**默认不给**：

```cpp
QJsonValue CallSync(const QUuid &id, const QString &method,
                    const QJsonObject &params, int timeout_ms, bool *ok);
```

风险：嵌套事件循环会重入 —— GUI 线程调用期间可能处理别的 RPC，
甚至重入同一个 handler。只在明确需要时（如 CLI 工具）用。

### 关闭

```cpp
connect(qApp, &QCoreApplication::aboutToQuit, mgr, &PostprocessManager::StopAll);
```

子进程侧同理：`aboutToQuit` 时发 `closing` 通知再退出。

## 11. PostprocessWidget

新增程序化 API，GUI 按钮和 RPC 共用：

```cpp
void LoadProjectOrThrow(const QString &path);
void ApplyStartupConfig(const QJsonObject &config, QStringList *errors = nullptr);

ObjectId AddEquation(const QString &name, const QString &content,
                     const QString &tag, bool redefine);
ObjectId AddExpression(const QString &content, const QString &tag);
void AddDataset(const QString &name, const QString &format,
                const QString &path, bool make_default);
void RemoveDataset(const QString &name);
void SetDefaultDataset(const QString &name);

std::vector<std::string> DatasetNames() const;
QString DefaultDatasetName() const;

void RaiseWindow();
void SetStatusText(const QString &text);
```

这些方法**抛异常**，不弹 `QMessageBox`。交互层 `On*()` 槽包 try/catch 弹对话框。

## 12. 用法

```cpp
auto *mgr = new PostprocessManager(this);
mgr->set_executable(PostprocessManager::DefaultExecutablePath());

LaunchConfig cfg = LaunchConfig::FromFile("demo/demo_project.json");
cfg.title = "LNA run";
QUuid a = mgr->Launch(cfg);
cfg.title = "LQ run";
QUuid b = mgr->Launch(cfg);

mgr->Call(a, "equation.add", {
    {"name", "gain"},
    {"content", "LNA.amplifier.HB1.HB.Gain"},
    {"redefine", true},
});
mgr->Call(b, "window.raise", {});
mgr->Call(b, "dataset.set_default", {{"name", "LQ"}});
mgr->Call(a, "dataset.add", {
    {"name", "LQ"}, {"path", "../3rd/REL/case/LQ.s2p"},
    {"format", "touchstone"}, {"make_default", true},
}, [](bool ok, const QJsonValue &r, const QString &e) {
    if (!ok) qWarning() << e;
});

mgr->Call(a, "project.get_state", {}, [](bool ok, const QJsonValue &r, const QString &) {
    if (ok) qDebug() << r.toObject()["datasets"];
});

mgr->RegisterMethod("host.resolve_path", [](rpc::Params p) {
    return QJsonValue(ResolveIt(p.Require<QString>("path")));
});
mgr->OnNotification("status_changed", [](const QUuid &id, const QJsonObject &p) {
    qDebug() << id << p["text"].toString();
});

mgr->Stop(a);
mgr->StopAll();
```

`PostprocessManager` 非 RPC 方法：`Launch` / `Stop` / `StopAll` /
`instance` / `ids` / `count` / `RegisterMethod` / `OnNotification`。

## 13. MCP

MCP 就是 JSON-RPC 2.0，协议层 100% 复用。

| MCP 方法 | 实现 |
|---|---|
| `initialize` | 返回 serverInfo + capabilities |
| `tools/list` | 遍历 `MethodSpec` 表，生成 MCP tool 列表 |
| `tools/call` | `{name, arguments}` → 查表 → 调 handler |
| `resources/list` | 可映射 dataset / equation 列表 |
| `notifications/*` | 走 `JsonRpcPeer::Notify` |
| `sampling/createMessage` | **server → client 的 request**，靠对称 peer |

命名不冲突：MCP 用 `/`（`tools/call`），我们用 `.`（`dataset.add`），
扁平 map 里共存。

MCP 对设计的三个要求：

1. `MethodSpec` 必做 —— `tools/list` 要返回每个 tool 的 JSON Schema
2. 对称 peer 是刚需 —— MCP server 要主动向 client 发 request
3. Transport 用消息级接口 —— HTTP 传输没有持久字节流

## 14. 待确认

1. 主程序还叫 `demo`？还是 `launcher` / `host`？
2. `host/` 单独编译成静态库，还是编进 `demo` 目标？
3. `raise` 在 Windows 上临时置顶会闪，可接受？还是用 `AllowSetForegroundWindow`？
4. 启动参数要不要支持 `python_plugins`？
5. 子进程是否需要主动向 host 发 request（协议层已支持）？
6. 要不要提供方法名常量 `rpc::kMethodEquationAdd`？
7. MCP 是 `postprocess` 的一个模式，还是独立可执行文件？
8. 要不要提供 `CallSync()`（嵌套事件循环有重入风险）？

## 15. 实施

1. `git mv demo_widget.* → postprocess_widget.*`，改类名
2. `PostprocessWidget` 加程序化 API
3. `rpc/`：message → peer
4. `rpc/params.h` + `method_spec.h`
5. `rpc/transport.h` + `stream_transport` + `local_socket_transport`
6. `rpc/methods/` 四个模块 + service 装配
7. `postprocess_main.cc`
8. `host/`：launch_config → instance → manager
9. `launcher_widget` + `launcher_main`
10. `demo/CMakeLists.txt`：两个可执行目标
11. 编译、修错、验证多实例
12. （后续）`http_transport` + `mcp_service`
