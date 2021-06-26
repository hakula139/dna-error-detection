# 实验报告

[![wakatime](https://wakatime.com/badge/github/hakula139/dna-error-detection.svg)](https://wakatime.com/badge/github/hakula139/dna-error-detection)

- [实验报告](#实验报告)
  - [1. 解题思路](#1-解题思路)
    - [1.1 建立索引](#11-建立索引)

## 1. 解题思路

在 task 1 的基础上，本题的主要难点分为两部分：

1. 如何将 `long.fasta` 中的 `read` 片段快速匹配到参考字符串 `ref` 上
2. 如何在 `read` 片段平均 15% 的噪音干扰下，准确找到 SV 片段

这里我们参考了 Minimap2 [^1] 的论文思路，根据实际情况进行了简化和调整。算法核心分为以下三个步骤：

1. 对参考字符串 `ref` 利用最小哈希建立索引
2. 利用 `ref` 的索引，将 `read` 片段匹配到 `ref` 上的对应区域
3. 比较 `read` 片段和 `ref` 上匹配到的区域，查找 SV 片段

### 1.1 建立索引

如何对参考字符串 `ref` 建立索引？简单来说，就是将 `ref` 中每个长度为 $k$（即 `HASH_SIZE`，默认为 `15`，所有参数均可在 [src/utils/config.cpp](../src/utils/config.cpp) 中调整）的子字符串，利用一个哈希函数转化为一个整数。具体来说，我们利用一个 2 位二进制整数来表示一个 DNA 碱基：`00` 表示 `A`、`01` 表示 `T`、`10` 表示 `C`、`11` 表示 `G`。对于一条 DNA 链，其哈希值就是将所有碱基的二进制数表示连接起来，例如 `GCTA` 的哈希值就是 `11100100`。通过这种方式，我们可以将每个连续的 $k$ 位子字符串转化为一个 $2k$ 位二进制数作为其哈希值。我们不妨称这样一个 $k$ 位子字符串为一个 $\textrm{k-mer}$。

对于一条长度为 $N$ 的 DNA 链，就包含了 $N-k+1$ 个这样的 $\textrm{k-mer}$。为了减少 $\textrm{k-mer}$ 的数量，以减少使用的内存空间，我们维护一个长度为 `WINDOW_SIZE`（默认为 `10`）的滑动窗口，只有滑动窗口中哈希值最小的 $\textrm{k-mer}$ 才会被保存作为索引。具体可参见 [src/dna.cpp](../src/dna.cpp) 中函数 `Dna::CreateIndex` 的实现。

为了方便重复使用，我们提供了 `Dna::PrintIndex` 函数用于将索引导出成文件，以及 `Dna::ImportIndex` 函数用于从文件中读取索引。

[^1]: [Heng Li. Minimap2: pairwise alignment for nucleotide sequences. Bioinformatics, 34, 18, 2018: 3094–3100.](https://doi.org/10.1093/bioinformatics/bty191)
