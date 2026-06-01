#include "legacy/porytiles_context.h"

#include <fmt/color.h>
#include <string>

#include "legacy/logger.h"
#include "legacy/porytiles_exception.h"

namespace porytiles_legacy {
void die(const PorytilesContext &ctx, const std::string &errorMessage) {
    if (ctx.printDieMsg) {
        pt_println(stderr, "{}", errorMessage);
    }
    throw PorytilesException{errorMessage};
}

void die_compilationTerminated(const PorytilesContext &ctx, const std::string &srcPath,
                               const std::string &errorMessage) {
    if (ctx.printDieMsg) {
        pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}

void die_compilationTerminatedFailHard(const PorytilesContext &ctx, const std::string &srcPath) {
    if (ctx.printDieMsg) {
        pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    std::exit(1);
}

void die_decompilationTerminated(const PorytilesContext &ctx, const std::string &srcPath,
                                 const std::string &errorMessage) {
    if (ctx.printDieMsg) {
        pt_println(stderr, "terminating decompilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}

void die_errorCount(const PorytilesContext &ctx, const std::string &srcPath, const std::string &errorMessage) {
    if (ctx.printDieMsg) {
        // TODO : display warn and err count here once all errors are migrated
        pt_println(stderr, "terminating compilation of {}", styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}
} // namespace porytiles_legacy