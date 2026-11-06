#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = "mfspr r3, 268"_ppc;
int main() { return static_cast<int>(kWord); }
