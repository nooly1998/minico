# minico - 轻量级 C 协程库

minico 是一个高性能、跨平台的 C 语言协程库，提供了用户态协程的创建、调度和管理功能。

## 特性

- 🚀 **高性能**：零系统调用的用户态协程切换
- 🔄 **跨平台**：支持 x86_64 和 ARM64 架构
- 🛡️ **安全性**：可选的栈溢出保护和完善的内存管理
- 🧵 **线程池**：内置虚拟线程池支持协程调度
- 📦 **轻量化**：简洁的 API 设计，易于集成

## 系统要求

- **操作系统**：Linux、macOS
- **架构**：x86_64 或 ARM64
- **编译器**：GCC 或 Clang
- **构建工具**：CMake 3.10+

## 快速开始

### 编译安装
```shell
bash git clone <repository-url> 
cd minico 
mkdir build && cd build cmake .. make

```

### 基本使用

```c
#include "co.h" 
#include <stdio.h>
static void hello_coroutine(void *arg) 
{ 
  const char *name = (const char *)arg; 
  for (int i = 0; i < 3; i++) 
  { 
    printf("Hello from %s, iteration %d\n", name, i); 
    co_yield(); // 暂停协程，让出执行权 
  } 
}
int main() 
{ 
  // 创建协程 
  coroutine_t *co1 = co_create(hello_coroutine, "协程1", 0); 
  coroutine_t *co2 = co_create(hello_coroutine, "协程2", 0);
  
  // 交替执行协程
  while (co1->state != DEAD || co2->state != DEAD) {
    if (co1->state != DEAD) co_resume(co1);
    if (co2->state != DEAD) co_resume(co2);
  }

  // 清理资源
  co_destroy(co1);
  co_destroy(co2);

  return 0;

}
```
## API 参考

### 核心协程 API

#### `co_create()`
```c
coroutine_t* co_create(void (_entry)(void_), void* arg, size_t stack_sz);
```
创建一个新的协程。
**参数**：
- `entry`：协程入口函数
- `arg`：传递给协程的参数
- `stack_sz`：栈大小（0 表示使用默认值 64KB）
**返回值**：协程对象指针，失败时返回 NULL
#### `co_resume()`
```c
void co_resume(coroutine_t* co);
```
恢复协程的执行。
#### `co_yield()`
```c
void co_yield(void);
```
暂停当前协程的执行，将控制权交还给调用者。
#### `co_destroy()`
```c
void co_destroy(coroutine_t* co);
```

```c
typedef enum {
  NEW,      // 新创建，未开始执行
  RUNNING,  // 正在执行
  SUSPEND,  // 已暂停
  DEAD      // 已结束
} coroutine_state_t;
```

### 虚拟线程池 API
#### `vtp_init()`
```c
void vtp_init(vtp_t* pool, size_t num_threads);
```
初始化虚拟线程池。
#### `vtp_submit()`
```c
void vtp_submit(vtp_t* pool, void (*entry)(void*), void* arg, size_t stack_sz);
```
向线程池提交协程任务。
#### `vtp_shutdown()`
```c
void vtp_shutdown(vtp_t* pool);
```
关闭线程池并等待所有任务完成。
## 使用场景
### 高并发网络服务
```c
// 处理每个客户端连接的协程
static void handle_client(void* arg) {
    int client_fd = *(int*)arg;
    // 处理客户端请求
    while (1) {
        // 读取数据
        // 处理请求
        // 发送响应
        co_yield();  // 让出 CPU 给其他协程
    }
}
```

### 异步任务处理
```c
// 异步任务协程
static void async_task(void* arg) {
    task_t* task = (task_t*)arg;
    // 执行任务
    process_task(task);
    co_yield();  // 任务完成后暂停
}
```
## 性能特点
- **零系统调用切换**：协程切换完全在用户态完成，避免内核态切换开销
- **最小化内存占用**：每个协程仅需要独立的栈空间
- **高效的上下文保存**：仅保存必要的寄存器状态
- **无锁设计**：协程调度器采用无锁设计，减少同步开销

## 线程安全
- 协程本身是线程安全的，可以在多线程环境中使用
- 虚拟线程池内部使用互斥锁保证线程安全
- 建议在单线程内管理协程的生命周期

## 注意事项
1. **栈大小**：默认协程栈大小为 64KB，可根据需要调整
2. **栈溢出**：在 Linux 上会自动启用栈保护页面
3. **内存管理**：确保及时调用 `co_destroy()` 释放协程资源
4. **异常处理**：协程内部的异常不会自动传播到调用者
## 许可证
本项目采用 MIT 许可证，详见 LICENSE 文件。
## 贡献
欢迎提交 Issue 和 Pull Request 来改进这个项目。
## 相关链接
- [协程原理介绍](https://en.wikipedia.org/wiki/Coroutine)
- [上下文切换优化](https://lwn.net/Articles/250967/)

