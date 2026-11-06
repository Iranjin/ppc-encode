#pragma once

#include "registers.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ppc {

enum class Hint : uint8_t { None, Taken, NotTaken };

enum class Bo : uint8_t {
    DnzFalse = 0,
    DzFalse = 2,
    False = 4,
    DnzTrue = 8,
    DzTrue = 10,
    True = 12,
    Dnz = 16,
    Dz = 18,
    Always = 20,
};

enum class Spr : uint16_t { Xer = 1, Lr = 8, Ctr = 9 };

namespace detail {

// Never defined: reaching it during constant evaluation turns a bad operand into a compile error.
void encodeError(const char* message);

consteval uint32_t simm16(int32_t value) {
    if (value < -0x8000 || value > 0x7FFF) encodeError("signed 16-bit immediate out of range");
    return static_cast<uint32_t>(value) & 0xFFFF;
}

consteval uint32_t uimm16(int32_t value) {
    if (value < 0 || value > 0xFFFF) encodeError("unsigned 16-bit immediate out of range");
    return static_cast<uint32_t>(value);
}

consteval uint32_t anyImm16(int32_t value) {
    if (value < -0x8000 || value > 0xFFFF) encodeError("16-bit immediate out of range");
    return static_cast<uint32_t>(value) & 0xFFFF;
}

consteval uint32_t uimm5(int32_t value) {
    if (value < 0 || value > 31) encodeError("5-bit field out of range");
    return static_cast<uint32_t>(value);
}

consteval uint32_t dForm(uint32_t op, uint32_t rt, uint32_t ra, uint32_t imm) {
    return (op << 26) | (rt << 21) | (ra << 16) | imm;
}

consteval uint32_t xForm(uint32_t op, uint32_t rt, uint32_t ra, uint32_t rb, uint32_t xo) {
    return (op << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (xo << 1);
}

consteval uint32_t aForm(uint32_t op, Fpr frt, Fpr fra, Fpr frb, Fpr frc, uint32_t xo) {
    return (op << 26) | (frt.mIndex << 21) | (fra.mIndex << 16) | (frb.mIndex << 11) | (frc.mIndex << 6) | (xo << 1);
}

consteval uint32_t mForm(uint32_t op, Gpr ra, Gpr rs, uint32_t sh, int32_t mb, int32_t me) {
    return (op << 26) | (rs.mIndex << 21) | (ra.mIndex << 16) | (sh << 11) | (uimm5(mb) << 6) | (uimm5(me) << 1);
}

consteval uint32_t iForm(int32_t li, bool lk) {
    if (li & 3) encodeError("misaligned branch displacement");
    if (li < -0x2000000 || li > 0x1FFFFFC) encodeError("branch displacement out of 26-bit range");
    return (18u << 26) | (static_cast<uint32_t>(li) & 0x03FFFFFC) | (lk ? 1u : 0u);
}

consteval uint32_t bForm(Bo bo, CrBit bi, int32_t bd, Hint hint, bool lk) {
    if (bd & 3) encodeError("misaligned branch displacement");
    if (bd < -0x8000 || bd > 0x7FFC) encodeError("branch displacement out of 16-bit range");
    if (hint != Hint::None && bo == Bo::Always) encodeError("unconditional branch cannot take a hint");
    // Espresso uses the classic y bit: it flips the static forward-not-taken / backward-taken guess.
    const bool y = (hint == Hint::Taken && bd >= 0) || (hint == Hint::NotTaken && bd < 0);
    return (16u << 26) | ((static_cast<uint32_t>(bo) | (y ? 1u : 0u)) << 21) | (bi.mIndex << 16) |
           (static_cast<uint32_t>(bd) & 0xFFFC) | (lk ? 1u : 0u);
}

consteval uint32_t xlBranch(uint32_t xo, bool lk) {
    return (19u << 26) | (static_cast<uint32_t>(Bo::Always) << 21) | (xo << 1) | (lk ? 1u : 0u);
}

consteval uint32_t sprField(Spr spr) {
    const uint32_t n = static_cast<uint32_t>(spr);
    return ((n & 0x1F) << 5) | (n >> 5);
}

consteval Spr sprNumber(int32_t value) {
    if (value != 1 && value != 8 && value != 9) encodeError("unsupported SPR number");
    return static_cast<Spr>(value);
}

consteval uint32_t updateForm(uint32_t op, uint32_t rt, int32_t d, Gpr ra) {
    if (ra.mIndex == 0) encodeError("update form cannot use r0 as base");
    return dForm(op, rt, ra.mIndex, simm16(d));
}

consteval uint32_t gprLoadUpdate(uint32_t op, Gpr rt, int32_t d, Gpr ra) {
    if (ra.mIndex == rt.mIndex) encodeError("load with update cannot target its base register");
    return updateForm(op, rt.mIndex, d, ra);
}

} // namespace detail

// Integer arithmetic

consteval uint32_t addi(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(14, rt.mIndex, ra.mIndex, detail::simm16(si));
}
consteval uint32_t subi(Gpr rt, Gpr ra, int32_t si) { return addi(rt, ra, -si); }
consteval uint32_t addis(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(15, rt.mIndex, ra.mIndex, detail::anyImm16(si));
}
consteval uint32_t li(Gpr rt, int32_t si) { return detail::dForm(14, rt.mIndex, 0, detail::simm16(si)); }
consteval uint32_t lis(Gpr rt, int32_t si) { return detail::dForm(15, rt.mIndex, 0, detail::anyImm16(si)); }
consteval uint32_t addic(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(12, rt.mIndex, ra.mIndex, detail::simm16(si));
}
consteval uint32_t addicDot(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(13, rt.mIndex, ra.mIndex, detail::simm16(si));
}
consteval uint32_t mulli(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(7, rt.mIndex, ra.mIndex, detail::simm16(si));
}
consteval uint32_t subfic(Gpr rt, Gpr ra, int32_t si) {
    return detail::dForm(8, rt.mIndex, ra.mIndex, detail::simm16(si));
}

consteval uint32_t add(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 266); }
consteval uint32_t subf(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 40); }
consteval uint32_t sub(Gpr rt, Gpr ra, Gpr rb) { return subf(rt, rb, ra); }
consteval uint32_t neg(Gpr rt, Gpr ra) { return detail::xForm(31, rt.mIndex, ra.mIndex, 0, 104); }
consteval uint32_t mullw(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 235); }
consteval uint32_t mulhw(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 75); }
consteval uint32_t mulhwu(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 11); }
consteval uint32_t divw(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 491); }
consteval uint32_t divwu(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 459); }

