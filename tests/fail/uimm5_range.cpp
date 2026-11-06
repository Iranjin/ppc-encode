#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = rlwinm(r3, r4, 32, 0, 31);
int main() { return static_cast<int>(kWord); }
