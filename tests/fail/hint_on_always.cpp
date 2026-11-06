#include <ppc/encode.hpp>
using namespace ppc;
using namespace ppc::regs;
constexpr uint32_t kWord = bc(Bo::Always, cr0.lt(), 8, Hint::Taken);
int main() { return static_cast<int>(kWord); }
