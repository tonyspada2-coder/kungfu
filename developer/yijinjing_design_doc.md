---

## **`yijinjing` 模块设计文档 (深度解析版)**

### 1. 概述与设计哲学

#### 1.1 项目定位

`yijinjing` 是一个专为金融交易等超低延迟场景设计的进程间通信（IPC）和数据回放引擎。其核心目标是在保证数据时序性的前提下，实现纳秒级的消息传递延迟和极高的吞吐量。它并非一个通用的消息队列，而是针对特定领域（单机、多进程协作、数据持久化）的极致优化解决方案。

#### 1.2 设计哲学

*   **延迟高于一切**：所有设计决策都优先考虑如何降低端到端延迟。这包括避免系统调用、锁、动态内存分配以及任何可能导致CPU上下文切换的操作。
*   **写入者优先**：在资源竞争时，优先保证写入者的速度，允许其覆盖尚未被读取的数据。这符合金融行情“最新数据最重要”的业务特性。
*   **显式优于隐式**：系统的性能依赖于正确的使用方式，例如CPU亲和性设置、忙等轮询等。它将这些选择权交给开发者，而不是在内部隐藏复杂性。
*   **数据即日志**：所有写入的数据都被持久化为日志文件（Journal），这不仅是通信的媒介，也是系统状态恢复和历史回放的基石。

---

### 2. 顶层API接口设计

`yijinjing`的API设计简洁，围绕“位置”、“读取”和“写入”三个核心操作展开。

#### 2.1 `struct location` (位置)

`location`是`yijinjing`的资源标识符，定义了Journal的存储位置和元数据。它通过URI形式表示，统一了对不同存储媒介（如共享内存、文件）的访问方式。

*   **URI格式**: `protocol://group/name/mode`
    *   `protocol`: `ipc`, `shm`, `file` 等，定义了访问模式。`ipc`为标准的基于`mmap`的共享内存模式。
    *   `group`: Journal分组，用于逻辑隔离。
    *   `name`: Journal的唯一名称。
    *   `mode`: `live` (实时), `data` (数据) 等。

#### 2.2 `class reader` (读取者)

`reader`负责从一个或多个Journals中按时间顺序读取数据。

```cpp
// 核心API伪代码
class reader {
public:
    // 订阅一个Journal。from_time指定从何时开始读取（0表示从头）。
    void join(const location_ptr& loc, uint64_t from_time);

    // 阻塞或非阻塞地获取下一条消息（帧）。
    // 返回一个frame_ptr，它是一个指向共享内存的轻量级指针。
    frame_ptr next();

    // 将读取指针移动到指定时间点之后的第一条消息。
    void seek_to_time(uint64_t nanotime);

    // 清除所有订阅的Journal。
    void leave();
};
```
**设计亮点**: `reader`可以同时`join`多个Journals，`next()`方法会自动按时间戳合并所有来源的数据，返回全局有序的下一帧。

#### 2.3 `class writer` (写入者)

`writer`负责向单个Journal写入数据。

```cpp
// 核心API伪代码
class writer {
public:
    // 构造函数，绑定到一个特定的Journal位置。
    writer(const location_ptr& loc);

    // 1. 开启一个帧（声明写入意图）
    // 返回一个指向数据区的裸指针，供用户填充数据。
    void* open_frame(int64_t data_length, int32_t msg_type);

    // 2. 关闭帧（提交数据）
    // 只有调用此方法后，数据才对reader可见。
    void close_frame(int64_t written_length);

    // 一个便捷的组合API，封装了open/copy/close。
    void write(const void* data, int64_t length, int32_t msg_type);
};
```
**设计亮点**: `open_frame`/`close_frame`两阶段提交模式允许“零拷贝”写入。用户可以直接在共享内存缓冲区中构建复杂对象，避免了数据从用户缓冲区到IPC缓冲区的额外拷贝。

---

### 3. 核心内存布局

`yijinjing`的性能基石是其内存布局。每个Journal由一个或多个内存页（通常对应一个文件）组成，每个页结构如下：

#### 3.1 `page_header`

位于每页的起始位置，是实现无锁并发控制的关键。

```c
struct page_header {
    volatile uint32_t frame_count;      // 页面中已提交的帧数量
    volatile int64_t  last_pos;         // 页面中最后一个帧的结束位置
    int64_t           begin_time;       // 页面第一帧的时间戳
    int64_t           end_time;         // 页面最后一帧的时间戳
    // ... 其他元数据
};
```
*   `last_pos`: **写入者通过原子操作（CAS）更新此值**，用于在多写者场景下声明写入空间，是无锁写入的核心。

#### 3.2 `frame` (帧)

帧是最小的原子消息单元，由`frame_header`和`data`组成。

