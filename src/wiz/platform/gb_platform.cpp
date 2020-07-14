#include <string>
#include <cstddef>
#include <unordered_map>
#include <memory>

#include <wiz/ast/expression.h>
#include <wiz/ast/type_expression.h>
#include <wiz/ast/statement.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/compiler.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/utility/report.h>
#include <wiz/utility/misc.h>
#include <wiz/platform/gb_platform.h>

namespace wiz {
    GameBoyPlatform::GameBoyPlatform() {}
    GameBoyPlatform::~GameBoyPlatform() {}

    void GameBoyPlatform::reserveDefinitions(Builtins& builtins) {
        // http://problemkaputt.de/pandocs.htm#cpuinstructionset
        // http://imrannazar.com/Gameboy-Z80-Opcode-Map

        builtins.addDefineBoolean("__cpu_gb"_sv, true);

        auto stringPool = builtins.getStringPool();
        auto scope = builtins.getBuiltinScope();
        const auto decl = builtins.getBuiltinDeclaration();
        const auto u8Type = builtins.getDefinition(Builtins::DefinitionType::U8);
        const auto u16Type = builtins.getDefinition(Builtins::DefinitionType::U16);
        const auto u24Type = builtins.getDefinition(Builtins::DefinitionType::U24);
        const auto boolType = builtins.getDefinition(Builtins::DefinitionType::Bool);

        bitIndex7Expression = makeFwdUnique<Expression>(
            Expression::IntegerLiteral(Int128(7)),
            decl->location, 
            ExpressionInfo(EvaluationContext::CompileTime, 
                makeFwdUnique<TypeExpression>(TypeExpression::ResolvedIdentifier(u8Type), decl->location),
                Qualifiers {}));

        pointerSizedType = u16Type;
        farPointerSizedType = u24Type;

        // Registers.
        a = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("a"), decl);
        const auto b = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("b"), decl);
        const auto c = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("c"), decl);
        const auto d = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("d"), decl);
        const auto e = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("e"), decl);
        const auto h = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("h"), decl);
        const auto l = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("l"), decl);
        const auto af = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("af"), decl);
        const auto bc = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("bc"), decl);
        const auto de = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("de"), decl);
        const auto hl = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("hl"), decl);
        const auto sp = scope->createDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("sp"), decl);

        builtins.addRegisterDecomposition(bc, {c, b});
        builtins.addRegisterDecomposition(de, {e, d});
        builtins.addRegisterDecomposition(hl, {l, h});

        const auto patternA = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(a));
        const auto patternB = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(b));
        const auto patternC = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(c));
        const auto patternD = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(d));
        const auto patternE = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(e));
        const auto patternH = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(h));
        const auto patternL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(l));
        const auto patternAF = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(af));
        const auto patternBC = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(bc));
        const auto patternDE = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(de));
        const auto patternHL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(hl));
        const auto patternSP = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(sp));
        carry = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("carry"), decl);
        zero = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("zero"), decl);
        const auto patternZero = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(zero));
        const auto patternCarry = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(carry));
        const auto patternInterrupt = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("interrupt"), decl)));

        // Intrinsics.
        const auto push = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push"), decl);
        const auto pop = scope->createDefinition(nullptr, Definition::BuiltinLoadIntrinsic(u16Type), stringPool->intern("pop"), decl);
        const auto nop = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("nop"), decl);
        const auto halt = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("halt"), decl);
        const auto stop = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("stop"), decl);
        const auto decimal_adjust = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("decimal_adjust"), decl);
        const auto exchange_16_bit_registers = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("exchange_16_bit_registers"), decl);
        const auto swap_digits = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap_digits"), decl);
        const auto debug_break = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("debug_break"), decl);
        bit = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("bit"), decl);
        cmp = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("cmp"), decl);

        // Non-register operands.
        const auto patternFalse = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(false));
        const auto patternTrue = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(true));
        const auto patternAtLeast0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(0)));
        const auto patternAtLeast1 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(1)));
        const auto pattern0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0)));
        const auto pattern8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(8)));
        const auto pattern16 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(16)));
        const auto pattern24 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(24)));
        const auto pattern32 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(32)));
        const auto pattern40 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(40)));
        const auto pattern48 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(48)));
        const auto pattern56 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(56)));
        const auto patternIndirectBC = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternBC->clone(), 1));
        const auto patternIndirectDE = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternDE->clone(), 1));
        const auto patternIndirectHL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternHL->clone(), 1));
        const auto patternImmBitSubscript = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(7)));
        const auto patternImmU8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFF)));
        const auto patternImmU16 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFFFF)));
        const auto patternImmI8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(-0x80), Int128(0x7F)));
        const auto patternHighPage
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerRange(Int128(0xFF00), Int128(0xFFFF))))),
                1));
        const auto patternHighPageIndexedByC
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerRange(Int128(0xFF00))),
                patternC->clone(),
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
        const auto patternHLPostIncrementIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Unary(    
                    UnaryOperatorKind::PostIncrement,
                    patternHL->clone())),
                1));
        const auto patternHLPostDecrementIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Unary(    
                    UnaryOperatorKind::PostDecrement,
                    patternHL->clone())),
                1));

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

                const auto value = static_cast<std::uint16_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
                buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
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
        const auto encodingBitIndex = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size();
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                // n = $1th capture of $0
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                const auto n = static_cast<std::uint8_t>(captureLists[options.parameter[0]][options.parameter[1]]->variant.get<InstructionOperand::Integer>().value);
                buffer[buffer.size() - 1] |= static_cast<std::uint8_t>(n << 3);
                return true;
            });

        // Prefixes.
        const std::uint8_t prefixBit = 0xCB;

        // Register info.
        using RegisterInfo = std::tuple<const InstructionOperandPattern*, std::uint8_t>;
        const RegisterInfo generalRegisters[] {
            RegisterInfo {patternB, 0},
            RegisterInfo {patternC, 1},
            RegisterInfo {patternD, 2},
            RegisterInfo {patternE, 3},
            RegisterInfo {patternH, 4},
            RegisterInfo {patternL, 5},
            RegisterInfo {patternIndirectHL, 6},
            RegisterInfo {patternA, 7},
        };
        const RegisterInfo generalRegisterPairs[] {
            RegisterInfo {patternBC, 0},
            RegisterInfo {patternDE, 1},
            RegisterInfo {patternHL, 2},
            RegisterInfo {patternSP, 3},
        };

        // Instructions.
        // r = r2
        for (const auto& destRegister : generalRegisters) {
            for (const auto& sourceRegister : generalRegisters) {
                if (std::get<0>(destRegister) == std::get<0>(sourceRegister)) {
                    continue;
                }

                std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(0x40 | (std::get<1>(destRegister) << 3) | std::get<1>(sourceRegister))};
                const auto destRegisterOperand = std::get<0>(destRegister);
                const auto sourceRegisterOperand = std::get<0>(sourceRegister);

                builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {destRegisterOperand, sourceRegisterOperand}), encodingImplicit, InstructionOptions(opcode, {}, {}));                
            }
        }        
        // r = n
        for (const auto& destRegister : generalRegisters) {
            std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(0x6 | (std::get<1>(destRegister) << 3))};

            const auto destRegisterOperand = std::get<0>(destRegister);

            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {destRegisterOperand, patternImmU8}), encodingU8Operand, InstructionOptions(opcode, {1}, {}));
        }
        // a = 0 (a = a ^ a)
        // FIXME: xor clears the carry. sometimes, we don't want to clear the carry when loading 0 (eg. ld 0 followed by adc). need a way to disambiguate the instructions. for now disallow this optimization.
        //builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, pattern0}), encodingImplicit, InstructionOptions({0xAF}, {}, {}));
        // a = *(bc)
        // *(bc) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternIndirectBC}), encodingImplicit, InstructionOptions({0x0A}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternIndirectBC, patternA}), encodingImplicit, InstructionOptions({0x02}, {}, {}));
        // a = *(de)
        // *(de) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternIndirectDE}), encodingImplicit, InstructionOptions({0x1A}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternIndirectDE, patternA}), encodingImplicit, InstructionOptions({0x12}, {}, {}));
        // a = *(nn)
        // *(nn) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternAbsU8}), encodingU16Operand, InstructionOptions({0xFA}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternAbsU8, patternA}), encodingU16Operand, InstructionOptions({0xEA}, {0}, {}));
        // a = *(0xFF00 + n)
        // *(0xFF00 + n) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternHighPage}), encodingU8Operand, InstructionOptions({0xF0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternHighPage, patternA}), encodingU8Operand, InstructionOptions({0xE0}, {0}, {}));
        // a = *(0xFF00 + c)
        // *(0xFF00 + c) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternHighPageIndexedByC}), encodingImplicit, InstructionOptions({0xF2}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternHighPageIndexedByC, patternA}), encodingImplicit, InstructionOptions({0xE2}, {}, {}));
        // a = *(hl++)
        // *(hl++) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternHLPostIncrementIndirect}), encodingImplicit, InstructionOptions({0x2A}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternHLPostIncrementIndirect, patternA}), encodingImplicit, InstructionOptions({0x22}, {}, {}));
        // a = *(hl--)
        // *(hl--) = a
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternA, patternHLPostDecrementIndirect}), encodingImplicit, InstructionOptions({0x3A}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternHLPostDecrementIndirect, patternA}), encodingImplicit, InstructionOptions({0x32}, {}, {}));
        // rr = nn
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternBC, patternImmU16}), encodingU16Operand, InstructionOptions({0x01}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternDE, patternImmU16}), encodingU16Operand, InstructionOptions({0x11}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternHL, patternImmU16}), encodingU16Operand, InstructionOptions({0x21}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternSP, patternImmU16}), encodingU16Operand, InstructionOptions({0x31}, {1}, {}));
        // *(nn) = sp
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternAbsU16, patternSP}), encodingU16Operand, InstructionOptions({0x08}, {0}, {}));
        // sp = hl
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternSP, patternHL}), encodingImplicit, InstructionOptions({0xF9}, {}, {}));
        // push(rr)
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(push)), 0, {patternBC}), encodingImplicit, InstructionOptions({0xC5}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(push)), 0, {patternDE}), encodingImplicit, InstructionOptions({0xD5}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(push)), 0, {patternHL}), encodingImplicit, InstructionOptions({0xE5}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(push)), 0, {patternAF}), encodingImplicit, InstructionOptions({0xF5}, {}, {}));
        // rr = pop()
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::LoadIntrinsic(pop)), 0, {patternBC}), encodingImplicit, InstructionOptions({0xC1}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::LoadIntrinsic(pop)), 0, {patternDE}), encodingImplicit, InstructionOptions({0xD1}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::LoadIntrinsic(pop)), 0, {patternHL}), encodingImplicit, InstructionOptions({0xE1}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::LoadIntrinsic(pop)), 0, {patternAF}), encodingImplicit, InstructionOptions({0xF1}, {}, {}));
        // 8-bit arithmetic
        {
            using ArithmeticOperatorInfo = std::tuple<InstructionType, std::uint8_t>;
            const ArithmeticOperatorInfo arithmeticOperators[] {
                ArithmeticOperatorInfo {BinaryOperatorKind::Addition, 0x00},
                ArithmeticOperatorInfo {BinaryOperatorKind::AdditionWithCarry, 0x08},
                ArithmeticOperatorInfo {BinaryOperatorKind::Subtraction, 0x10},
                ArithmeticOperatorInfo {BinaryOperatorKind::SubtractionWithCarry, 0x18},
                ArithmeticOperatorInfo {BinaryOperatorKind::BitwiseAnd, 0x20},
                ArithmeticOperatorInfo {BinaryOperatorKind::BitwiseXor, 0x28},
                ArithmeticOperatorInfo {BinaryOperatorKind::BitwiseOr, 0x30},
                ArithmeticOperatorInfo {InstructionType::VoidIntrinsic(cmp), 0x38},
            };
            for (const auto& op : arithmeticOperators) {
                builtins.createInstruction(InstructionSignature(InstructionType(std::get<0>(op)), 0, {patternA, patternImmU8}), encodingU8Operand, InstructionOptions({static_cast<std::uint8_t>(0xC6 | std::get<1>(op))}, {1}, {}));

                for (const auto& sourceRegister : generalRegisters) {
                    std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(0x80 | std::get<1>(op) | std::get<1>(sourceRegister))};

                    const auto sourceRegisterOperand = std::get<0>(sourceRegister);

                    builtins.createInstruction(InstructionSignature(InstructionType(std::get<0>(op)), 0, {patternA, sourceRegisterOperand}), encodingImplicit, InstructionOptions(opcode, {}, {}));
                }
            }
        }
        // ++r
        // --r
        {
            using IncrementOperatorInfo = std::tuple<InstructionType, std::uint8_t>;
            const IncrementOperatorInfo incrementOperators[] {
                IncrementOperatorInfo {UnaryOperatorKind::PreIncrement, 0x04},
                IncrementOperatorInfo {UnaryOperatorKind::PreDecrement, 0x05},
            };
            for (const auto& op : incrementOperators) {
                for (const auto& sourceRegister : generalRegisters) {
                    std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(std::get<1>(op) | (std::get<1>(sourceRegister) << 3))};

                    const auto sourceRegisterOperand = std::get<0>(sourceRegister);

                    builtins.createInstruction(InstructionSignature(InstructionType(std::get<0>(op)), 0, {sourceRegisterOperand}), encodingImplicit, InstructionOptions(opcode, {}, {zero}));
                }
            }
        }
        // daa
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(decimal_adjust)), 0, {}), encodingImplicit, InstructionOptions({0x27}, {}, {}));
        // exx
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(exchange_16_bit_registers)), 0, {}), encodingImplicit, InstructionOptions({0xD9}, {}, {}));
        // a = ~a
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::BitwiseNegation), 0, {patternA}), encodingImplicit, InstructionOptions({0x2F}, {}, {}));
        // a = -a
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::SignedNegation), 0, {patternA}), encodingImplicit, InstructionOptions({0x2F, 0x3C}, {}, {}));
        // carry = false
        // carry = true
        // carry = !carry
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternCarry, patternFalse}), encodingImplicit, InstructionOptions({0x37, 0x3F}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternCarry, patternTrue}), encodingImplicit, InstructionOptions({0x37}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::LogicalNegation), 0, {patternCarry}), encodingImplicit, InstructionOptions({0x3F}, {}, {}));
        // nop
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(nop)), 0, {}), encodingImplicit, InstructionOptions({0x00}, {}, {}));
        // halt
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(halt)), 0, {}), encodingImplicit, InstructionOptions({0x76, 0x00}, {}, {}));
        // stop
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(stop)), 0, {}), encodingImplicit, InstructionOptions({0x10, 0x00}, {}, {}));
        // debug_break
        builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(debug_break)), 0, {}), encodingImplicit, InstructionOptions({0x40}, {}, {}));
        // interrupt = false
        // interrupt = true
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternInterrupt, patternFalse}), encodingImplicit, InstructionOptions({0xF3}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternInterrupt, patternTrue}), encodingImplicit, InstructionOptions({0xFB}, {}, {}));
        // hl += rr
        for (const auto& reg : generalRegisterPairs) {
            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Addition), 0, {patternHL, std::get<0>(reg)}), encodingImplicit, InstructionOptions({static_cast<std::uint8_t>(std::get<1>(reg) << 4 | 0x09)}, {}, {}));
        }
        // sp += dd
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Addition), 0, {patternSP, patternImmI8}), encodingI8Operand, InstructionOptions({0xE8}, {1}, {}));
        // hl = sp + dd
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Addition), 0, {patternHL, patternSP, patternImmI8}), encodingI8Operand, InstructionOptions({0xF8}, {2}, {}));
        // ++rr
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreIncrement), 0, {patternBC}), encodingImplicit, InstructionOptions({0x03}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreIncrement), 0, {patternDE}), encodingImplicit, InstructionOptions({0x13}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreIncrement), 0, {patternHL}), encodingImplicit, InstructionOptions({0x23}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreIncrement), 0, {patternSP}), encodingImplicit, InstructionOptions({0x33}, {}, {}));
        // --rr
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreDecrement), 0, {patternBC}), encodingImplicit, InstructionOptions({0x0B}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreDecrement), 0, {patternDE}), encodingImplicit, InstructionOptions({0x1B}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreDecrement), 0, {patternHL}), encodingImplicit, InstructionOptions({0x2B}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(UnaryOperatorKind::PreDecrement), 0, {patternSP}), encodingImplicit, InstructionOptions({0x3B}, {}, {}));
        // bitshifts
        {
            const std::pair<InstructionType, std::vector<std::uint8_t>> shiftOperators[] {
                {BinaryOperatorKind::LeftShift, {prefixBit, 0x20}}, // sla
                {BinaryOperatorKind::LogicalLeftShift, {prefixBit, 0x20}}, // sla
                {BinaryOperatorKind::RightShift, {prefixBit, 0x28}}, // sra
                {BinaryOperatorKind::LogicalRightShift, {prefixBit, 0x38}}, // srl
                {BinaryOperatorKind::LeftRotate, {prefixBit, 0x00}}, // rlc
                {BinaryOperatorKind::RightRotate, {prefixBit, 0x08}}, // rrc
                {BinaryOperatorKind::LeftRotateWithCarry, {prefixBit, 0x10}}, // rl
                {BinaryOperatorKind::RightRotateWithCarry, {prefixBit, 0x18}}, // rr
            };
            for (const auto& op : shiftOperators) {
                for (const auto& sourceRegister : generalRegisters) {
                    auto binKind = std::get<0>(op);
                    auto opcode = std::get<1>(op);
                    opcode[opcode.size() - 1] |= std::get<1>(sourceRegister);

                    const auto sourceRegisterOperand = std::get<0>(sourceRegister);

                    bool match = false;
                    if (const auto reg = sourceRegisterOperand->variant.tryGet<InstructionOperandPattern::Register>()) {
                        if (reg->definition == a && (binKind == BinaryOperatorKind::LeftShift || binKind == BinaryOperatorKind::LogicalLeftShift)) {
                            builtins.createInstruction(InstructionSignature(binKind, 0, {sourceRegisterOperand, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({0x87}, {1}, {}));
                            match = true;
                        }
                    }

                    if (!match) {
                        builtins.createInstruction(InstructionSignature(binKind, 0, {sourceRegisterOperand, patternImmU8}), encodingRepeatedImplicit, InstructionOptions(opcode, {1}, {}));
                    }
                }
            }
        }
        // hl <<= n
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::LeftShift), 0, {patternHL, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({0x29}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::LogicalLeftShift), 0, {patternHL, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({0x29}, {1}, {}));
        // bit(r $ n)
        // r $ n = true
        // r $ n = false
        // swap(r)
        for (const auto& reg : generalRegisters) {
            auto patternRegisterBit = builtins.createInstructionOperandPattern(InstructionOperandPattern::BitIndex(
                std::get<0>(reg)->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmBitSubscript->clone()))));

            builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(bit)), 0, {std::get<0>(reg), patternImmBitSubscript}), encodingBitIndex, InstructionOptions({prefixBit, static_cast<std::uint8_t>(0x40 | std::get<1>(reg))}, {1, 0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternRegisterBit, patternFalse}), encodingBitIndex, InstructionOptions({prefixBit, static_cast<std::uint8_t>(0x80 | std::get<1>(reg))}, {0, 0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternRegisterBit, patternTrue}), encodingBitIndex, InstructionOptions({prefixBit, static_cast<std::uint8_t>(0xC0 | std::get<1>(reg))}, {0, 0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType(InstructionType::VoidIntrinsic(swap_digits)), 0, {std::get<0>(reg)}), encodingImplicit, InstructionOptions({prefixBit, static_cast<std::uint8_t>(0x30 | std::get<1>(reg))}, {}, {}));
        }

        // jr abs
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16}), encodingPCRelativeI8Operand, InstructionOptions({0x18}, {1}, {}));
        // jr cond
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x20}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x28}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x30}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x38}, {1}, {}));
        // jp abs
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16}), encodingU16Operand, InstructionOptions({0xC3}, {1}, {}));
        // jp cond, abs
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternFalse}), encodingU16Operand, InstructionOptions({0xC2}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternTrue}), encodingU16Operand, InstructionOptions({0xCA}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternFalse}), encodingU16Operand, InstructionOptions({0xD2}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternTrue}), encodingU16Operand, InstructionOptions({0xDA}, {1}, {}));
        // jp hl
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternHL}), encodingImplicit, InstructionOptions({0xE9}, {}, {}));
        // call abs
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16}), encodingU16Operand, InstructionOptions({0xCD}, {1}, {}));
        // call cond, abs
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16, patternZero, patternFalse}), encodingU16Operand, InstructionOptions({0xC4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16, patternZero, patternTrue}), encodingU16Operand, InstructionOptions({0xCC}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16, patternCarry, patternFalse}), encodingU16Operand, InstructionOptions({0xD4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16, patternCarry, patternTrue}), encodingU16Operand, InstructionOptions({0xDC}, {1}, {}));
        // ret
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0xC9}, {1}, {}));
        // ret cond
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0, patternZero, patternFalse}), encodingImplicit, InstructionOptions({0xC0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0, patternZero, patternTrue}), encodingImplicit, InstructionOptions({0xC8}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0, patternCarry, patternFalse}), encodingImplicit, InstructionOptions({0xD0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0, patternCarry, patternTrue}), encodingImplicit, InstructionOptions({0xD8}, {1}, {}));
        // reti
        builtins.createInstruction(InstructionSignature(BranchKind::IrqReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0xD9}, {}, {}));
        // rst n
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern0}), encodingImplicit, InstructionOptions({0xC7}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern8}), encodingImplicit, InstructionOptions({0xCF}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern16}), encodingImplicit, InstructionOptions({0xD7}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern24}), encodingImplicit, InstructionOptions({0xDF}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern32}), encodingImplicit, InstructionOptions({0xE7}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern40}), encodingImplicit, InstructionOptions({0xEF}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern48}), encodingImplicit, InstructionOptions({0xF7}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, pattern56}), encodingImplicit, InstructionOptions({0xFF}, {0}, {}));
    }

    Definition* GameBoyPlatform::getPointerSizedType() const {
        return pointerSizedType;
    }

    Definition* GameBoyPlatform::getFarPointerSizedType() const {
        return farPointerSizedType;
    }

    std::unique_ptr<PlatformTestAndBranch> GameBoyPlatform::getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const {
        static_cast<void>(compiler);
        static_cast<void>(distanceHint);

        switch (op) {
            case BinaryOperatorKind::Equal:
            case BinaryOperatorKind::NotEqual: {
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    if (leftRegister->definition == a) {
                        // a == 0 -> { a |= a; } && zero
                        if (const auto rightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                            if (rightImmediate->value.isZero()) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    BinaryOperatorKind::BitwiseOr,
                                    std::vector<const Expression*> {left, left},
                                    std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                );
                            }
                        }

                        // a == right -> { cmp(a, right); } && zero
                        return std::make_unique<PlatformTestAndBranch>(
                            InstructionType::VoidIntrinsic(cmp),
                            std::vector<const Expression*> {left, right},
                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                        );
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::LessThan:
            case BinaryOperatorKind::GreaterThanOrEqual: {
                // a == right -> { cmp(a, right); } && carry
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isNegative()) {
                        if (const auto rightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                            if (rightImmediate->value.isZero()) {
                                // left < 0 -> { bit(left, 7); } && !zero
                                // left >= 0 -> { bit(left, 7); } && zero
                                std::vector<InstructionOperandRoot> operandRoots;
                                operandRoots.push_back(InstructionOperandRoot(left, compiler.createOperandFromExpression(left, true)));
                                operandRoots.push_back(InstructionOperandRoot(bitIndex7Expression.get(), compiler.createOperandFromExpression(bitIndex7Expression.get(), true)));

                                if (compiler.getBuiltins().selectInstruction(InstructionType::VoidIntrinsic(bit), 0, operandRoots)) {
                                    return std::make_unique<PlatformTestAndBranch>(
                                        InstructionType::VoidIntrinsic(bit),
                                        std::vector<const Expression*> {left, bitIndex7Expression.get()},
                                        std::vector<PlatformBranch> { PlatformBranch(zero, op != BinaryOperatorKind::LessThan, true) }
                                    );
                                }
                            }
                        }
                    } else {
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> { PlatformBranch(carry, op == BinaryOperatorKind::LessThan, true) }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::LessThanOrEqual: {
                // a == right -> { cmp(a, right); } && (zero || carry)
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isPositive()) {
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> {
                                        PlatformBranch(zero, true, true),
                                        PlatformBranch(carry, true, true)
                                    }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::GreaterThan: {
                // a == right -> { cmp(a, right); } && !zero && !carry
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isPositive()) {
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> {
                                        PlatformBranch(zero, true, false),
                                        PlatformBranch(carry, false, true)
                                    }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::BitIndexing: {
                // left $ right -> { bit(left, right); } && !zero
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    if (leftRegister->definition->variant.is<Definition::BuiltinRegister>()) {
                        if (const auto rightInteger = right->variant.tryGet<Expression::IntegerLiteral>()) {
                            const auto bitIndex = rightInteger->value;

                            if (Int128(0) <= bitIndex && bitIndex <= Int128(7)) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(bit),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> { PlatformBranch(zero, false, true) }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            default: return nullptr;
        }
    }

    Definition* GameBoyPlatform::getZeroFlag() const {
        return zero;
    }

    Int128 GameBoyPlatform::getPlaceholderValue() const {
        return Int128(UINT64_C(0xCCCCCCCCCCCCCCCC));
    }
}
