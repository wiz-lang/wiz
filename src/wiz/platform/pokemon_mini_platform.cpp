#if 0
#include <string>
#include <cstddef>
#include <unordered_map>
#include <memory>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/utility/report.h>
#include <wiz/utility/misc.h>

#include <wiz/platform/pokemon_mini_platform.h>

namespace wiz {
    PokemonMiniPlatform::PokemonMiniPlatform() {}
    PokemonMiniPlatform::~PokemonMiniPlatform() {}

    void PokemonMiniPlatform::reserveDefinitions(Builtins& builtins) {
        // https://www.pokemon-mini.net/documentation/

        // TODO: actual instruction set.
        // WIP.

        builtins.addDefineBoolean("__family_pokemon_mini"_sv, true);
        builtins.addDefineBoolean("__cpu_pokemon_mini"_sv, true);

        auto stringPool = builtins.getStringPool();
        auto scope = builtins.getBuiltinScope();
        const auto decl = builtins.getBuiltinDeclaration();
        const auto u8Type = builtins.getDefinition(Builtins::DefinitionType::U8);
        const auto u16Type = builtins.getDefinition(Builtins::DefinitionType::U16);
        const auto u24Type = builtins.getDefinition(Builtins::DefinitionType::U24);
        const auto boolType = builtins.getDefinition(Builtins::DefinitionType::Bool);

        pointerSizedType = u16Type;
        farPointerSizedType = u24Type;

        // Registers.
        a = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("a"), decl);
        ba = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("ba"), decl);
        hl = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("hl"), decl);
        x = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("x"), decl);
        y = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("y"), decl);
        sp = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("sp"), decl);
        const auto patternA = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(a));
        const auto patternB = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("b"), decl)));
        const auto patternH = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("h"), decl)));
        const auto patternL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("l"), decl)));
        const auto patternXI = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("xi"), decl)));
        const auto patternYI = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("yi"), decl)));
        const auto patternI = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("i"), decl)));
        const auto patternN = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("n"), decl)));
        const auto patternU = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("u"), decl)));
        const auto patternV = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("v"), decl)));
        const auto patternF = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("f"), decl)));
        const auto patternBA = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(ba));
        const auto patternHL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(hl));
        const auto patternX = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(x));
        const auto patternY = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(y));
        const auto patternSP = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(sp));
        const auto patternPC = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("pc"), decl)));
        const auto patternYIY = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u24Type), stringPool->intern("xix"), decl)));
        const auto patternXIX = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u24Type), stringPool->intern("yiy"), decl)));
        carry = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("carry"), decl);
        zero = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("zero"), decl);
        overflow = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("overflow"), decl);
        negative = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("negative"), decl);
        const auto patternZero = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(zero));
        const auto patternCarry = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(carry));
        const auto patternOverflow = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(overflow));
        const auto patternNegative = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(negative));

        // Intrinsics.
        const auto push = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push"), decl);
        const auto pop = scope->createDefinition(nullptr, Definition::BuiltinLoadIntrinsic(u16Type), stringPool->intern("pop"), decl);
        const auto nop = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("nop"), decl);
        const auto swap = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap"), decl);
        const auto swap_digits = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap_digits"), decl);
        const auto pack_digits = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("pack_digits"), decl);
        const auto unpack_digits = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("unpack_digits"), decl);
        const auto sign_extend = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("sign_extend"), decl);
        const auto halt = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("halt"), decl);
        const auto stop = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("stop"), decl);
        const auto divmod = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("divmod"), decl);
        const auto push_registers = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push_registers"), decl);
        const auto push_registers_ex = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push_registers_ex"), decl);
        const auto pop_registers = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("pop_registers"), decl);
        const auto pop_registers_ex = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("pop_registers_ex"), decl);
        test = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("test"), decl);
        cmp = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("cmp"), decl);

        // Non-register operands.
        const auto patternFalse = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(false));
        const auto patternTrue = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(true));
        const auto patternAtLeast0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(0)));
        const auto patternAtLeast1 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(1)));
        const auto patternHLIndirect = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternHL->clone(), 1));
        const auto patternXIXIndirect = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternXIX->clone(), 1));
        const auto patternYIYIndirect = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternYIY->clone(), 1));
        const auto patternImmU8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFF)));
        const auto patternImmU16 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFFFF)));
        const auto patternImmI8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(-0x80), Int128(0x7F)));
        const auto patternNRelativeU8Indirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternN->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                1, 1));
        const auto patternXIXPlusLIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternXIX->clone(),
                patternL->clone(),
                1, 1));
        const auto patternYIYPlusLIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternYIY->clone(),
                patternL->clone(),
                1, 1));
        const auto patternXIXRelativeI8Indirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternXIX->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmI8->clone())),
                1, 1));
        const auto patternYIYRelativeI8Indirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternYIY->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmI8->clone())),
                1, 1));
        const auto patternSPRelativeI8Indirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternSP->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmI8->clone())),
                1, 1));
        const auto patternAbsU8
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                1));
        const auto patternAbsU16
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                2));

        // Instruction encodings.
        const auto encodingImplicit = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size();
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(captureLists);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                return true;
            });
        const auto encodingU8Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 1;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                buffer.push_back(static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value));
                return true;
            });
        const auto encodingU16Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 2;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                const auto value = static_cast<std::uint16_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
                buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
                return true;
            });
        const auto encodingU8OperandU8Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 2;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                buffer.push_back(static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value));
                buffer.push_back(static_cast<std::uint8_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value));
                return true;
            });
        const auto encodingPCRelativeI8Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 1;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());

                const auto base = static_cast<std::int32_t>(bank->getAddress().absolutePosition.get() & 0xFFFF);
                const auto dest = static_cast<std::int32_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                const auto offset = dest - base - 2;
                if (offset >= -128 && offset <= 127) {
                    buffer.push_back(offset < 0
                        ? (static_cast<std::uint8_t>(-offset) ^ 0xFF) + 1
                        : static_cast<std::uint8_t>(offset));
                } else {
                    buffer.push_back(0);
                    report->error("pc-relative offset is outside of representable signed 8-bit range -128..127", location);
                    return false;
                }
                return true;
            });
        const auto encodingPCRelativeI16Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 2;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());

                const auto base = static_cast<std::int32_t>(bank->getAddress().absolutePosition.get());
                const auto dest = static_cast<std::int32_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                const auto offset = dest - base - 3;
                if (offset >= -32768 && offset <= 32767) {
                    std::uint16_t value = offset < 0
                        ? (static_cast<std::uint16_t>(-offset) ^ 0xFFFF) + 1
                        : static_cast<std::uint16_t>(offset);
                    buffer.push_back(static_cast<std::uint8_t>(value));
                    buffer.push_back(static_cast<std::uint8_t>(value >> 8));
                    return true;
                } else {
                    buffer.push_back(0);
                    buffer.push_back(0);
                    report->error("pc-relative offset is outside of representable signed 8-bit range -32768..32767", location);
                    return false;
                }
            });
        const auto encodingI8Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 1;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(bank);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());

                const auto i8val = static_cast<int>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                if (i8val >= -128 && i8val <= 127) {
                    buffer.push_back(i8val < 0
                        ? (static_cast<std::uint8_t>(-i8val) ^ 0xFF) + 1
                        : static_cast<std::uint8_t>(i8val));
                } else {
                    buffer.push_back(0);
                    report->error("signed value is outside of representable signed 8-bit range -128..127", location);
                    return false;
                }
                return true;
            });
        const auto encodingRepeatedImplicit = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                return static_cast<std::size_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value) * options.opcode.size();
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                const auto count = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                for (std::size_t i = 0; i != count; ++i) {
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                }
                return true;
            });
        const auto encodingRepeatedU8Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                return static_cast<std::size_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value) * (options.opcode.size() + 1);
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                const auto value = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                const auto count = static_cast<std::uint8_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value);
                for (std::size_t i = 0; i != count; ++i) {
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer.push_back(value);
                }
                return true;
            });

        // Instructions.
        // TODO.
        // mov
        {
            using Load8OperandInfo = std::tuple<InstructionOperandPattern*, std::uint8_t, std::uint8_t>;
            const Load8OperandInfo load8Operands[] {
                Load8OperandInfo {patternA, 0, 0},
                Load8OperandInfo {patternB, 1, 1},
                Load8OperandInfo {patternL, 2, 2},
                Load8OperandInfo {patternH, 3, 3},
                Load8OperandInfo {patternNRelativeU8Indirect, 4, 7},
                Load8OperandInfo {patternHLIndirect, 5, 6},
                Load8OperandInfo {patternXIXIndirect, 6, 4},
                Load8OperandInfo {patternYIYIndirect, 7, 5},
            };
            for (const auto& destOperand : load8Operands) {
                for (const auto& sourceOperand : load8Operands) {
                    if (std::get<0>(destOperand) == std::get<0>(sourceOperand)) {
                        continue;
                    }

                    const auto opcode = static_cast<std::uint8_t>(0x40 | (std::get<1>(destOperand) << 3) | std::get<2>(sourceOperand));
                    if (std::get<0>(destOperand) == patternNRelativeU8Indirect || std::get<0>(sourceOperand) == patternNRelativeU8Indirect) {
                        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {std::get<0>(destOperand), std::get<0>(sourceOperand)}), encodingU8Operand, InstructionOptions({opcode}, {1}, {}));
                    } else {
                        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {std::get<0>(destOperand), std::get<0>(sourceOperand)}), encodingImplicit, InstructionOptions({opcode}, {}, {}));
                    }
                }
            }
        }

        // mov f, nn (9F)
        // mov {a|b|l|h|n|[hl]|[x]|[y]}, #nn (B0 + index, nn)
        // mov [n+#nn], #nn (DD, #nn, #nn)
        // mov {u|i|xi|yi}, #nn (CE, (C4 + index), #nn)
        // mov a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 40 + index, #ss)
        // mov {[x+#ss]|[y+#ss]|[x+l]|[y+l]}, a (CE, 44 + index, #ss)
        // mov b, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 48 + index, #ss)
        // mov {[x+#ss]|[y+#ss]|[x+l]|[y+l]}, b (CE, 4C + index, #ss)
        // mov l, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 50 + index, #ss)
        // mov {[x+#ss]|[y+#ss]|[x+l]|[y+l]}, l (CE, 54 + index, #ss)
        // mov h, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 58 + index, #ss)
        // mov {[x+#ss]|[y+#ss]|[x+l]|[y+l]}, h (CE, 5C + index, #ss)
        // mov [hl], {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 60 + index, #ss)
        // mov [x], {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 68 + index, #ss)
        // mov [y], {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 78 + index, #ss)
        // mov ba, [#nnnn] (B8, nn, nn)
        // mov hl, [#nnnn] (B9, nn, nn)
        // mov x, [#nnnn] (BA, nn, nn)
        // mov y, [#nnnn] (BB, nn, nn)
        // mov sp, [#nnnn] (CF, 78, nn, nn)
        // mov [#nnnn], ba (B8, nn, nn)
        // mov hl, [#nnnn], hl (C5, nn, nn)
        // mov [#nnnn], x (C6, nn, nn)
        // mov [#nnnn], y (C7, nn, nn)
        // mov [#nnnn], sp (CF, 6E, nn, nn)
        // mov a, n (CE, C0)
        // mov a, f (CE, C1)
        // mov n, a (CE, C2)
        // mov f, a (CE, C3)
        // mov a, {v|i|xi|yi} (CE, C8 + index) -- v when read
        // mov  {u|i|xi|yi}, a (CE, CC + index) -- u when written
        // mov {a|b|l|h}, [#nnnn] (CE, D0 + index, nn, nn)
        // mov [#nnnn], {a|b|l|h} (CE, D4 + index, nn, nn)
        // mov {ba|hl|x|y}, [sp+#ss] (CF, 70+index, ss)
        // mov [sp+#ss], {ba|hl|x|y} (CF, 74+index, ss)
        // mov {ba|hl|x|y}, [hl] (CF, C0+index, ss)
        // mov {ba|hl|x|y}, [x] (CF, D0+index, ss)
        // mov {ba|hl|x|y}, [y] (CF, D8+index, ss)
        // mov [hl], {ba|hl|x|y}  (CF, C4+index, ss)
        // mov [x], {ba|hl|x|y} (CF, D4+index, ss)
        // mov [y], {ba|hl|x|y} (CF, DC+index, ss)
        // mov ba, {ba|hl|x|y} (CF, E0+index, ss)
        // mov hl, {ba|hl|x|y} (CF, E4+index, ss)
        // mov x, {ba|hl|x|y} (CF, E8+index, ss)
        // mov y, {ba|hl|x|y} (CF, EC+index, ss)
        // mov sp, {ba|hl|x|y} (CF, F0+index, ss)
        // mov hl, {sp|pc} (CF, F4+index, ss)
        // mov ba, {sp|pc} (CF, F8+index, ss)
        // mov {x|y}, sp (CF, FA+index, ss)



        // add
        // add a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (00+index, nn, nn)
        // add a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 00+index, #ss)
        // add {ba|hl|x|y}, #nnnn (C0+index, nn, nn)
        // add sp, #nnnn (CF, 68, nn, nn)
        // add [hl], {a|#nn|[x]|[y]} (CE, 04+index, nn)
        // add ba, {ba|hl|x|y} (CF, 00+index)
        // add hl, {ba|hl|x|y} (CF, 20+index)
        // add sp, {ba|hl} (CF, 44+index)
        // add x, {ba|hl} (CF, 40+index)
        // add y, {ba|hl} (CF, 42+index)
        //
        // sub
        // sub a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (10+index, nn, nn)
        // sub a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 10+index, #ss)
        // sub {ba|hl|x|y}, #nnnn (D0+index, nn, nn)
        // sub sp, #nnnn (CF, 6A, nn, nn)
        // sub [hl], {a|#nn|[x]|[y]} (CE, 14+index, nn)
        // sub ba, {ba|hl|x|y} (CF, 08+index)
        // sub hl, {ba|hl|x|y} (CF, 28+index)
        // sub sp, {ba|hl} (CF, 4C+index)
        // sub x, {ba|hl} (CF, 48+index)
        // sub y, {ba|hl} (CF, 4A+index)
        //
        // cmp
        // cmp a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (30+index, nn, nn)
        // cmp a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 30+index, #ss)
        // cmp {ba|hl|x|y}, #nnnn (D4+index, nn, nn)
        // cmp sp, #nnnn (CF, 6C, nn, nn)
        // cmp [hl], {a|#nn|[x]|[y]} (CE, 34+index, nn)
        // cmp ba, {ba|hl|x|y} (CF, 18+index)
        // cmp hl, {ba|hl|x|y} (CF, 38+index)
        // cmp sp, {ba|hl} (CF, 5C+index)
        // cmp [n+#nn], #nn (DB, nn, nn)
        // cmp {b|l|h|n}, #nn (CE, BC+index, nn)
        //
        // adc
        // adc a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (08+index, nn, nn)
        // adc a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 08+index, #ss)
        // adc [hl], {a|#nn|[x]|[y]} (CE, 0C+index, nn)
        // adc ba, {ba|hl|x|y} (CF, 04+index)
        // adc hl, {ba|hl|x|y} (CF, 24+index)
        // adc {ba|hl}, #nnnn (CF, 60+index)
        //
        // sbc
        // sbc a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (18+index, nn, nn)
        // sbc a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 18+index, #ss)
        // sbc [hl], {a|#nn|[x]|[y]} (CE, 1C+index, nn)
        // sbc ba, {ba|hl|x|y} (CF, 0C+index)
        // sbc hl, {ba|hl|x|y} (CF, 2C+index)
        // sbc {ba|hl}, #nnnn (CF, 62+index)
        //
        // and
        // and a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (20+index, nn, nn)
        // and a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 20+index, #ss)
        // and [hl], {a|#nn|[x]|[y]} (CE, 24+index, nn)
        // and f, #nn (9C, nn)
        // and {b|l|h}, #nn (CE, B0 + index, nn)
        // and [n+#nn], #nn (D8, nn, nn)
        //
        // or
        // or a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (28+index, nn, nn)
        // or a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 28+index, #ss)
        // or [hl], {a|#nn|[x]|[y]} (CE, 2C+index, nn)
        // or f, #nn (9D, nn)
        // or {b|l|h}, #nn (CE, B4 + index, nn)
        // or [n+#nn], #nn (D9, nn, nn)
        //
        // xor
        // xor a, {a|b|#nn|[hl]|[n+#nn]|[#nnnn]|[x]|[y]} (38+index, nn, nn)
        // xor a, {[x+#ss]|[y+#ss]|[x+l]|[y+l]} (CE, 38+index, #ss)
        // xor [hl], {a|#nn|[x]|[y]} (CE, 3C+index, nn)
        // xor f, #nn (9E, nn)
        // xor {b|l|h}, #nn (CE, B8 + index, nn)
        // xor [n+#nn], #nn (DA, nn, nn)
        {
            struct ArithmericOperatorInfo {
                InstructionType type;
                std::uint8_t accumulatorByTermOpcode;
                std::uint8_t reg16ByImmU16Opcode;
                std::uint8_t spByImmU16Opcode;
                std::uint8_t hlIndirectByTermOpcode;
                std::uint8_t baByTermOpcode;
                std::uint8_t hlByTermOpcode;
                std::uint8_t spByTermOpcode;
                std::uint8_t xByTermOpcode;
                std::uint8_t yByTermOpcode;
                std::uint8_t baOrHLByImmU16Opcode;

                ArithmericOperatorInfo(
                    InstructionType type,
                    std::uint8_t accumulatorByTermOpcode,
                    std::uint8_t reg16ByImmU16Opcode,
                    std::uint8_t spByImmU16Opcode,
                    std::uint8_t hlIndirectByTermOpcode,
                    std::uint8_t baByTermOpcode,
                    std::uint8_t hlByTermOpcode,
                    std::uint8_t spByTermOpcode,
                    std::uint8_t xByTermOpcode,
                    std::uint8_t yByTermOpcode,
                    std::uint8_t baOrHLByImmU16Opcode)
                : type(type),
                accumulatorByTermOpcode(accumulatorByTermOpcode),
                reg16ByImmU16Opcode(reg16ByImmU16Opcode),
                spByImmU16Opcode(spByImmU16Opcode),
                hlIndirectByTermOpcode(hlIndirectByTermOpcode),
                baByTermOpcode(baByTermOpcode),
                hlByTermOpcode(hlByTermOpcode),
                spByTermOpcode(spByTermOpcode),
                xByTermOpcode(xByTermOpcode),
                yByTermOpcode(yByTermOpcode),
                baOrHLByImmU16Opcode(baOrHLByImmU16Opcode) {}
            };

            const ArithmericOperatorInfo arithmeticOperators[] {
                ArithmericOperatorInfo {BinaryOperatorKind::Addition, 0x00, 0xC0, 0x68, 0x04, 0x00, 0x20, 0x44, 0x40, 0x42, 0xFF},
                ArithmericOperatorInfo {BinaryOperatorKind::AdditionWithCarry, 0x08, 0xFF, 0xFF, 0x0C, 0x04, 0x24, 0xFF, 0xFF, 0xFF, 0x60},
                ArithmericOperatorInfo {BinaryOperatorKind::Subtraction, 0x10, 0xD0, 0x6A, 0x14, 0x08, 0x28, 0x4C, 0x48, 0x4A, 0xFF},
                ArithmericOperatorInfo {BinaryOperatorKind::SubtractionWithCarry, 0x18, 0xFF, 0xFF, 0x1C, 0x0C, 0x2C, 0xFF, 0xFF, 0xFF, 0x62},
                ArithmericOperatorInfo {BinaryOperatorKind::BitwiseAnd, 0x20, 0xFF, 0xFF, 0x24, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                ArithmericOperatorInfo {BinaryOperatorKind::BitwiseOr, 0x28, 0xFF, 0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                ArithmericOperatorInfo {InstructionType::VoidIntrinsic(cmp), 0x30, 0xD4, 0x6C, 0x34, 0x18, 0x38, 0x5C, 0xFF, 0xFF, 0xFF},
                ArithmericOperatorInfo {BinaryOperatorKind::BitwiseXor, 0x38, 0xFF, 0xFF, 0x3C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            };


        // add x, {ba|hl} (CF, 40+index)
        // add y, {ba|hl} (CF, 42+index)

        // add ba, {ba|hl|x|y} (CF, 00+index)
        // add hl, {ba|hl|x|y} (CF, 20+index)
        // add sp, {ba|hl} (CF, 44+index)


            InstructionOperandPattern* const accumulatorOpTermDests[] {
                patternA,
                patternB,
                patternImmU8,
                patternHLIndirect,
                patternNRelativeU8Indirect,
                patternAbsU8,
                patternXIXIndirect,
                patternYIYIndirect,
            };

            InstructionOperandPattern* const accumulatorOpIndexedTermDests[] {
                patternXIXRelativeI8Indirect,
                patternYIYRelativeI8Indirect,
                patternXIXPlusLIndirect,
                patternYIYPlusLIndirect,
            };

            InstructionOperandPattern* const reg16OpImmU16TermSources[] {
                patternBA,
                patternHL,
                patternX,
                patternY,
            };

            InstructionOperandPattern* const hlIndirectOpTermDests[] {
                patternA,
                patternImmU8,
                patternXIXIndirect,
                patternYIYIndirect,
            };

            InstructionOperandPattern* const baOrHLByTermDests[] {
                patternBA,
                patternHL,
                patternX,
                patternY,
            };

            InstructionOperandPattern* const spOrXOrYByTermDests[] {
                patternBA,
                patternHL,
            };

            for (const auto& op : arithmeticOperators) {
                {
                    std::uint8_t index = 0;
                    for (auto operand : accumulatorOpTermDests) {
                        auto encoding = encodingImplicit;
                        if (operand == patternAbsU8) {
                            encoding = encodingU16Operand;
                        } else if (operand) {
                            encoding = encodingU8Operand;
                        }
                        builtins.createInstruction(InstructionSignature(op.type, 0, {patternA, operand}), encoding, InstructionOptions({static_cast<std::uint8_t>(op.accumulatorByTermOpcode | index)}, {1}, {}));
                        ++index;
                    }
                }

                {
                    std::uint8_t index = 0;
                    for (const auto& operand : accumulatorOpIndexedTermDests) {
                        auto encoding = encodingImplicit;
                        if (operand == patternXIXRelativeI8Indirect || operand == patternYIYRelativeI8Indirect) {
                            encoding = encodingI8Operand;
                        } else if (operand) {
                            encoding = encodingU8Operand;
                        }
                        builtins.createInstruction(InstructionSignature(op.type, 0, {patternA, operand}), encoding, InstructionOptions({0xCE, static_cast<std::uint8_t>(op.accumulatorByTermOpcode | index)}, {1}, {}));
                        ++index;
                    }
                }

                if (op.reg16ByImmU16Opcode != 0xFF) {
                    std::uint8_t index = 0;
                    for (const auto& operand : reg16OpImmU16TermSources) {
                        builtins.createInstruction(InstructionSignature(op.type, 0, {operand, patternImmU16}), encodingU16Operand, InstructionOptions({0xCE, static_cast<std::uint8_t>(op.reg16ByImmU16Opcode | index)}, {1}, {}));
                        ++index;
                    }
                }

                if (op.spByImmU16Opcode != 0xFF) {
                    builtins.createInstruction(InstructionSignature(op.type, 0, {patternSP, patternImmU16}), encodingU16Operand, InstructionOptions({0xCF, op.spByImmU16Opcode}, {1}, {}));
                }

                {
                    std::uint8_t index = 0;
                    for (const auto& operand : hlIndirectOpTermDests) {
                        const auto encoding = operand == patternImmU8 ? encodingImplicit : encodingU8Operand;
                        builtins.createInstruction(InstructionSignature(op.type, 0, {patternHLIndirect, operand}), encoding, InstructionOptions({0xCE, static_cast<std::uint8_t>(op.hlIndirectByTermOpcode | index)}, {1}, {}));
                        ++index;
                    }
                }

                {
                    std::uint8_t index = 0;
                    for (const auto& operand : hlIndirectOpTermDests) {
                        const auto encoding = operand == patternImmU8 ? encodingImplicit : encodingU8Operand;
                        builtins.createInstruction(InstructionSignature(op.type, 0, {patternHLIndirect, operand}), encoding, InstructionOptions({0xCE, static_cast<std::uint8_t>(op.hlIndirectByTermOpcode | index)}, {1}, {}));
                        ++index;
                    }
                }
            }
        }

        // test
        // tst a, b (94)
        // tst {[hl]|a|b}, #nn (95+index, #nn)
        // tst [n+#nn], #nn (DC, #nn, #nn)

        // mul: hl = l * a (CE D8)
        // div: hl = divmod(hl, a) (CE D9)
        // not: not {a|b|[n+#nn]|[hl] (CE, A0+index, nn)
        // neg: neg {a|b|[n+#nn]|[hl] (CE, A4+index, nn)
        // shift left logical: shl {a|b|[n+#nn]|[hl] (CE, 84+index, nn)
        // shift left arithmetic: sal {a|b|[n+#nn]|[hl] (CE, 80+index, nn)
        // shift right logical: shr {a|b|[n+#nn]|[hl] (CE, 8C+index, nn)
        // shift right arithmetic: sar {a|b|[n+#nn]|[hl] (CE, 88+index, nn)
        // rotate left: rol {a|b|[n+#nn]|[hl] (CE, 94+index, nn)
        // rotate right: ror {a|b|[n+#nn]|[hl] (CE, 9C+index, nn)
        // rotate left with carry: rolc {a|b|[n+#nn]|[hl] (CE, 90+index, nn)
        // rotate right with carry:: rorc {a|b|[n+#nn]|[hl] (CE, 98+index, nn)

        // inc
        // ++{a|b|l|h|n|[n+#nn]|[hl]|sp} (80+index)
        // ++{ba|hl|x|y} (90+index)
        // dec
        // --{a|b|l|h|n|[n+#nn]|[hl]|sp} (88+index)
        // --{ba|hl|x|y} (98+index)
        // swap:
        // swap(ba, {hl|x|y|sp}) (C8+index)
        // swap(a, {b,[hl]}) (CD+index)
        // pack_digits() (DE)
        // unpack_digits() (DF)
        // swap_digits: swap_digits({a|[hl]}); (F6+index)
        // sign_extend: ba = sign_extend(a); (CE, A8)

        // push
        // push({ba|hl|x|y|n|i|x|f}) (A0+index)
        // push({a|b|l|h}) (CF, B0+index)
        // push_registers() (B8)
        // push_registers_ex() (B9)

        // pop
        // {ba|hl|x|y|n|i|x|f} = pop(); (A8+index)
        // {a|b|l|h} = pop(); (CF, B4+index)
        // pop_registers(); (CF, BC)
        // pop_registers_ex(); (CF, BD)

        // nop() (FF)
        // halt() (CE, AE)
        // stop() (CE, AF)

        // goto #ss
        // goto #ss if carry
        // goto #ss if !carry
        // goto #ss if zero
        // goto #ss if !zero
        // goto #ss if exception0
        // goto #ss if !exception0
        // goto #ss if exception1
        // goto #ss if !exception1
        // goto #ss if exception2
        // goto #ss if !exception2
        // goto #ss if exception3
        // goto #ss if !exception3
        // goto #ssss
        // goto #ssss if carry
        // goto #ssss if !carry
        // goto #ssss if zero
        // goto #ssss if !zero
        // goto #ssss if signed <
        // goto #ssss if signed <=
        // goto #ssss if signed >
        // goto #ssss if signed >=
        // goto #ssss if overflow
        // goto #ssss if !overflow
        // goto #ssss if negative
        // goto #ssss if !negative
        // goto hl
        // irqgoto(#nn)
        
        // call #ss
        // call #ss if carry
        // call #ss if !carry
        // call #ss if zero
        // call #ss if !zero
        // call #ss if exception0
        // call #ss if !exception0
        // call #ss if exception1
        // call #ss if !exception1
        // call #ss if exception2
        // call #ss if !exception2
        // call #ss if exception3
        // call #ss if !exception3
        // call #ssss
        // call #ssss if carry
        // call #ssss if !carry
        // call #ssss if zero
        // call #ssss if !zero
        // call #ssss if signed <
        // call #ssss if signed <=
        // call #ssss if signed >
        // call #ssss if signed >=
        // call #ssss if overflow
        // call #ssss if !overflow
        // call #ssss if negative
        // call #ssss if !negative
        // call [#nnnn]
        // irqcall(#nn)

        // return
        // irqreturn
        // retskip
    }

    Definition* PokemonMiniPlatform::getPointerSizedType() const {
        return pointerSizedType;
    }

    Definition* PokemonMiniPlatform::getFarPointerSizedType() const {
        return farPointerSizedType;
    }

    std::unique_ptr<PlatformTestAndBranch> PokemonMiniPlatform::getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const {
        static_cast<void>(compiler);
        static_cast<void>(op);
        static_cast<void>(left);
        static_cast<void>(right);
        static_cast<void>(distanceHint);

        // TODO: unsigned comparison instructions.
        // TODO: signed comparison instructions.
        // TODO: bitwise test instructions.
        return nullptr;
    }

    Definition* PokemonMiniPlatform::getZeroFlag() const {
        return zero;
    }

    Int128 PokemonMiniPlatform::getPlaceholderValue() const {
        return Int128(UINT64_C(0xCCCCCCCCCCCCCCCC));
    }
}
#endif