```c
struct frame_header {
    volatile int64_t length;        // 帧总长度（Header + Data），关键状态字段
    uint64_t         gen_time;      // 消息生成时间戳 (ns)
    int32_t          msg_type;      // 消息类型
    uint32_t         source;        // 消息源ID
    // ... 其他业务字段
};
```
*   `length`: 这是一个特殊字段。
    *   **写入时**: 写入者在数据完全写入后，**最后一步原子地写入一个正值的`length`**。
    *   **读取时**: 读取者通过检查`length`是否为正值来判断该帧是否已完全写入。一个非正值（通常为0）表示该帧区域为空或正在写入。

---

### 4. 详细实现原理

#### 4.1 写入操作 (Lock-Free Write)

1.  **空间声明 (Claim)**:
    *   写入者读取当前页的`page_header->last_pos`。
    *   使用`__sync_fetch_and_add`或`std::atomic::fetch_add`等原子指令，将`last_pos`增加自己需要的帧长度。这个操作会返回增加前的旧值，该值就是写入者获得的写入起始位置。
    *   由于`fetch_and_add`是原子的，即使多个写入者同时执行，它们也会获得连续且不重叠的内存空间。

2.  **数据写入**:
    *   写入者首先将`frame_header->length`设置为一个**负值**或0，标记此帧为“正在写入”状态。
    *   然后，将消息体(`data`)和其他`frame_header`字段（如`gen_time`, `msg_type`）写入。

3.  **提交 (Commit)**:
    *   确保所有数据都已写入共享内存后，执行一个**写内存屏障（Store Memory Barrier）**。这确保了CPU和编译器不会将此操作重排到数据写入之前。
    *   最后，**原子地将`frame_header->length`写入为正的实际帧长度**。
    *   一旦`length`变为正值，该帧就对所有`reader`可见了。

#### 4.2 读取操作 (Wait-Free Read)

1.  **轮询检查 (Poll)**:
    *   `reader`维护一个指向当前读取位置的指针`cursor`。
    *   它循环地读取`cursor`指向的`frame_header->length`。

2.  **数据消费**:
    *   如果`length`为0，表示已读到日志末尾，`reader`进入忙等（Busy-Wait）或短暂休眠，等待新数据。
    *   如果`length`为正值，表示这是一个完整的、可读的帧。
    *   执行一个**读内存屏障（Load Memory Barrier）**，确保在读取`length`之后才读取帧的其他内容。
    *   读取`frame_header`和`data`。

3.  **指针推进**:
    *   处理完数据后，`reader`将`cursor`向后移动`length`个字节，指向下一帧的起始位置，开始新一轮的轮询。

这个模型中，`reader`从不修改共享数据，因此多个`reader`可以并发读取而无需任何同步。

---

### 5. Corner Case 处理

*   **写入者崩溃**:
    *   **场景**: 写入者在写入`length`为负值后、写入正值前崩溃。
    *   **后果**: 产生一个“僵尸帧”，`reader`会永远在此处等待。
    *   **检测与恢复**: `yijinjing`的守护进程（`master`）会周期性扫描Journals。如果一个帧的`length`长时间为负值，且其`gen_time`远早于当前时间，就会被判定为僵尸帧。`master`会将其`length`修正为0或一个特殊的负值，并修复`page_header->last_pos`，让`reader`可以跳过它。

*   **环形覆盖 (Journal Full)**:
    *   当`last_pos`到达页尾时，写入者会从页头开始继续写入，覆盖最旧的数据。
    *   这是设计决策，不是Bug。它保证了写入永远不会被阻塞。`reader`有责任及时消费数据，否则数据会丢失。这符合行情处理的业务需求。

*   **跨页写入**:
    *   当一个帧无法在当前页的剩余空间中完整存放时，写入者会在页尾写入一个特殊的`PAGE_END`帧。
    *   此帧的`msg_type`为特殊值，其数据区包含下一页（下一个文件）的信息。
    *   `reader`读到`PAGE_END`帧后，会自动关闭当前内存映射，打开并映射新的Journal文件，无缝衔接读取过程。

---

### 6. 设计考虑与注意事项

*   **性能调优**:
    *   **CPU亲和性**: 为获得极致性能，读写线程应绑定到不同的物理CPU核心，避免争抢CPU资源和缓存。
    *   **忙等 vs. 休眠**: `yijinjing`默认使用忙等，以牺牲CPU换取最低延迟。但在非极端场景下，可配置为短暂`yield`或`sleep`，以降低CPU占用率。
    *   **缓存行对齐**: `page_header`和`frame_header`都经过精心设计，确保高频访问的原子变量（如`last_pos`和`length`）位于独立的缓存行，避免伪共享。

*   **数据持久化与一致性**:
    *   `mmap`依赖操作系统的延迟刷盘机制，断电可能导致少量数据丢失。
    *   如果需要强一致性，应用层可以在写入关键消息后手动调用`msync`，但这会带来显著的性能开销，违背了`yijinjing`的设计初衷。

*   **适用场景**:
    *   **适合**: 单机内多进程、低延迟、高吞吐量的消息广播，如交易系统的行情分发、订单路由、状态同步。
    *   **不适合**: 需要高可靠、跨网络、支持复杂查询的通用消息队列场景。请为此选择Kafka, RabbitMQ等工具。