// Logical (the C++ alternative tokens and/or/xor/not force a trailing underscore)

consteval uint32_t ori(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(24, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t oris(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(25, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t xori(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(26, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t xoris(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(27, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t andiDot(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(28, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t andisDot(Gpr ra, Gpr rs, int32_t ui) {
    return detail::dForm(29, rs.mIndex, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t nop() { return ori(regs::r0, regs::r0, 0); }

consteval uint32_t or_(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 444); }
consteval uint32_t and_(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 28); }
consteval uint32_t xor_(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 316); }
consteval uint32_t nor(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 124); }
consteval uint32_t andc(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 60); }
consteval uint32_t orc(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 412); }
consteval uint32_t mr(Gpr ra, Gpr rs) { return or_(ra, rs, rs); }
consteval uint32_t not_(Gpr ra, Gpr rs) { return nor(ra, rs, rs); }
consteval uint32_t extsb(Gpr ra, Gpr rs) { return detail::xForm(31, rs.mIndex, ra.mIndex, 0, 954); }
consteval uint32_t extsh(Gpr ra, Gpr rs) { return detail::xForm(31, rs.mIndex, ra.mIndex, 0, 922); }
consteval uint32_t cntlzw(Gpr ra, Gpr rs) { return detail::xForm(31, rs.mIndex, ra.mIndex, 0, 26); }

// Shift / rotate

consteval uint32_t slw(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 24); }
consteval uint32_t srw(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 536); }
consteval uint32_t sraw(Gpr ra, Gpr rs, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 792); }
consteval uint32_t srawi(Gpr ra, Gpr rs, int32_t sh) {
    return detail::xForm(31, rs.mIndex, ra.mIndex, detail::uimm5(sh), 824);
}

consteval uint32_t rlwinm(Gpr ra, Gpr rs, int32_t sh, int32_t mb, int32_t me) {
    return detail::mForm(21, ra, rs, detail::uimm5(sh), mb, me);
}
consteval uint32_t rlwimi(Gpr ra, Gpr rs, int32_t sh, int32_t mb, int32_t me) {
    return detail::mForm(20, ra, rs, detail::uimm5(sh), mb, me);
}
consteval uint32_t rlwnm(Gpr ra, Gpr rs, Gpr rb, int32_t mb, int32_t me) {
    return detail::mForm(23, ra, rs, rb.mIndex, mb, me);
}
consteval uint32_t rotlwi(Gpr ra, Gpr rs, int32_t n) { return rlwinm(ra, rs, n, 0, 31); }
consteval uint32_t slwi(Gpr ra, Gpr rs, int32_t n) { return rlwinm(ra, rs, n, 0, 31 - n); }
consteval uint32_t srwi(Gpr ra, Gpr rs, int32_t n) { return rlwinm(ra, rs, (32 - detail::uimm5(n)) & 31, n, 31); }
consteval uint32_t clrlwi(Gpr ra, Gpr rs, int32_t n) { return rlwinm(ra, rs, 0, n, 31); }
consteval uint32_t clrrwi(Gpr ra, Gpr rs, int32_t n) { return rlwinm(ra, rs, 0, 0, 31 - n); }

// Compare

consteval uint32_t cmpwi(CrField cr, Gpr ra, int32_t si) {
    return detail::dForm(11, cr.mIndex << 2, ra.mIndex, detail::simm16(si));
}
consteval uint32_t cmpwi(Gpr ra, int32_t si) { return cmpwi(regs::cr0, ra, si); }
consteval uint32_t cmplwi(CrField cr, Gpr ra, int32_t ui) {
    return detail::dForm(10, cr.mIndex << 2, ra.mIndex, detail::uimm16(ui));
}
consteval uint32_t cmplwi(Gpr ra, int32_t ui) { return cmplwi(regs::cr0, ra, ui); }
consteval uint32_t cmpw(CrField cr, Gpr ra, Gpr rb) {
    return detail::xForm(31, cr.mIndex << 2, ra.mIndex, rb.mIndex, 0);
}
consteval uint32_t cmpw(Gpr ra, Gpr rb) { return cmpw(regs::cr0, ra, rb); }
consteval uint32_t cmplw(CrField cr, Gpr ra, Gpr rb) {
    return detail::xForm(31, cr.mIndex << 2, ra.mIndex, rb.mIndex, 32);
}
consteval uint32_t cmplw(Gpr ra, Gpr rb) { return cmplw(regs::cr0, ra, rb); }

// Load / store

consteval uint32_t lwz(Gpr rt, int32_t d, Gpr ra) { return detail::dForm(32, rt.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t lwzu(Gpr rt, int32_t d, Gpr ra) { return detail::gprLoadUpdate(33, rt, d, ra); }
consteval uint32_t lbz(Gpr rt, int32_t d, Gpr ra) { return detail::dForm(34, rt.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t lbzu(Gpr rt, int32_t d, Gpr ra) { return detail::gprLoadUpdate(35, rt, d, ra); }
consteval uint32_t lhz(Gpr rt, int32_t d, Gpr ra) { return detail::dForm(40, rt.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t lhzu(Gpr rt, int32_t d, Gpr ra) { return detail::gprLoadUpdate(41, rt, d, ra); }
consteval uint32_t lha(Gpr rt, int32_t d, Gpr ra) { return detail::dForm(42, rt.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t lhau(Gpr rt, int32_t d, Gpr ra) { return detail::gprLoadUpdate(43, rt, d, ra); }
consteval uint32_t stw(Gpr rs, int32_t d, Gpr ra) { return detail::dForm(36, rs.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t stwu(Gpr rs, int32_t d, Gpr ra) { return detail::updateForm(37, rs.mIndex, d, ra); }
consteval uint32_t stb(Gpr rs, int32_t d, Gpr ra) { return detail::dForm(38, rs.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t stbu(Gpr rs, int32_t d, Gpr ra) { return detail::updateForm(39, rs.mIndex, d, ra); }
consteval uint32_t sth(Gpr rs, int32_t d, Gpr ra) { return detail::dForm(44, rs.mIndex, ra.mIndex, detail::simm16(d)); }
consteval uint32_t sthu(Gpr rs, int32_t d, Gpr ra) { return detail::updateForm(45, rs.mIndex, d, ra); }

consteval uint32_t lmw(Gpr rt, int32_t d, Gpr ra) {
    if (ra.mIndex >= rt.mIndex) detail::encodeError("lmw base register is in the loaded range");
    return detail::dForm(46, rt.mIndex, ra.mIndex, detail::simm16(d));
}

consteval uint32_t stmw(Gpr rs, int32_t d, Gpr ra) {
    return detail::dForm(47, rs.mIndex, ra.mIndex, detail::simm16(d));
}

consteval uint32_t lwzx(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 23); }
consteval uint32_t lbzx(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 87); }
consteval uint32_t lhzx(Gpr rt, Gpr ra, Gpr rb) { return detail::xForm(31, rt.mIndex, ra.mIndex, rb.mIndex, 279); }
consteval uint32_t stwx(Gpr rs, Gpr ra, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 151); }
consteval uint32_t stbx(Gpr rs, Gpr ra, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 215); }
consteval uint32_t sthx(Gpr rs, Gpr ra, Gpr rb) { return detail::xForm(31, rs.mIndex, ra.mIndex, rb.mIndex, 407); }

consteval uint32_t lfs(Fpr frt, int32_t d, Gpr ra) {
    return detail::dForm(48, frt.mIndex, ra.mIndex, detail::simm16(d));
}
consteval uint32_t lfsu(Fpr frt, int32_t d, Gpr ra) { return detail::updateForm(49, frt.mIndex, d, ra); }
consteval uint32_t lfd(Fpr frt, int32_t d, Gpr ra) {
    return detail::dForm(50, frt.mIndex, ra.mIndex, detail::simm16(d));
}
consteval uint32_t lfdu(Fpr frt, int32_t d, Gpr ra) { return detail::updateForm(51, frt.mIndex, d, ra); }
consteval uint32_t stfs(Fpr frs, int32_t d, Gpr ra) {
    return detail::dForm(52, frs.mIndex, ra.mIndex, detail::simm16(d));
}
consteval uint32_t stfsu(Fpr frs, int32_t d, Gpr ra) { return detail::updateForm(53, frs.mIndex, d, ra); }
consteval uint32_t stfd(Fpr frs, int32_t d, Gpr ra) {
    return detail::dForm(54, frs.mIndex, ra.mIndex, detail::simm16(d));
}
consteval uint32_t stfdu(Fpr frs, int32_t d, Gpr ra) { return detail::updateForm(55, frs.mIndex, d, ra); }

consteval uint32_t lfsx(Fpr frt, Gpr ra, Gpr rb) { return detail::xForm(31, frt.mIndex, ra.mIndex, rb.mIndex, 535); }
consteval uint32_t lfdx(Fpr frt, Gpr ra, Gpr rb) { return detail::xForm(31, frt.mIndex, ra.mIndex, rb.mIndex, 599); }
consteval uint32_t stfsx(Fpr frs, Gpr ra, Gpr rb) { return detail::xForm(31, frs.mIndex, ra.mIndex, rb.mIndex, 663); }
consteval uint32_t stfdx(Fpr frs, Gpr ra, Gpr rb) { return detail::xForm(31, frs.mIndex, ra.mIndex, rb.mIndex, 727); }

// Floating point

consteval uint32_t fmr(Fpr frt, Fpr frb) { return detail::xForm(63, frt.mIndex, 0, frb.mIndex, 72); }
consteval uint32_t fneg(Fpr frt, Fpr frb) { return detail::xForm(63, frt.mIndex, 0, frb.mIndex, 40); }
consteval uint32_t fabs(Fpr frt, Fpr frb) { return detail::xForm(63, frt.mIndex, 0, frb.mIndex, 264); }
consteval uint32_t frsp(Fpr frt, Fpr frb) { return detail::xForm(63, frt.mIndex, 0, frb.mIndex, 12); }
consteval uint32_t fctiwz(Fpr frt, Fpr frb) { return detail::xForm(63, frt.mIndex, 0, frb.mIndex, 15); }
consteval uint32_t fcmpu(CrField cr, Fpr fra, Fpr frb) {
    return detail::xForm(63, cr.mIndex << 2, fra.mIndex, frb.mIndex, 0);
}
consteval uint32_t fcmpo(CrField cr, Fpr fra, Fpr frb) {
    return detail::xForm(63, cr.mIndex << 2, fra.mIndex, frb.mIndex, 32);
}

consteval uint32_t fadd(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(63, frt, fra, frb, regs::f0, 21); }
consteval uint32_t fsub(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(63, frt, fra, frb, regs::f0, 20); }
consteval uint32_t fmul(Fpr frt, Fpr fra, Fpr frc) { return detail::aForm(63, frt, fra, regs::f0, frc, 25); }
consteval uint32_t fdiv(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(63, frt, fra, frb, regs::f0, 18); }
consteval uint32_t fadds(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(59, frt, fra, frb, regs::f0, 21); }
consteval uint32_t fsubs(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(59, frt, fra, frb, regs::f0, 20); }
consteval uint32_t fmuls(Fpr frt, Fpr fra, Fpr frc) { return detail::aForm(59, frt, fra, regs::f0, frc, 25); }
consteval uint32_t fdivs(Fpr frt, Fpr fra, Fpr frb) { return detail::aForm(59, frt, fra, frb, regs::f0, 18); }

// Special registers / sync

consteval uint32_t mfspr(Gpr rt, Spr spr) {
    return detail::xForm(31, rt.mIndex, 0, 0, 339) | (detail::sprField(spr) << 11);
}
consteval uint32_t mtspr(Spr spr, Gpr rs) {
    return detail::xForm(31, rs.mIndex, 0, 0, 467) | (detail::sprField(spr) << 11);
}
consteval uint32_t mflr(Gpr rt) { return mfspr(rt, Spr::Lr); }
consteval uint32_t mtlr(Gpr rs) { return mtspr(Spr::Lr, rs); }
consteval uint32_t mfctr(Gpr rt) { return mfspr(rt, Spr::Ctr); }
consteval uint32_t mtctr(Gpr rs) { return mtspr(Spr::Ctr, rs); }
consteval uint32_t mfcr(Gpr rt) { return detail::xForm(31, rt.mIndex, 0, 0, 19); }
consteval uint32_t sync() { return detail::xForm(31, 0, 0, 0, 598); }
consteval uint32_t isync() { return detail::xForm(19, 0, 0, 0, 150); }
consteval uint32_t trap() { return detail::xForm(31, 31, 0, 0, 4); }

// Branch

consteval uint32_t b(int32_t li) { return detail::iForm(li, false); }
consteval uint32_t bl(int32_t li) { return detail::iForm(li, true); }
consteval uint32_t b(uint32_t from, uint32_t to) { return b(static_cast<int32_t>(to - from)); }
consteval uint32_t bl(uint32_t from, uint32_t to) { return bl(static_cast<int32_t>(to - from)); }

consteval uint32_t bc(Bo bo, CrBit bi, int32_t bd, Hint hint = Hint::None) {
    return detail::bForm(bo, bi, bd, hint, false);
}
consteval uint32_t bc(Bo bo, int32_t bd) { return bc(bo, regs::cr0.lt(), bd); }

consteval uint32_t blt(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::True, cr.lt(), bd, hint); }
consteval uint32_t bgt(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::True, cr.gt(), bd, hint); }
consteval uint32_t beq(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::True, cr.eq(), bd, hint); }
consteval uint32_t bso(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::True, cr.so(), bd, hint); }
consteval uint32_t bge(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::False, cr.lt(), bd, hint); }
consteval uint32_t ble(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::False, cr.gt(), bd, hint); }
consteval uint32_t bne(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::False, cr.eq(), bd, hint); }
consteval uint32_t bns(CrField cr, int32_t bd, Hint hint = Hint::None) { return bc(Bo::False, cr.so(), bd, hint); }
consteval uint32_t blt(int32_t bd, Hint hint = Hint::None) { return blt(regs::cr0, bd, hint); }
consteval uint32_t bgt(int32_t bd, Hint hint = Hint::None) { return bgt(regs::cr0, bd, hint); }
consteval uint32_t beq(int32_t bd, Hint hint = Hint::None) { return beq(regs::cr0, bd, hint); }
consteval uint32_t bso(int32_t bd, Hint hint = Hint::None) { return bso(regs::cr0, bd, hint); }
consteval uint32_t bge(int32_t bd, Hint hint = Hint::None) { return bge(regs::cr0, bd, hint); }
consteval uint32_t ble(int32_t bd, Hint hint = Hint::None) { return ble(regs::cr0, bd, hint); }
consteval uint32_t bne(int32_t bd, Hint hint = Hint::None) { return bne(regs::cr0, bd, hint); }
consteval uint32_t bns(int32_t bd, Hint hint = Hint::None) { return bns(regs::cr0, bd, hint); }
consteval uint32_t bdnz(int32_t bd, Hint hint = Hint::None) { return bc(Bo::Dnz, regs::cr0.lt(), bd, hint); }
consteval uint32_t bdz(int32_t bd, Hint hint = Hint::None) { return bc(Bo::Dz, regs::cr0.lt(), bd, hint); }

consteval uint32_t blr() { return detail::xlBranch(16, false); }
consteval uint32_t blrl() { return detail::xlBranch(16, true); }
consteval uint32_t bctr() { return detail::xlBranch(528, false); }
consteval uint32_t bctrl() { return detail::xlBranch(528, true); }

namespace detail {

enum class OperandKind : uint8_t { Gpr, Fpr, Cr, Imm, Mem };

struct Operand {
    OperandKind mKind;
    uint8_t mReg; // register number, or the base register of a Mem operand
    int64_t mValue; // immediate, or the displacement of a Mem operand
};

consteval bool isDigit(char c) { return c >= '0' && c <= '9'; }
consteval bool isWordChar(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isDigit(c) || c == '.'; }

class AsmCursor {
public:
    consteval explicit AsmCursor(std::string_view text) : mText(text) {}

    consteval char peek() {
        while (mPos < mText.size() && (mText[mPos] == ' ' || mText[mPos] == '\t')) ++mPos;
        return mPos < mText.size() ? mText[mPos] : '\0';
    }

    consteval bool accept(char c) {
        if (peek() != c) return false;
        ++mPos;
        return true;
    }

    consteval void expect(char c) {
        if (!accept(c)) encodeError("unexpected character in instruction text");
    }

    // Reads the character right after the previous token, so `bne+` is told apart from `bne +4`.
    consteval char take() {
        return mPos < mText.size() && (mText[mPos] == '+' || mText[mPos] == '-') ? mText[mPos++] : '\0';
    }

    consteval std::string_view word() {
        peek();
        const std::size_t start = mPos;
        while (mPos < mText.size() && isWordChar(mText[mPos])) ++mPos;
        if (mPos == start) encodeError("expected a mnemonic or register");
        return mText.substr(start, mPos - start);
    }

    consteval int64_t number() {
        const bool negative = accept('-');
        peek();
        int64_t base = 10;
        if (mText.substr(mPos, 2) == "0x" || mText.substr(mPos, 2) == "0X") {
            base = 16;
            mPos += 2;
        }
        const std::size_t start = mPos;
        int64_t value = 0;
        for (; mPos < mText.size(); ++mPos) {
            const char c = mText[mPos];
            int64_t digit = 0;
            if (isDigit(c))
                digit = c - '0';
            else if (base == 16 && c >= 'a' && c <= 'f')
                digit = c - 'a' + 10;
            else if (base == 16 && c >= 'A' && c <= 'F')
                digit = c - 'A' + 10;
            else
                break;
            value = value * base + digit;
            if (value > 0xFFFFFFFF) encodeError("number out of 32-bit range");
        }
        if (mPos == start) encodeError("expected a number");
        return negative ? -value : value;
    }

private:
    std::string_view mText;
    std::size_t mPos = 0;
};

consteval uint8_t registerIndex(std::string_view name, std::size_t prefix, int64_t limit) {
    if (name.size() <= prefix) return 0xFF;
    int64_t index = 0;
    for (std::size_t i = prefix; i < name.size(); ++i) {
        if (!isDigit(name[i])) return 0xFF;
        index = index * 10 + (name[i] - '0');
    }
    return index <= limit ? static_cast<uint8_t>(index) : 0xFF;
}

class AsmLine {
public:
    consteval explicit AsmLine(std::string_view text) {
        AsmCursor cursor(text);
        if (isDigit(cursor.peek())) {
            mHasAddress = true;
            mAddress = static_cast<uint32_t>(cursor.number());
            cursor.expect(':');
        }
        mMnemonic = cursor.word();
        const char hint = cursor.take();
        mHint = hint == '+' ? Hint::Taken : hint == '-' ? Hint::NotTaken : Hint::None;
        if (cursor.peek() == '\0') return;
        do {
            if (mCount == 5) encodeError("too many operands");
            mOperands[mCount++] = operand(cursor);
        } while (cursor.accept(','));
        if (cursor.peek() != '\0') encodeError("unexpected text after operands");
    }

    consteval bool is(std::string_view mnemonic, uint32_t count) const {
        return mMnemonic == mnemonic && mCount == count;
    }

    consteval Hint hint() const { return mHint; }

    consteval Gpr gpr(uint32_t i) const { return {at(i, OperandKind::Gpr).mReg}; }
    consteval Fpr fpr(uint32_t i) const { return {at(i, OperandKind::Fpr).mReg}; }
    consteval CrField cr(uint32_t i) const { return {at(i, OperandKind::Cr).mReg}; }
    consteval int32_t imm(uint32_t i) const { return int32(at(i, OperandKind::Imm).mValue); }
    consteval int32_t disp(uint32_t i) const { return int32(at(i, OperandKind::Mem).mValue); }
    consteval Gpr base(uint32_t i) const { return {at(i, OperandKind::Mem).mReg}; }

    // Displacement as written, or the distance from the instruction's own address when one is given.
    consteval int32_t target(uint32_t i) const {
        const int64_t value = at(i, OperandKind::Imm).mValue;
        if (!mHasAddress) return int32(value);
        if (value < 0) encodeError("branch target address cannot be negative");
        return static_cast<int32_t>(static_cast<uint32_t>(value) - mAddress);
    }

private:
    static consteval Operand operand(AsmCursor& cursor) {
        const char c = cursor.peek();
        if (!isDigit(c) && c != '-') {
            const std::string_view name = cursor.word();
            uint8_t index = 0xFF;
            if (name.starts_with("cr") && (index = registerIndex(name, 2, 7)) != 0xFF)
                return {OperandKind::Cr, index, 0};
            if (name[0] == 'r' && (index = registerIndex(name, 1, 31)) != 0xFF) return {OperandKind::Gpr, index, 0};
            if (name[0] == 'f' && (index = registerIndex(name, 1, 31)) != 0xFF) return {OperandKind::Fpr, index, 0};
            encodeError("unknown register");
        }
        const int64_t value = cursor.number();
        if (!cursor.accept('(')) return {OperandKind::Imm, 0, value};
        const uint8_t index = registerIndex(cursor.word(), 1, 31);
        if (index == 0xFF) encodeError("memory operand base must be a GPR");
        cursor.expect(')');
        return {OperandKind::Mem, index, value};
    }

    static consteval int32_t int32(int64_t value) {
        if (value < -0x80000000LL || value > 0x7FFFFFFF) encodeError("immediate out of 32-bit signed range");
        return static_cast<int32_t>(value);
    }

    consteval const Operand& at(uint32_t i, OperandKind kind) const {
        if (mOperands[i].mKind != kind) encodeError("operand has the wrong kind");
        return mOperands[i];
    }

    std::string_view mMnemonic;
    Hint mHint = Hint::None;
    bool mHasAddress = false;
    uint32_t mAddress = 0;
    Operand mOperands[5]{};
    uint32_t mCount = 0;
};

consteval uint32_t conditionalBranch(const AsmLine& l, std::string_view name, Bo bo, uint32_t bit) {
    if (l.is(name, 1)) return bc(bo, CrBit{static_cast<uint8_t>(bit)}, l.target(0), l.hint());
    const CrField cr = l.cr(0);
    return bc(bo, CrBit{static_cast<uint8_t>(cr.mIndex * 4 + bit)}, l.target(1), l.hint());
}

consteval uint32_t assemble(const AsmLine& l) {
    constexpr struct {
        std::string_view mName;
        Bo mBo;
        uint32_t mBit;
    } kConditional[] = {
            {"blt", Bo::True, 0},  {"bgt", Bo::True, 1},  {"beq", Bo::True, 2},  {"bso", Bo::True, 3},
            {"bge", Bo::False, 0}, {"ble", Bo::False, 1}, {"bne", Bo::False, 2}, {"bns", Bo::False, 3},
    };
    for (const auto& branch : kConditional) {
        if (l.is(branch.mName, 1) || l.is(branch.mName, 2))
            return conditionalBranch(l, branch.mName, branch.mBo, branch.mBit);
    }
    if (l.is("bdnz", 1)) return bdnz(l.target(0), l.hint());
    if (l.is("bdz", 1)) return bdz(l.target(0), l.hint());
    if (l.is("bc", 3)) {
        return bc(static_cast<Bo>(uimm5(l.imm(0))), CrBit{static_cast<uint8_t>(uimm5(l.imm(1)))}, l.target(2),
                  l.hint());
    }
    if (l.hint() != Hint::None) encodeError("only conditional branches take a hint");

    // Integer arithmetic
    if (l.is("addi", 3)) return addi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("subi", 3)) return subi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("addis", 3)) return addis(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("li", 2)) return li(l.gpr(0), l.imm(1));
    if (l.is("lis", 2)) return lis(l.gpr(0), l.imm(1));
    if (l.is("addic", 3)) return addic(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("addic.", 3)) return addicDot(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("mulli", 3)) return mulli(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("subfic", 3)) return subfic(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("add", 3)) return add(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("subf", 3)) return subf(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("sub", 3)) return sub(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("neg", 2)) return neg(l.gpr(0), l.gpr(1));
    if (l.is("mullw", 3)) return mullw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("mulhw", 3)) return mulhw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("mulhwu", 3)) return mulhwu(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("divw", 3)) return divw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("divwu", 3)) return divwu(l.gpr(0), l.gpr(1), l.gpr(2));

    // Logical
    if (l.is("ori", 3)) return ori(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("oris", 3)) return oris(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("xori", 3)) return xori(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("xoris", 3)) return xoris(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("andi.", 3)) return andiDot(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("andis.", 3)) return andisDot(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("nop", 0)) return nop();
    if (l.is("or", 3)) return or_(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("and", 3)) return and_(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("xor", 3)) return xor_(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("nor", 3)) return nor(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("andc", 3)) return andc(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("orc", 3)) return orc(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("mr", 2)) return mr(l.gpr(0), l.gpr(1));
    if (l.is("not", 2)) return not_(l.gpr(0), l.gpr(1));
    if (l.is("extsb", 2)) return extsb(l.gpr(0), l.gpr(1));
    if (l.is("extsh", 2)) return extsh(l.gpr(0), l.gpr(1));
    if (l.is("cntlzw", 2)) return cntlzw(l.gpr(0), l.gpr(1));

    // Shift / rotate
    if (l.is("slw", 3)) return slw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("srw", 3)) return srw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("sraw", 3)) return sraw(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("srawi", 3)) return srawi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("rlwinm", 5)) return rlwinm(l.gpr(0), l.gpr(1), l.imm(2), l.imm(3), l.imm(4));
    if (l.is("rlwimi", 5)) return rlwimi(l.gpr(0), l.gpr(1), l.imm(2), l.imm(3), l.imm(4));
    if (l.is("rlwnm", 5)) return rlwnm(l.gpr(0), l.gpr(1), l.gpr(2), l.imm(3), l.imm(4));
    if (l.is("rotlwi", 3)) return rotlwi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("slwi", 3)) return slwi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("srwi", 3)) return srwi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("clrlwi", 3)) return clrlwi(l.gpr(0), l.gpr(1), l.imm(2));
    if (l.is("clrrwi", 3)) return clrrwi(l.gpr(0), l.gpr(1), l.imm(2));

    // Compare
    if (l.is("cmpwi", 2)) return cmpwi(l.gpr(0), l.imm(1));
    if (l.is("cmpwi", 3)) return cmpwi(l.cr(0), l.gpr(1), l.imm(2));
    if (l.is("cmplwi", 2)) return cmplwi(l.gpr(0), l.imm(1));
    if (l.is("cmplwi", 3)) return cmplwi(l.cr(0), l.gpr(1), l.imm(2));
    if (l.is("cmpw", 2)) return cmpw(l.gpr(0), l.gpr(1));
    if (l.is("cmpw", 3)) return cmpw(l.cr(0), l.gpr(1), l.gpr(2));
    if (l.is("cmplw", 2)) return cmplw(l.gpr(0), l.gpr(1));
    if (l.is("cmplw", 3)) return cmplw(l.cr(0), l.gpr(1), l.gpr(2));

    // Load / store
    if (l.is("lwz", 2)) return lwz(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lwzu", 2)) return lwzu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lbz", 2)) return lbz(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lbzu", 2)) return lbzu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lhz", 2)) return lhz(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lhzu", 2)) return lhzu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lha", 2)) return lha(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lhau", 2)) return lhau(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("stw", 2)) return stw(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("stwu", 2)) return stwu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("stb", 2)) return stb(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("stbu", 2)) return stbu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("sth", 2)) return sth(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("sthu", 2)) return sthu(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lmw", 2)) return lmw(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("stmw", 2)) return stmw(l.gpr(0), l.disp(1), l.base(1));
    if (l.is("lwzx", 3)) return lwzx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("lbzx", 3)) return lbzx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("lhzx", 3)) return lhzx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("stwx", 3)) return stwx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("stbx", 3)) return stbx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("sthx", 3)) return sthx(l.gpr(0), l.gpr(1), l.gpr(2));
    if (l.is("lfs", 2)) return lfs(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("lfsu", 2)) return lfsu(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("lfd", 2)) return lfd(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("lfdu", 2)) return lfdu(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("stfs", 2)) return stfs(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("stfsu", 2)) return stfsu(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("stfd", 2)) return stfd(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("stfdu", 2)) return stfdu(l.fpr(0), l.disp(1), l.base(1));
    if (l.is("lfsx", 3)) return lfsx(l.fpr(0), l.gpr(1), l.gpr(2));
    if (l.is("lfdx", 3)) return lfdx(l.fpr(0), l.gpr(1), l.gpr(2));
    if (l.is("stfsx", 3)) return stfsx(l.fpr(0), l.gpr(1), l.gpr(2));
    if (l.is("stfdx", 3)) return stfdx(l.fpr(0), l.gpr(1), l.gpr(2));

    // Floating point
    if (l.is("fmr", 2)) return fmr(l.fpr(0), l.fpr(1));
    if (l.is("fneg", 2)) return fneg(l.fpr(0), l.fpr(1));
    if (l.is("fabs", 2)) return fabs(l.fpr(0), l.fpr(1));
    if (l.is("frsp", 2)) return frsp(l.fpr(0), l.fpr(1));
    if (l.is("fctiwz", 2)) return fctiwz(l.fpr(0), l.fpr(1));
    if (l.is("fcmpu", 3)) return fcmpu(l.cr(0), l.fpr(1), l.fpr(2));
    if (l.is("fcmpo", 3)) return fcmpo(l.cr(0), l.fpr(1), l.fpr(2));
    if (l.is("fadd", 3)) return fadd(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fsub", 3)) return fsub(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fmul", 3)) return fmul(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fdiv", 3)) return fdiv(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fadds", 3)) return fadds(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fsubs", 3)) return fsubs(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fmuls", 3)) return fmuls(l.fpr(0), l.fpr(1), l.fpr(2));
    if (l.is("fdivs", 3)) return fdivs(l.fpr(0), l.fpr(1), l.fpr(2));

    // Special registers / sync
    if (l.is("mfspr", 2)) return mfspr(l.gpr(0), sprNumber(l.imm(1)));
    if (l.is("mtspr", 2)) return mtspr(sprNumber(l.imm(0)), l.gpr(1));
    if (l.is("mflr", 1)) return mflr(l.gpr(0));
    if (l.is("mtlr", 1)) return mtlr(l.gpr(0));
    if (l.is("mfctr", 1)) return mfctr(l.gpr(0));
    if (l.is("mtctr", 1)) return mtctr(l.gpr(0));
    if (l.is("mfcr", 1)) return mfcr(l.gpr(0));
    if (l.is("sync", 0)) return sync();
    if (l.is("isync", 0)) return isync();
    if (l.is("trap", 0)) return trap();

    // Branch
    if (l.is("b", 1)) return b(l.target(0));
    if (l.is("bl", 1)) return bl(l.target(0));
    if (l.is("blr", 0)) return blr();
    if (l.is("blrl", 0)) return blrl();
    if (l.is("bctr", 0)) return bctr();
    if (l.is("bctrl", 0)) return bctrl();

    encodeError("unknown mnemonic or wrong operand count");
    return 0;
}

} // namespace detail

