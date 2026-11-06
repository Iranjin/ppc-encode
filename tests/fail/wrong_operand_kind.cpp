#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = "lwz r3, r4"_ppc;
int main() { return static_cast<int>(kWord); }
