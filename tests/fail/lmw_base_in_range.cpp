#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = lmw(r26, 8, r28);
int main() { return static_cast<int>(kWord); }
