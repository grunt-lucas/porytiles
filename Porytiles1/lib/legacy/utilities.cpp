#include "legacy/utilities.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "legacy/logger.h"
#include "legacy/porytiles_context.h"
#include "legacy/types.h"
#include "panic/panic.hpp"

#include <regex>

namespace porytiles1 {

std::vector<std::string> split(std::string input, const std::string &delimiter) {
    std::vector<std::string> result;
    size_t pos;
    std::string token;
    while ((pos = input.find(delimiter)) != std::string::npos) {
        token = input.substr(0, pos);
        result.push_back(token);
        input.erase(0, pos + delimiter.length());
    }
    result.push_back(input);
    return result;
}

bool checkFullStringMatch(const std::string &str, const std::string &pattern) {
    try {
        const std::regex re{pattern};
        return std::regex_match(str, re);
    } catch (const std::regex_error &e) {
        throw std::runtime_error{e.what()};
    }
}

void trim(std::string &string) {
    string.erase(string.begin(),
                 std::find_if(string.begin(), string.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    string.erase(
        std::find_if(string.rbegin(), string.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
        string.end());
}

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName) {
    return std::filesystem::temp_directory_path() / parentDir / fileName;
}

std::filesystem::path createTmpdir() {
    int maxTries = 1000;
    auto tmpDir = std::filesystem::temp_directory_path();
    int i = 0;
    std::random_device randomDevice;
    std::mt19937 mersennePrng(randomDevice());
    std::uniform_int_distribution<uint64_t> uniformIntDistribution(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream stringStream;
        stringStream << std::hex << uniformIntDistribution(mersennePrng);
        path = tmpDir / ("porytiles_" + stringStream.str());
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == maxTries) {
            Panic("tmpfiles::createTmpdir getTmpdirPath took too many tries");
        }
        i++;
    }
    return path;
}

static RGBA32 parseJascLine(PorytilesContext &ctx, const CompilerMode *compilerMode,
                            const DecompilerMode *decompilerMode, const std::string &jascLine,
                            const std::string &fileName) {
    std::vector<std::string> colorComponents = split(jascLine, " ");
    if (colorComponents.size() != 3) {
        if (compilerMode != nullptr && decompilerMode != nullptr) {
            Panic("utilities::parseJascLine both mode parameters were non-null");
        }
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("expected valid JASC line in pal file {}, saw {}", fileName, jascLine);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("expected valid JASC line in pal file {}, saw {}", fileName, jascLine);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("utilities::parseJascLine both mode parameters were null");
    }

    if (colorComponents[0].at(colorComponents[0].size() - 1) == '\r') {
        colorComponents[0].pop_back();
    }
    if (colorComponents[1].at(colorComponents[1].size() - 1) == '\r') {
        colorComponents[1].pop_back();
    }
    if (colorComponents[2].at(colorComponents[2].size() - 1) == '\r') {
        colorComponents[2].pop_back();
    }

    int red = parseInteger<int>(colorComponents[0].c_str());
    int green = parseInteger<int>(colorComponents[1].c_str());
    int blue = parseInteger<int>(colorComponents[2].c_str());

    if (red < 0 || red > 255) {
        if (compilerMode != nullptr && decompilerMode != nullptr) {
            Panic("utilities::parseJascLine both mode parameters were non-null");
        }
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid red component: range must be 0 <= red <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid red component: range must be 0 <= red <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("utilities::parseJascLine both mode parameters were null");
    }
    if (green < 0 || green > 255) {
        if (compilerMode != nullptr && decompilerMode != nullptr) {
            Panic("utilities::parseJascLine both mode parameters were non-null");
        }
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid green component: range must be 0 <= green <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid green component: range must be 0 <= green <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("utilities::parseJascLine both mode parameters were null");
    }
    if (blue < 0 || blue > 255) {
        if (compilerMode != nullptr && decompilerMode != nullptr) {
            Panic("utilities::parseJascLine both mode parameters were non-null");
        }
        if (compilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid blue component: range must be 0 <= blue <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode), msg);
        }
        if (decompilerMode != nullptr) {
            const auto msg = fmt::format("{}: invalid blue component: range must be 0 <= blue <= 255", fileName);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode), msg);
        }
        Panic("utilities::parseJascLine both mode parameters were null");
    }

    return RGBA32{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                  ALPHA_OPAQUE};
}

RGBA32 parseJascLineCompiler(PorytilesContext &ctx, CompilerMode compilerMode, const std::string &jascLine,
                             const std::string &fileName) {
    return parseJascLine(ctx, &compilerMode, nullptr, jascLine, fileName);
}

RGBA32 parseJascLineDecompiler(PorytilesContext &ctx, DecompilerMode decompilerMode, const std::string &jascLine,
                               const std::string &fileName) {
    return parseJascLine(ctx, nullptr, &decompilerMode, jascLine, fileName);
}

std::string palIndexToFileName(std::size_t index) {
    std::string file = std::to_string(index) + ".png";
    if (index < 10) {
        file = "0" + file;
    }
    return file;
}

#ifndef DOCTEST_CONFIG_DISABLE
void doctestAssertFileBytesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath) {
    REQUIRE(std::filesystem::exists(expectedPath));
    REQUIRE(std::filesystem::exists(actualPath));
    std::FILE *expected;
    std::FILE *actual;
    expected = fopen(expectedPath.string().c_str(), "r");
    if (expected == NULL) {
        FAIL("std::FILE `expected' was null");
    }
    actual = fopen(actualPath.string().c_str(), "r");
    if (actual == NULL) {
        fclose(expected);
        FAIL("std::FILE `expected' was null");
    }
    fseek(expected, 0, SEEK_END);
    long expectedSize = ftell(expected);
    rewind(expected);
    fseek(actual, 0, SEEK_END);
    long actualSize = ftell(actual);
    rewind(actual);
    CHECK(expectedSize == actualSize);

    std::uint8_t expectedByte;
    std::uint8_t actualByte;
    std::size_t bytesRead;
    for (long i = 0; i < actualSize; i++) {
        bytesRead = fread(&expectedByte, 1, 1, expected);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        bytesRead = fread(&actualByte, 1, 1, actual);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        CHECK(expectedByte == actualByte);
    }
    fclose(expected);
    fclose(actual);
}

void doctestAssertFileLinesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath) {
    REQUIRE(std::filesystem::exists(expectedPath));
    REQUIRE(std::filesystem::exists(actualPath));

    std::string expectedLine;
    std::string actualLine;
    std::string dummy;
    std::size_t expectedLinesCount = 0;
    std::size_t actualLinesCount = 0;
    std::ifstream expected{expectedPath};
    std::ifstream actual{actualPath};

    while (std::getline(expected, dummy)) {
        expectedLinesCount++;
    }
    while (std::getline(actual, dummy)) {
        actualLinesCount++;
    }
    expected.close();
    expected.clear();
    actual.close();
    actual.clear();
    CHECK(expectedLinesCount == actualLinesCount);

    expected.open(expectedPath);
    actual.open(actualPath);
    while (std::getline(expected, expectedLine) && std::getline(actual, actualLine)) {
        CHECK(expectedLine == actualLine);
    }
}
#endif // DOCTEST_CONFIG_DISABLE

} // namespace porytiles1
