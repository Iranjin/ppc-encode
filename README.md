# ppc-encode

A header-only C++20 library that encodes PowerPC instructions at compile time.

The target is the 32-bit PowerPC 750 family as implemented by the Wii U's Espresso: the 32-bit
user-level integer, floating-point, load/store and branch instructions. Every encoding is `consteval`, so
a bad operand (an immediate out of range, a misaligned branch, an unknown mnemonic) fails the build instead
of producing a wrong word at runtime.

Not covered: 64-bit instructions (`ld`, `rldicl`, `cmpd`, ...), paired singles (`psq_l`, `ps_add`, ...),
AltiVec, and Book E / VLE.

## Usage

```cpp
#include <ppc/encode.hpp>

#include <array>

using namespace ppc;        // functions, register names via ppc::regs, and the _ppc literal
using namespace ppc::regs;

constexpr auto kPatch = std::to_array<uint32_t>({
        "mflr r0"_ppc,
        stw(r0, 4, r1),
        "0x02000010: bl 0x02000100"_ppc, // a leading address makes branch targets absolute
        "beq+ cr7, 8"_ppc,               // + / - set the static branch prediction hint
        blr(),
});
```

To bring in only the literal, use `using namespace ppc::literals;`.

### Two ways to write an instruction

- **Functions**: `addi(r3, r1, 16)`, `lwz(r3, 0x10, r1)`, `bne(cr1, 12, Hint::Taken)`. `and`, `or`,
  `xor` and `not` are reserved words in C++, so these are `and_`, `or_`, `xor_` and `not_`. Record forms
  are spelled `Dot`: `andiDot`, `addicDot`.
- **The `_ppc` literal**: one instruction in disassembler syntax. Registers are `rN`, `fN` and `crN`.
  Numbers are decimal or `0x` hex, and a memory operand is written `d(rA)`. Branch operands are
  displacements, unless the line starts with `ADDR:`.

### Filling in values at runtime

When an address or register is only known at runtime, encode the instruction with a 0 or r0 in that
field, then OR the value in:

| Helper | Field |
| --- | --- |
| `hi(v)`, `lo(v)`, `ha(v)` | 16-bit immediate (`ha` adjusts for the sign of the low half, for `lis` + `addi`) |
| `branchField(displacement)` | 26-bit displacement of `b` / `bl` |
| `rtField(n)`, `raField(n)`, `rbField(n)` | register fields |
| `crfField(n)` | CR field of a compare |

```cpp
const uint32_t lis = "lis r12, 0"_ppc | ppc::ha(target);
const uint32_t addi = "addi r12, r12, 0"_ppc | ppc::lo(target);
```

`hi`, `lo` and `ha` also accept a pointer, but only when built for a 32-bit target.

## Integrating

The library is just the `include/` directory, so a plain Makefile (devkitPro or otherwise) only needs
`-I<path>/include` and `-std=c++20` (or `gnu++20`).

With CMake:

```cmake
add_subdirectory(third_party/ppc-encode)
target_link_libraries(my_target PRIVATE ppc::encode)
```

After `cmake --install`, use `find_package(ppc-encode REQUIRED)` instead of `add_subdirectory`.

## Tests

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

- `tests/vectors.txt` lists one instruction per line. `tests/gen_vectors.py` assembles them with
  devkitPPC's GNU assembler (`-m750cl`) and writes the expected words to `tests/vectors.inc`, which
  `encode_test.cpp` checks with `static_assert`. Run the script again after you edit the list.
- Each file in `tests/fail/` must be rejected by the encoder's compile-time checks.

## Licence

Private. Do not redistribute.
