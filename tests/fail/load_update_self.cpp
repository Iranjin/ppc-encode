#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = lwzu(r3, 4, r3);
int main() { return static_cast<int>(kWord); }
