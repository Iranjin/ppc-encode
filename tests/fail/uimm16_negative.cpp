#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = ori(r3, r3, -1);
int main() { return static_cast<int>(kWord); }
