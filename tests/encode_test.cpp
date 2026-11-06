#include <ppc/encode.hpp>

#include <array>
#include <cstdio>

using namespace ppc;
using namespace ppc::regs;

#define PPC_VECTOR(text, word) static_assert(text##_ppc == (word), text);
#include "vectors.inc"
#undef PPC_VECTOR

static_assert(li(r3, 0) == "li r3, 0"_ppc);
static_assert(stwu(r1, -16, r1) == "stwu r1, -16(r1)"_ppc);
static_assert(lfs(f1, 8, r31) == "lfs f1, 8(r31)"_ppc);
static_assert(cmpwi(cr7, r3, -1) == "cmpwi cr7, r3, -1"_ppc);
static_assert(bne(cr1, 12, Hint::Taken) == "bne+ cr1, 12"_ppc);
static_assert(bc(Bo::True, cr6.eq(), 8) == "bc 12, 26, 8"_ppc);
static_assert(mtspr(Spr::Ctr, r12) == mtctr(r12));

static_assert(b(0x02000000u, 0x02000010u) == "0x02000000: b 0x02000010"_ppc);
static_assert(bl(0x02000010u, 0x02000000u) == "0x02000010: bl 0x02000000"_ppc);
static_assert("0x100: beq cr7, 0xF8"_ppc == "beq cr7, -8"_ppc);
static_assert("li\tr3 ,\t1"_ppc == "li r3, 1"_ppc);

static_assert(hi(0x8001FFFFu) == 0x8001 && lo(0x8001FFFFu) == 0xFFFF);
static_assert(ha(0x80018000u) == 0x8002 && ha(0x80017FFFu) == 0x8001);
static_assert(("lis r3, 0"_ppc | ha(0x12348000u)) == "lis r3, 0x1235"_ppc);
static_assert(("addi r0, r0, 0"_ppc | rtField(4) | raField(5)) == "addi r4, r5, 0"_ppc);
static_assert(("add r0, r0, r0"_ppc | rbField(7)) == "add r0, r0, r7"_ppc);
static_assert(("cmpwi cr0, r3, 0"_ppc | crfField(5)) == "cmpwi cr5, r3, 0"_ppc);
static_assert(("b 0"_ppc | branchField(static_cast<uint32_t>(-8))) == "b -8"_ppc);

constexpr auto kSequence = std::to_array<uint32_t>({
        "mflr r0"_ppc,
        "stw r0, 4(r1)"_ppc,
        "blr"_ppc,
});
static_assert(kSequence.size() == 3 && kSequence[2] == blr());

#if UINTPTR_MAX == 0xFFFFFFFF
alignas(4) static const uint32_t kTarget = 0;

static bool pointerFillers() {
    const auto address = static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(&kTarget));
    return hi(&kTarget) == hi(address) && lo(&kTarget) == lo(address) && ha(&kTarget) == ha(address);
}
#else
static bool pointerFillers() { return true; }
#endif

int main() {
    if (!pointerFillers()) {
        std::puts("ppc-encode: pointer fillers disagree with the integer fillers");
        return 1;
    }
    std::puts("ppc-encode: all encodings checked at compile time");
    return 0;
}
