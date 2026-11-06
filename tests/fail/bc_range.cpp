#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = beq(0x8000);
int main() { return static_cast<int>(kWord); }
