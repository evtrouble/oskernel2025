<font face="Maple Mono SC NF">

###### OS大赛 - 内核设计 - RuOK队

---

RuOK 队设计的操作系统，其**龙芯架构部分**是基于去年优秀作品、武汉大学的“俺争取不掉队”的内核进行修改开发的；而 RISC-V 架构部分则延续了 xv6 的设计思路。
该系统主要采用 C++ 语言编写，继承了团队此前的开发习惯和代码基础，因此具备良好的可读性与可迁移性。

同时，鉴于代码演进思路与原始设计保持高度一致，相关文档在继承原作品框架的基础上融入了本团队的架构优化与技术升级,进行增加riscv架构、修改系统调用和调试修复原有不合理之处等操作，为完善本操作系统内核作出了卓越的贡献。

### 文档

#### 0. [=> 开发环境搭建指引](./doc/develop-environmnet.md)

#### 1. [=> 工程架构](./doc/project.md)

#### 2. [=> 硬件接口设计](./doc/hsai.md)

#### 3. [=> HSAI 参考](./doc/hsai_reference.md)

#### 4. [=> ls2k的IO方式](./doc/ls2k_io.md)

#### 5. [=> 文件系统](./doc/fs.md)

#### 6. [=> 动态内存](./doc/dyn_mem.md)

#### 7. [=> 空间地址划分](./doc/memlayout.md)

#### 8. [=> uboot启动与调试](./doc/how_to_uboot.md)

#### 9. [=> 如何适配使用glibc的用户程序](./doc/how_to_adapt_glibc.md)

#### 10. [=> 适配2k1000星云板](./doc/adapt-2k1000la-dp.md)

#### 11. [=> 开发流程](./doc/img/path.png)

#### 12. [=> 开发问题和解决记录](./doc/detail.md)

#### 13. [=> 系统调用说明](./oscomp_syscalls_ruok.md)

#### 14. [=> 暂时没了解、解决的问题](./doc/to_solve.md)