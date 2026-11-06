#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = stwu(r1, -16, r0);
int main() { return static_cast<int>(kWord); }
