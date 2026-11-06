#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = b(0x2000000);
int main() { return static_cast<int>(kWord); }