// Runtime immediate fillers for a compile-time-encoded instruction whose immediate field is 0

constexpr uint32_t hi(uint32_t value) { return value >> 16; }
constexpr uint32_t lo(uint32_t value) { return value & 0xFFFF; }
constexpr uint32_t ha(uint32_t value) { return (value + 0x8000) >> 16; }
constexpr uint32_t branchField(uint32_t displacement) { return displacement & 0x03FFFFFC; }

// Runtime register fillers for a compile-time-encoded instruction whose register field is r0 / f0 / cr0

constexpr uint32_t rtField(uint32_t index) { return (index & 31) << 21; }
constexpr uint32_t raField(uint32_t index) { return (index & 31) << 16; }
constexpr uint32_t rbField(uint32_t index) { return (index & 31) << 11; }
constexpr uint32_t crfField(uint32_t index) { return (index & 7) << 23; }

namespace detail {

template <typename T>
inline uint32_t address(T* pointer) {
    static_assert(sizeof(T*) == 4, "pointer fillers need a 32-bit target");
    return static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

} // namespace detail

template <typename T>
inline uint32_t hi(T* pointer) {
    return hi(detail::address(pointer));
}

template <typename T>
inline uint32_t lo(T* pointer) {
    return lo(detail::address(pointer));
}

template <typename T>
inline uint32_t ha(T* pointer) {
    return ha(detail::address(pointer));
}

// "lwz r3, 0x10(r1)"_ppc encodes one instruction at compile time, in the disassembler's syntax.
// A leading "ADDR:" makes branch operands absolute targets instead of displacements.
inline namespace literals {

consteval uint32_t operator""_ppc(const char* text, std::size_t length) {
    return detail::assemble(detail::AsmLine({text, length}));
}

} // namespace literals

} // namespace ppc
