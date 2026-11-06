#pragma once

#include <cstdint>

namespace ppc {

struct Gpr {
    uint8_t mIndex;
};

struct Fpr {
    uint8_t mIndex;
};

struct CrBit {
    uint8_t mIndex;
};

struct CrField {
    uint8_t mIndex;

    consteval CrBit lt() const { return {static_cast<uint8_t>(mIndex * 4 + 0)}; }
    consteval CrBit gt() const { return {static_cast<uint8_t>(mIndex * 4 + 1)}; }
    consteval CrBit eq() const { return {static_cast<uint8_t>(mIndex * 4 + 2)}; }
    consteval CrBit so() const { return {static_cast<uint8_t>(mIndex * 4 + 3)}; }
};

namespace regs {

inline constexpr Gpr r0{0}, r1{1}, r2{2}, r3{3}, r4{4}, r5{5}, r6{6}, r7{7};
inline constexpr Gpr r8{8}, r9{9}, r10{10}, r11{11}, r12{12}, r13{13}, r14{14}, r15{15};
inline constexpr Gpr r16{16}, r17{17}, r18{18}, r19{19}, r20{20}, r21{21}, r22{22}, r23{23};
inline constexpr Gpr r24{24}, r25{25}, r26{26}, r27{27}, r28{28}, r29{29}, r30{30}, r31{31};

inline constexpr Fpr f0{0}, f1{1}, f2{2}, f3{3}, f4{4}, f5{5}, f6{6}, f7{7};
inline constexpr Fpr f8{8}, f9{9}, f10{10}, f11{11}, f12{12}, f13{13}, f14{14}, f15{15};
inline constexpr Fpr f16{16}, f17{17}, f18{18}, f19{19}, f20{20}, f21{21}, f22{22}, f23{23};
inline constexpr Fpr f24{24}, f25{25}, f26{26}, f27{27}, f28{28}, f29{29}, f30{30}, f31{31};

inline constexpr CrField cr0{0}, cr1{1}, cr2{2}, cr3{3}, cr4{4}, cr5{5}, cr6{6}, cr7{7};

} // namespace regs

} // namespace ppc
