# 实验报告

[![wakatime](https://wakatime.com/badge/github/hakula139/dna-error-detection.svg)](https://wakatime.com/badge/github/hakula139/dna-error-detection)

- [实验报告](#实验报告)
  - [1. 解题思路](#1-解题思路)
    - [1.1 建立索引](#11-建立索引)
    - [1.2 字符串片段的模糊匹配](#12-字符串片段的模糊匹配)
      - [1.2.1 生成 minimizer](#121-生成-minimizer)
    - [1.2.2 合并 minimizer](#122-合并-minimizer)
    - [1.2.3 匹配 ref 链和 sv 链](#123-匹配-ref-链和-sv-链)

## 1. 解题思路

在 Task 1 的基础上，本题的主要难点分为两部分：

1. 如何将 `long.fasta` 中的 $\textrm{read}$ 片段快速匹配到参考字符串 $\textrm{ref}$ 上
2. 如何在 $\textrm{read}$ 片段平均 15% 的噪音干扰下，准确找到 SV 片段

这里我们参考了 Minimap2 [^1] 的论文思路，根据实际情况进行了简化和调整。算法核心分为以下三个步骤：

1. 对参考字符串 $\textrm{ref}$ 利用最小哈希建立索引
2. 利用 $\textrm{ref}$ 的索引，将 $\textrm{read}$ 片段匹配到 $\textrm{ref}$ 上的对应区域
3. 比较 $\textrm{read}$ 片段和 $\textrm{ref}$ 上匹配到的区域，查找 SV 片段

### 1.1 建立索引

由于噪音和 SV 片段的存在，我们无法简单地在 $\textrm{ref}$ 字符串中直接查找一个 $\textrm{read}$ 子字符串片段。因此，我们需要建立索引。

如何对参考字符串 $\textrm{ref}$ 建立索引？简单来说，就是将 $\textrm{ref}$ 中每个长度为 $k$（即 `HASH_SIZE`，默认为 `15`，所有参数均可在 [src/utils/config.cpp](../src/utils/config.cpp) 中调整）的子字符串，利用一个哈希函数转化为一个整数。具体来说，我们利用一个 2 位二进制整数来表示一个 DNA 碱基：`00` 表示 `A`、`01` 表示 `T`、`10` 表示 `C`、`11` 表示 `G`。对于一条 DNA 链，其哈希值就是将所有碱基的二进制数表示连接起来，例如 `GCTA` 的哈希值就是 `11100100`。通过这种方式，我们可以将每个连续的 $k$ 位子字符串转化为一个 $2k$ 位二进制数作为其哈希值 $\textrm{hash}$。我们称这样一个从 $\textrm{hash}$ 到这个子字符串在 $\textrm{ref}$ 中位置 $[i, i+k)$ 的映射为一个 $\textrm{k-mer}$。

对于一条长度为 $N$ 的 DNA 链，就包含了 $N-k+1$ 个这样的 $\textrm{k-mer}$。为了减少 $\textrm{k-mer}$ 的数量，以减少使用的内存空间，我们维护一个长度为 `WINDOW_SIZE`（默认为 `10`）的滑动窗口，只有滑动窗口中哈希值最小的 $\textrm{k-mer}$ 才会被保存作为索引。

具体逻辑可参见 [src/dna.cpp](../src/dna.cpp) 中函数 `Dna::CreateIndex` 的实现。为了方便重复使用，我们提供了 `Dna::PrintIndex` 函数用于将索引导出成文件，以及 `Dna::ImportIndex` 函数用于从文件中读取索引。

### 1.2 字符串片段的模糊匹配

#### 1.2.1 生成 minimizer

建立完索引后，我们就可以在每个 $\textrm{read}$ 片段中遍历所有长度为 $k$ 的子字符串，根据其哈希值查找是否有相同哈希值的 $\textrm{k-mer}$。同时，我们对于 $\textrm{read}$ 片段的反向互补序列 $\textrm{read'}$ 也进行同样的操作。对于每个找到的 $\textrm{k-mer}$，我们保存一个这样的结构：$\{ \textrm{range}_\textrm{ref},\,\textrm{key}_\textrm{read},\,\textrm{range}_\textrm{read} \}$，我们称其为一个 $\textrm{minimizer}$。其中，$\textrm{range}_\textrm{ref}$ 表示 $\textrm{k-mer}$ 映射到 $\textrm{ref}$ 上的位置 $[i, i+k)$，$\textrm{key}_\textrm{read}$ 表示 $\textrm{read}$ 的编号（例如 $\textrm{S1_1}$），$\textrm{range}_\textrm{read}$ 表示这个子字符串在 $\textrm{read}$（或 $\textrm{read'}$）上的位置 $[j, j+k)$。同时，在每个 $\textrm{range}$ 中还额外保存了一个原字符串（$\textrm{ref}$ 或 $\textrm{read}$）的指针，用于之后读取及合并这个子字符串的值。在 $\textrm{range}_\textrm{read}$ 中还额外保存了 `inverted` 字段和 `unknown` 字段，分别用于指示当前 $\textrm{read}$ 是否进行了反向互补操作，以及是否包含一定数量的未知字符 `N`（在合并时用于提高效率，不关键）。

随后，我们根据 $\textrm{read}$ 和 $\textrm{read'}$ 片段上 $\textrm{minimizer}$ 的数量，决定是否对 $\textrm{read}$ 进行反向互补操作。即如果 $\textrm{read'}$ 上的 $\textrm{minimizer}$ 较多，则进行反向互补操作，反之则不进行。

具体逻辑可参见 [src/dna.cpp](../src/dna.cpp) 中函数 `Dna::FindOverlaps` 的实现。同时，我们提供了 `Dna::PrintOverlaps` 函数用于将 $\textrm{minimizer}$ 导出成文件，以及 `Dna::ImportOverlaps` 函数用于从文件中读取 $\textrm{minimizer}$。

### 1.2.2 合并 minimizer

生成 $\textrm{minimizer}$ 后，我们需要对它们进行过滤及合并。其中，过滤指的是将错误匹配的 $\textrm{minimizer}$ 移除，合并指的是将两个 $\textrm{minimizer}$ 根据其 $\textrm{range}_\textrm{ref}$ 的范围 $[i_1, i_1+k)$, $[i_2, i_2+k)$ 进行合并。

具体来说，在与一个聚类合并时，对于每一个 $\textrm{minimizer}$，我们比较此次合并后 $\textrm{range}_\textrm{ref}$ 和 $\textrm{range}_\textrm{read}$ 表示范围的增量 $\Delta_\textrm{ref}$ 和 $\Delta_\textrm{read}$。如果它们的差距不大，则将这个 $\textrm{minimizer}$ 归并到当前聚类，同时此聚类的计数器加 1；反之则尝试合并到下一个聚类，如果没有可合并的聚类，则将其单独分到一个新的聚类。

合并后，新的 $\textrm{minimizer}$ 的 $\textrm{range}_\textrm{ref}$ 为 $[\min\{i_1, i_2\}, \max\{i_1, i_2\} + k)$，$\textrm{key}_\textrm{read}$ 为空字符串，$\textrm{range}_\textrm{read}$ 为 $[0, l)$，其中 $l$ 为合并后新生成的 $\textrm{read}$ 字符串的长度，同时 $\textrm{range}_\textrm{read}$ 中保存的指针指向这个新字符串。

于是，我们就得到了若干 $\textrm{minimizer}$ 聚类。我们将其中计数器值较小或者范围较小的 $\textrm{minimizer}$ 过滤。

具体逻辑可参见 [src/dna_overlap.cpp](../src/dna_overlap.cpp) 中函数 `DnaOverlap::Merge` 的实现。

### 1.2.3 匹配 ref 链和 sv 链

由于 `long.fasta` 中同时包含了多条 $\textrm{sv}$ 链的采样，我们需要从中找到与 $\textrm{ref}$ 链匹配的 $\textrm{sv}$ 链。这里我们根据 $\textrm{minimizer}$ 在 $\textrm{ref}$ 上的覆盖率，选择覆盖率最高的 $\textrm{sv}$ 链与 $\textrm{ref}$ 链相匹配。

在 [src/utils/config.cpp](../src/utils/config.cpp) 中修改 `LOG_LEVEL` 为 `DEBUG`，即可在日志 [logs/output.log](../logs/output.log) 中看到 $\textrm{minimizer}$ 的覆盖率（搜索 `cover rate`）。对于本题的测试数据，我们的覆盖率分别达到了：

- `NZ_AP012323.1`: $98.88\%$ (`S1`)
- `NC_014616.1`: $99.17\%$ (`S2`)

具体逻辑可参见 [src/dna_overlap.cpp](../src/dna_overlap.cpp) 中函数 `DnaOverlap::SelectChain` 和 `DnaOverlap::CheckCoverage` 的实现。

[^1]: [Heng Li. Minimap2: pairwise alignment for nucleotide sequences. Bioinformatics, 34, 18, 2018: 3094–3100.](https://doi.org/10.1093/bioinformatics/bty191)
