#include <string>
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <tuple>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/compiler.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/utility/report.h>
#include <wiz/platform/mos6502_platform.h>

namespace wiz {
    Mos6502Platform::Mos6502Platform(Revision revision)
    : revision(revision) {}

    Mos6502Platform::~Mos6502Platform() {}

    void Mos6502Platform::reserveDefinitions(Builtins& builtins) {
        // http://www.obelisk.me.uk/6502/index.html

        builtins.addDefineBoolean("__family_6502"_sv, true);
        switch (revision) {
            case Revision::Base6502:
                builtins.addDefineBoolean("__cpu_6502"_sv, true);
                break;
            case Revision::Base65C02:
                builtins.addDefineBoolean("__family_6C502"_sv, true);
                builtins.addDefineBoolean("__cpu_65c02"_sv, true);
                break;
            case Revision::Rockwell65C02:
                builtins.addDefineBoolean("__family_65C02"_sv, true);
                builtins.addDefineBoolean("__cpu_rockwell65c02"_sv, true);
                break;
            case Revision::Wdc65C02:
                builtins.addDefineBoolean("__family_65C02"_sv, true);
                builtins.addDefineBoolean("__cpu_wdc65c02"_sv, true);
                break;
            case Revision::Huc6280:
                builtins.addDefineBoolean("__family_65C02"_sv, true);
                builtins.addDefineBoolean("__cpu_huc6280"_sv, true);
                break;
        }

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
        x = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("x"), decl);
        y = scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("y"), decl);
        const auto patternA = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(a));
        const auto patternX = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(x));
        const auto patternY = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(y));
        const auto patternS = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("s"), decl)));
        const auto patternP = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("p"), decl)));
        carry = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("carry"), decl);
        zero = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("zero"), decl);
        nointerrupt = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("nointerrupt"), decl);
        decimal = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("decimal"), decl);
        overflow = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("overflow"), decl);
        negative = scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("negative"), decl);
        const auto patternCarry = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(carry));
        const auto patternZero = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(zero));
        const auto patternNoInterrupt = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(nointerrupt));
        const auto patternDecimal = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(decimal));
        const auto patternOverflow = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(overflow));
        const auto patternNegative = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(negative));

        // Intrinsics.
        cmp = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("cmp"), decl);
        bit = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("bit"), decl);
        const auto push = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push"), decl);
        const auto pop = scope->createDefinition(nullptr, Definition::BuiltinLoadIntrinsic(u8Type), stringPool->intern("pop"), decl);
        const auto irqcall = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("irqcall"), decl);
        const auto nop = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("nop"), decl);

        // Non-register operands.
        const auto patternFalse = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(false));
        const auto patternTrue = builtins.createInstructionOperandPattern(InstructionOperandPattern::Boolean(true));
        const auto patternAtLeast0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(0)));
        const auto patternAtLeast1 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(1)));
        const auto pattern0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0)));
        const auto patternImmU8 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFF)));
        const auto patternImmZPU8 = revision == Revision::Huc6280 ? builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0x2000), Int128(0x20FF))) : patternImmU8;
        const auto patternImmU16 = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFFFF)));
        const auto patternZeroPage
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmZPU8->clone())),
                1));
        const auto patternZeroPageIndexedByX
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmZPU8->clone())),
                patternX->clone(),
                1, 1));
        const auto patternZeroPageIndexedByY
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmZPU8->clone())),
                patternY->clone(),
                1, 1));
        const auto patternZeroPageIndexedByXIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmZPU8->clone())),
                    patternX->clone(),
                    1, 2)),
                1));
        const auto patternZeroPageIndirectIndexedByY
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmZPU8->clone())),
                    2)),
                patternY->clone(),
                1, 1));
        const auto patternZeroPageIndirect
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmZPU8->clone())),
                    2)),
                1));
        const auto patternAbsolute
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                1));
        const auto patternAbsoluteIndexedByX
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternX->clone(),
                1, 1));
        const auto patternAbsoluteIndexedByY
            = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternY->clone(),
                1, 1));
        const auto patternIndirectJump
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
                    return true;
                } else {
                    buffer.push_back(0);
                    report->error("pc-relative offset is outside of representable signed 8-bit range -128..127", location);
                    return false;
                }
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
        const auto encodingRepeatedU16Operand = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                return static_cast<std::size_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value) * (options.opcode.size() + 2);
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                const auto value = static_cast<std::uint16_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                const auto count = static_cast<std::uint8_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value);
                for (std::size_t i = 0; i != count; ++i) {
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
                }
                return true;
            });
        const auto encodingU8OperandBitIndex = builtins.createInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 1;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                // *(zp) $ n bit-wise access.
                // zp = 0th capture of $0
                // n = $2th capture of $1
                const auto zp = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                const auto n = static_cast<std::uint8_t>(captureLists[options.parameter[1]][options.parameter[2]]->variant.get<InstructionOperand::Integer>().value);
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                buffer[buffer.size() - 1] |= static_cast<std::uint8_t>(n << 4);
                buffer.push_back(zp);
                return true;
            });

        // Instructions.
        const std::pair<InstructionType, std::vector<std::uint8_t>> arithmeticOperators[] {
            {BinaryOperatorKind::BitwiseOr, {0x00}},
            {BinaryOperatorKind::BitwiseAnd, {0x20}},
            {BinaryOperatorKind::BitwiseXor, {0x40}},
            {BinaryOperatorKind::AdditionWithCarry, {0x60}},
            {BinaryOperatorKind::Addition, {0x18, 0x60}},
            {BinaryOperatorKind::Assignment, {0xA0}},
            {InstructionType::VoidIntrinsic(cmp), {0xC0}},
            {BinaryOperatorKind::SubtractionWithCarry, {0xE0}},
            {BinaryOperatorKind::Subtraction, {0x38, 0xE0}},
        };

        using ArithmeticOperandSignature = std::tuple<const InstructionOperandPattern*, const InstructionEncoding*, std::uint8_t>;
        const ArithmeticOperandSignature arithmeticOperandSignatures[] {
            ArithmeticOperandSignature {patternImmU8, encodingU8Operand, 0x09},
            ArithmeticOperandSignature {patternZeroPage, encodingU8Operand, 0x05},
            ArithmeticOperandSignature {patternZeroPageIndexedByX, encodingU8Operand, 0x15},
            ArithmeticOperandSignature {patternZeroPageIndexedByXIndirect, encodingU8Operand, 0x01},
            ArithmeticOperandSignature {patternZeroPageIndirectIndexedByY, encodingU8Operand, 0x11},
            ArithmeticOperandSignature {patternAbsolute, encodingU16Operand, 0x0D},
            ArithmeticOperandSignature {patternAbsoluteIndexedByX, encodingU16Operand, 0x1D},
            ArithmeticOperandSignature {patternAbsoluteIndexedByY, encodingU16Operand, 0x19},
        };
        //  lda, accumulator arithmetic
        for (const auto& op : arithmeticOperators) {
            for (const auto& sig : arithmeticOperandSignatures) {
                std::vector<std::uint8_t> opcode = op.second;
                opcode[opcode.size() - 1] |= std::get<2>(sig);
                builtins.createInstruction(InstructionSignature(op.first, 0, {patternA, std::get<0>(sig)}), std::get<1>(sig), InstructionOptions(std::move(opcode), {1}, {}));
            }
        }
        // sta
        for (const auto& sig : arithmeticOperandSignatures) {
            if (std::get<0>(sig) == patternImmU8) {
                continue;
            }

            std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(0x80 | std::get<2>(sig))};
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {std::get<0>(sig), patternA}), std::get<1>(sig), InstructionOptions(std::move(opcode), {0}, {}));
        }
        // bit - overflow = mem $ 6, negative = mem $ 7, zero = a & mem
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), 0, {patternZeroPage}), encodingU8Operand, InstructionOptions({0x24}, {0}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), 0, {patternAbsolute}), encodingU16Operand, InstructionOptions({0x2C}, {0}, {}));
        // ldx
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternImmU8}), encodingU8Operand, InstructionOptions({0xA2}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternZeroPage}), encodingU8Operand, InstructionOptions({0xA6}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternZeroPageIndexedByY}), encodingU8Operand, InstructionOptions({0xB6}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternAbsolute}), encodingU16Operand, InstructionOptions({0xAE}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternAbsoluteIndexedByY}), encodingU16Operand, InstructionOptions({0xBE}, {1}, {}));
        // ldy
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternImmU8}), encodingU8Operand, InstructionOptions({0xA0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternZeroPage}), encodingU8Operand, InstructionOptions({0xA4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternZeroPageIndexedByX}), encodingU8Operand, InstructionOptions({0xB4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternAbsolute}), encodingU16Operand, InstructionOptions({0xAC}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternAbsoluteIndexedByX}), encodingU16Operand, InstructionOptions({0xBC}, {1}, {}));
        // stx
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPage, patternX}), encodingU8Operand, InstructionOptions({0x86}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPageIndexedByY, patternX}), encodingU8Operand, InstructionOptions({0x96}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternAbsolute, patternX}), encodingU16Operand, InstructionOptions({0x8E}, {0}, {}));
        // sty
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPage, patternY}), encodingU8Operand, InstructionOptions({0x84}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPageIndexedByX, patternY}), encodingU8Operand, InstructionOptions({0x94}, {0}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternAbsolute, patternY}), encodingU16Operand, InstructionOptions({0x8C}, {0}, {}));
        // cpx
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternX, patternImmU8}), encodingU8Operand, InstructionOptions({0xE0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternX, patternZeroPage}), encodingU8Operand, InstructionOptions({0xE4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternX, patternAbsolute}), encodingU16Operand, InstructionOptions({0xEC}, {1}, {}));
        // cpy
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternY, patternImmU8}), encodingU8Operand, InstructionOptions({0xC0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternY, patternZeroPage}), encodingU8Operand, InstructionOptions({0xC4}, {1}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), 0, {patternY, patternAbsolute}), encodingU16Operand, InstructionOptions({0xCC}, {1}, {}));
        // transfer instructions
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternX}), encodingImplicit, InstructionOptions({0x8A}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternY}), encodingImplicit, InstructionOptions({0x98}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternA}), encodingImplicit, InstructionOptions({0xAA}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, patternA}), encodingImplicit, InstructionOptions({0xA8}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, patternS}), encodingImplicit, InstructionOptions({0xBA}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternS, patternX}), encodingImplicit, InstructionOptions({0x9A}, {}, {}));
        // push
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternA}), encodingImplicit, InstructionOptions({0x48}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternP}), encodingImplicit, InstructionOptions({0x08}, {}, {}));
        // pop
        builtins.createInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternA}), encodingImplicit, InstructionOptions({0x68}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternP}), encodingImplicit, InstructionOptions({0x28}, {}, {}));
        // increment
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternZeroPage}), encodingU8Operand, InstructionOptions({0xE6}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternZeroPageIndexedByX}), encodingU8Operand, InstructionOptions({0xF6}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternAbsolute}), encodingU16Operand, InstructionOptions({0xEE}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternAbsoluteIndexedByX}), encodingU16Operand, InstructionOptions({0xFE}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternX}), encodingImplicit, InstructionOptions({0xE8}, {}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternY}), encodingImplicit, InstructionOptions({0xC8}, {}, {zero}));
        // decrement
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternZeroPage}), encodingU8Operand, InstructionOptions({0xC6}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternZeroPageIndexedByX}), encodingU8Operand, InstructionOptions({0xD6}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternAbsolute}), encodingU16Operand, InstructionOptions({0xCE}, {0}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternAbsoluteIndexedByX}), encodingU16Operand, InstructionOptions({0xDE}, {0}, {zero})) ;
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternX}), encodingImplicit, InstructionOptions({0xCA}, {}, {zero}));
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternY}), encodingImplicit, InstructionOptions({0x88}, {}, {zero}));
        // bitwise negation
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::BitwiseNegation, 0, {patternA}), encodingImplicit, InstructionOptions({0x49, 0xFF}, {}, {}));
        // signed negation
        builtins.createInstruction(InstructionSignature(UnaryOperatorKind::SignedNegation, 0, {patternA}), encodingImplicit, InstructionOptions({0x49, 0xFF, 0x18, 0x69, 0x01}, {}, {}));
        // bitshifts
        const std::pair<InstructionType, std::uint8_t> shiftOperators[] {
            {BinaryOperatorKind::LeftShift, 0x00},
            {BinaryOperatorKind::LogicalLeftShift, 0x00},
            {BinaryOperatorKind::LeftRotateWithCarry, 0x20},
            {BinaryOperatorKind::LogicalRightShift, 0x40},
            {BinaryOperatorKind::RightRotateWithCarry, 0x60},
        };
        for (const auto& op : shiftOperators) {
            builtins.createInstruction(InstructionSignature(op.first, 0, {patternA, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0A)}, {1}, {}));
            builtins.createInstruction(InstructionSignature(op.first, 0, {patternZeroPage, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x06)}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(op.first, 0, {patternZeroPageIndexedByX, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x16)}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(op.first, 0, {patternAbsolute, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0E)}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(op.first, 0, {patternAbsoluteIndexedByX, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x1E)}, {0, 1}, {}));
        }
        // jump / branch instructions
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {revision == Revision::Base6502 ? patternAtLeast0 : patternAtLeast1, patternImmU16}), encodingU16Operand, InstructionOptions({0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectJump}), encodingU16Operand, InstructionOptions({0x6C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x90}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xB0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0xD0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xF0}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternNegative, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x10}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternNegative, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x30}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternOverflow, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x50}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternOverflow, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x70}, {1}, {}));        
        // long branch instructions
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternFalse}), encodingU16Operand, InstructionOptions({0xB0, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternTrue}), encodingU16Operand, InstructionOptions({0x90, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternFalse}), encodingU16Operand, InstructionOptions({0xF0, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternTrue}), encodingU16Operand, InstructionOptions({0xD0, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternNegative, patternFalse}), encodingU16Operand, InstructionOptions({0x30, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternNegative, patternTrue}), encodingU16Operand, InstructionOptions({0x10, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternOverflow, patternFalse}), encodingU16Operand, InstructionOptions({0x70, 3, 0x4C}, {1}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternOverflow, patternTrue}), encodingU16Operand, InstructionOptions({0x50, 3, 0x4C}, {1}, {}));
        // call instructions
        builtins.createInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16}), encodingU16Operand, InstructionOptions({0x20}, {1}, {}));
        // return instructions
        builtins.createInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x60}, {}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::IrqReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x40}, {}, {}));
        builtins.createInstruction(InstructionSignature(BranchKind::NmiReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x40}, {}, {}));
        // brk
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(irqcall), 0, {}), encodingImplicit, InstructionOptions({0x00}, {}, {}));
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(irqcall), 0, {patternImmU8}), encodingU8Operand, InstructionOptions({0x00}, {0}, {}));
        // nop
        builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(nop), 0, {}), encodingImplicit, InstructionOptions({0xEA}, {}, {}));
        // carry - clc / sec
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternCarry, patternFalse}), encodingImplicit, InstructionOptions({0x18}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternCarry, patternTrue}), encodingImplicit, InstructionOptions({0x38}, {}, {}));
        // decimal - cld / sed
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternDecimal, patternFalse}), encodingImplicit, InstructionOptions({0xD8}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternDecimal, patternTrue}), encodingImplicit, InstructionOptions({0xF8}, {}, {}));
        // interrupt - cli/sei
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternNoInterrupt, patternFalse}), encodingImplicit, InstructionOptions({0x58}, {}, {}));
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternNoInterrupt, patternTrue}), encodingImplicit, InstructionOptions({0x78}, {}, {}));
        // overflow - clv
        builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternOverflow, patternFalse}), encodingImplicit, InstructionOptions({0xB8}, {}, {}));

        // Extra 65c02 instructions
        if (revision == Revision::Base65C02
        || revision == Revision::Rockwell65C02
        || revision == Revision::Wdc65C02
        || revision == Revision::Huc6280) {
            // http://6502.org/tutorials/65c02opcodes.html#2
            // http://6502.org/tutorials/65c02opcodes.html#3
            const auto test_and_reset = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("test_and_reset"), decl);
            const auto test_and_set = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("test_and_set"), decl);

            const auto patternIndirectJumpIndexedByX
                = builtins.createInstructionOperandPattern(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU16->clone())),
                    patternX->clone(),
                    1, 2));

            // arithmetic operators can use indrected zero page variable without indexing it by x or y.
            for (const auto& op : arithmeticOperators) {
                std::vector<std::uint8_t> opcode = op.second;
                opcode[opcode.size() - 1] |= 0x12;
                builtins.createInstruction(InstructionSignature(op.first, 0, {patternA, patternZeroPageIndirect}), encodingU8Operand, InstructionOptions(std::move(opcode), {1}, {}));
            }
            // sta
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPageIndirect, patternA}), encodingU8Operand, InstructionOptions({0x92}, {0}, {}));
            // bit
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), 0, {patternImmU8}), encodingU8Operand, InstructionOptions({0x89}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), 0, {patternZeroPageIndexedByX}), encodingU8Operand, InstructionOptions({0x34}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), 0, {patternAbsoluteIndexedByX}), encodingU16Operand, InstructionOptions({0x3C}, {0}, {}));
            // ++a
            // --a
            builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, 0, {patternA}), encodingImplicit, InstructionOptions({0x1A}, {}, {zero}));
            builtins.createInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, 0, {patternA}), encodingImplicit, InstructionOptions({0x3A}, {}, {zero}));
            // branch always
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16}), encodingPCRelativeI8Operand, InstructionOptions({0x80}, {1}, {}));
            // indirect jump indexed by x
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectJumpIndexedByX}), encodingU16Operand, InstructionOptions({0x6C}, {1}, {}));
            // push
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternX}), encodingImplicit, InstructionOptions({0xDA}, {}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternY}), encodingImplicit, InstructionOptions({0x5A}, {}, {}));
            // pop
            builtins.createInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternX}), encodingImplicit, InstructionOptions({0xFA}, {}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternY}), encodingImplicit, InstructionOptions({0x7A}, {}, {}));
            // stz
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPage, pattern0}), encodingU8Operand, InstructionOptions({0x64}, {0}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternZeroPageIndexedByX, pattern0}), encodingU8Operand, InstructionOptions({0x74}, {0}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternAbsolute, pattern0}), encodingU16Operand, InstructionOptions({0x9C}, {0}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternAbsoluteIndexedByX, pattern0}), encodingU16Operand, InstructionOptions({0x9E}, {0}, {}));
            // trb
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), 0, {patternZeroPage, patternA}), encodingU8Operand, InstructionOptions({0x14}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), 0, {patternAbsolute, patternA}), encodingU16Operand, InstructionOptions({0x1C}, {0}, {}));
            // tsb
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), 0, {patternZeroPage, patternA}), encodingU8Operand, InstructionOptions({0x04}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), 0, {patternAbsolute, patternA}), encodingU16Operand, InstructionOptions({0x0C}, {0}, {}));
        }

        // Extra bit-related instructions (WDC, Rockwell, HuC)
        if (revision == Revision::Wdc65C02
        || revision == Revision::Rockwell65C02
        || revision == Revision::Huc6280) {
            // http://6502.org/tutorials/65c02opcodes.html#4
            const auto patternImmBitSubscript = builtins.createInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(7)));
            const auto patternZeroPageBitIndex
                = builtins.createInstructionOperandPattern(InstructionOperandPattern::BitIndex(
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                        false,
                        makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                            patternImmZPU8->clone())),
                        1)),
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmBitSubscript->clone()))));

            const auto encodingU8OperandBitIndexBranch = builtins.createInstructionEncoding(
                [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                    static_cast<void>(captureLists);
                    return options.opcode.size() + 2;
                },
                [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                    // goto dest if *(zp) $ n
                    // zp = 0th capture of $0
                    // n = $2th capture of $1
                    // dest = 0th capture of $3
                    const auto zp = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto n = static_cast<std::uint8_t>(captureLists[options.parameter[1]][options.parameter[2]]->variant.get<InstructionOperand::Integer>().value);
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer[buffer.size() - 1] |= static_cast<std::uint8_t>(n << 4);
                    buffer.push_back(zp);

                    const auto base = static_cast<int>(bank->getAddress().absolutePosition.get());
                    const auto dest  = static_cast<int>(captureLists[options.parameter[3]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto offset = dest - base - 3;
                    if (offset >= -128 && offset <= 127) {
                        buffer.push_back(offset < 0
                            ? (static_cast<std::uint8_t>(-offset) ^ 0xFF) + 1
                            : static_cast<std::uint8_t>(offset));
                        return true;
                    } else {
                        buffer.push_back(0);
                        report->error("pc-relative offset is outside of representable signed 8-bit range -128..127", location);
                        return false;
                    }
                });
            const auto encodingU8OperandBitIndexLongBranch = builtins.createInstructionEncoding(
                [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                    static_cast<void>(captureLists);
                    return options.opcode.size() + 3;
                },
                [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                    static_cast<void>(report);
                    static_cast<void>(bank);
                    static_cast<void>(location);

                    // ^goto dest if *(zp) $ n
                    // zp = 0th capture of $0
                    // n = $2th capture of $1
                    // dest = 0th capture of $3
                    const auto zp = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto n = static_cast<std::uint8_t>(captureLists[options.parameter[1]][options.parameter[2]]->variant.get<InstructionOperand::Integer>().value);
                    const auto dest = static_cast<std::uint16_t>(captureLists[options.parameter[3]][0]->variant.get<InstructionOperand::Integer>().value);
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer[0] |= static_cast<std::uint8_t>(n << 4);
                    buffer.push_back(zp);
                    buffer.push_back(static_cast<std::uint8_t>(dest & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((dest >> 8) & 0xFF));
                    return true;
                });

            // branch on bit - goto label if zp $ n;
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZeroPageBitIndex, patternFalse}), encodingU8OperandBitIndexBranch, InstructionOptions({0x0F}, {2, 2, 1, 1}, {}));
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZeroPageBitIndex, patternTrue}), encodingU8OperandBitIndexBranch, InstructionOptions({0x8F}, {2, 2, 1, 1}, {}));
            // long branch on bit - goto label if zp $ n;
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZeroPageBitIndex, patternFalse}), encodingU8OperandBitIndexLongBranch, InstructionOptions({0x8F, 3, 0x4C}, {2, 2, 1, 1}, {}));
            builtins.createInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZeroPageBitIndex, patternTrue}), encodingU8OperandBitIndexLongBranch, InstructionOptions({0x0F, 3, 0x4C}, {2, 2, 1, 1}, {}));
            // zp $ n = false
            // zp $ n = true
            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternZeroPageBitIndex, patternFalse}), encodingU8OperandBitIndex, InstructionOptions({0x07}, {0, 0, 1}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType(BinaryOperatorKind::Assignment), 0, {patternZeroPageBitIndex, patternTrue}), encodingU8OperandBitIndex, InstructionOptions({0x87}, {0, 0, 1}, {}));
        }

        // Extra power-saving instructions (WDC)
        if (revision == Revision::Wdc65C02) {
            // http://6502.org/tutorials/65c02opcodes.html#5
            const auto stop_until_reset = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("stop_until_reset"), decl);
            const auto wait_until_interrupt = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("wait_until_interrupt"), decl);

            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(stop_until_reset), 0, {}), encodingImplicit, InstructionOptions({0xDB}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(wait_until_interrupt), 0, {}), encodingImplicit, InstructionOptions({0xCB}, {0}, {}));
        }

        // Extra Huc6280 instructions
        if (revision == Revision::Huc6280) {
            // See: http://archaicpixels.com/HuC6280_Instruction_Set
            const auto patternIndirectX = builtins.createInstructionOperandPattern(InstructionOperandPattern::Dereference(false, patternX->clone(), 1));
            const auto patternTurboSpeed = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("turbo_speed"), decl)));
            const auto patternVdcSelect = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("vdc_select"), decl)));
            const auto patternVdcDataL = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("vdc_data_l"), decl)));
            const auto patternVdcDataH = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("vdc_data_h"), decl)));
            const auto patternMPR0 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr0"), decl)));
            const auto patternMPR1 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr1"), decl)));
            const auto patternMPR2 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr2"), decl)));
            const auto patternMPR3 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr3"), decl)));
            const auto patternMPR4 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr4"), decl)));
            const auto patternMPR5 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr5"), decl)));
            const auto patternMPR6 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr6"), decl)));
            const auto patternMPR7 = builtins.createInstructionOperandPattern(InstructionOperandPattern::Register(scope->createDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("mpr7"), decl)));

            const auto swap = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap"), decl);
            const auto transfer_alternate_to_increment = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_alternate_to_increment"), decl);
            const auto transfer_increment_to_alternate = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_increment_to_alternate"), decl);
            const auto transfer_decrement_to_decrement = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_decrement_to_decrement"), decl);
            const auto transfer_increment_to_increment = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_increment_to_increment"), decl);
            const auto transfer_increment_to_fixed = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_increment_to_fixed"), decl);
            const auto mpr_set = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mpr_set"), decl);

            tst = scope->createDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("tst"), decl);

            const auto encodingZeroPageBitwiseTest = builtins.createInstructionEncoding(
                [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                    static_cast<void>(captureLists);
                    return options.opcode.size() + 3;
                },
                [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                    static_cast<void>(report);
                    static_cast<void>(bank);
                    static_cast<void>(location);

                    const auto mask = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto source = static_cast<std::uint8_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value);
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer.push_back(mask);
                    buffer.push_back(source);
                    return true;
                });
            const auto encodingAbsoluteBitwiseTest = builtins.createInstructionEncoding(
                [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                    static_cast<void>(captureLists);
                    return options.opcode.size() + 3;
                },
                [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                    static_cast<void>(report);
                    static_cast<void>(bank);
                    static_cast<void>(location);

                    const auto mask = static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto source = static_cast<std::uint16_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value);
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer.push_back(mask);
                    buffer.push_back(static_cast<std::uint8_t>(source & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((source >> 8) & 0xFF));
                    return true;
                });
            const auto encodingBlockTransfer = builtins.createInstructionEncoding(
                [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                    static_cast<void>(captureLists);
                    return options.opcode.size() + 6;
                },
                [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                    static_cast<void>(report);
                    static_cast<void>(bank);
                    static_cast<void>(location);

                    const auto src = static_cast<std::uint16_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto dest = static_cast<std::uint16_t>(captureLists[options.parameter[1]][0]->variant.get<InstructionOperand::Integer>().value);
                    const auto length = static_cast<std::uint16_t>(captureLists[options.parameter[2]][0]->variant.get<InstructionOperand::Integer>().value);
                    buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                    buffer.push_back(static_cast<std::uint8_t>(src & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((src >> 8) & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>(dest & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((dest >> 8) & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>(length & 0xFF));
                    buffer.push_back(static_cast<std::uint8_t>((length>> 8) & 0xFF));
                    return true;
                });

            // cla / clx / cly
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, pattern0}), encodingImplicit, InstructionOptions({0x62}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternX, pattern0}), encodingImplicit, InstructionOptions({0x82}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternY, pattern0}), encodingImplicit, InstructionOptions({0xC2}, {}, {}));
            // csl / csh
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternTurboSpeed, patternFalse}), encodingImplicit, InstructionOptions({0xD4}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternTurboSpeed, patternTrue}), encodingImplicit, InstructionOptions({0x54}, {}, {}));
            // sax / say / sxy
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(swap), 0, {patternA, patternX}), encodingImplicit, InstructionOptions({0x22}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(swap), 0, {patternA, patternY}), encodingImplicit, InstructionOptions({0x42}, {0}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(swap), 0, {patternX, patternY}), encodingImplicit, InstructionOptions({0x02}, {0}, {}));
            // `set`-prefixed arithmetic instruction: Sets the T flag before doing arithmetic, which in turn replaces a with *(x) in the immediate next arithmetic instruction.
            for (const auto& arithmeticOperator : arithmeticOperators) {
                bool canUseTFlag = false;
                if (const auto op = std::get<0>(arithmeticOperator).variant.tryGet<BinaryOperatorKind>()) {
                    switch (*op) {
                        case BinaryOperatorKind::Addition:
                        case BinaryOperatorKind::AdditionWithCarry:
                        case BinaryOperatorKind::BitwiseXor:
                        case BinaryOperatorKind::BitwiseOr:
                        case BinaryOperatorKind::BitwiseAnd:
                            canUseTFlag = true;
                        default: break;
                    }
                }
                if (canUseTFlag) {
                    for (const auto& sig : arithmeticOperandSignatures) {
                        std::vector<std::uint8_t> opcode = arithmeticOperator.second;

                        // Need to be directly before the arithmetic instruction (eg. when modifying carry), or else T flag gets cleared. http://forums.magicengine.com/en/viewtopic.php?p=10344#10344
                        opcode.insert(opcode.end() - 1, 0xF4);
                        opcode[opcode.size() - 1] |= std::get<2>(sig);
                        builtins.createInstruction(InstructionSignature(arithmeticOperator.first, 0, {patternIndirectX, std::get<0>(sig)}), std::get<1>(sig), InstructionOptions(std::move(opcode), {1}, {}));
                    }
                }
            }
            // st0 / st1 / st2 - set vdc register n to immediate value.
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternVdcSelect, patternImmU8}), encodingU8Operand, InstructionOptions({0x03}, {1}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternVdcDataL, patternImmU8}), encodingU8Operand, InstructionOptions({0x13}, {1}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternVdcDataH, patternImmU8}), encodingU8Operand, InstructionOptions({0x23}, {1}, {}));
            // block transfers: tai/tia/tdd/tii/tin
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_alternate_to_increment), 0, {patternImmU16, patternImmU16, patternImmU16}), encodingBlockTransfer, InstructionOptions({0xF3}, {0, 1, 2}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_increment_to_alternate), 0, {patternImmU16, patternImmU16, patternImmU16}), encodingBlockTransfer, InstructionOptions({0xE3}, {0, 1, 2}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_decrement_to_decrement), 0, {patternImmU16, patternImmU16, patternImmU16}), encodingBlockTransfer, InstructionOptions({0xC3}, {0, 1, 2}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_increment_to_increment), 0, {patternImmU16, patternImmU16, patternImmU16}), encodingBlockTransfer, InstructionOptions({0x73}, {0, 1, 2}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_increment_to_fixed), 0, {patternImmU16, patternImmU16, patternImmU16}), encodingBlockTransfer, InstructionOptions({0xD3}, {0, 1, 2}, {}));
            // tam - modify MPR0-7 based on accumulator
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mpr_set), 0, {patternImmU8, patternA}), encodingU8Operand, InstructionOptions({0x53}, {0}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR0, patternA}), encodingImplicit, InstructionOptions({0x53, 0x01}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR1, patternA}), encodingImplicit, InstructionOptions({0x53, 0x02}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR2, patternA}), encodingImplicit, InstructionOptions({0x53, 0x04}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR3, patternA}), encodingImplicit, InstructionOptions({0x53, 0x08}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR4, patternA}), encodingImplicit, InstructionOptions({0x53, 0x10}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR5, patternA}), encodingImplicit, InstructionOptions({0x53, 0x20}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR6, patternA}), encodingImplicit, InstructionOptions({0x53, 0x40}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternMPR7, patternA}), encodingImplicit, InstructionOptions({0x53, 0x80}, {}, {}));
            // tma - read MPRn into accumulator
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR0}), encodingImplicit, InstructionOptions({0x43, 0x01}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR1}), encodingImplicit, InstructionOptions({0x43, 0x02}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR2}), encodingImplicit, InstructionOptions({0x43, 0x04}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR3}), encodingImplicit, InstructionOptions({0x43, 0x08}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR4}), encodingImplicit, InstructionOptions({0x43, 0x10}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR5}), encodingImplicit, InstructionOptions({0x43, 0x20}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR6}), encodingImplicit, InstructionOptions({0x43, 0x40}, {}, {}));
            builtins.createInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternA, patternMPR7}), encodingImplicit, InstructionOptions({0x43, 0x80}, {}, {}));
            // tst #imm, mem - overflow = mem $ 6, negative = mem $ 7, zero = imm & mem
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(tst), 0, {patternImmU8, patternZeroPage}), encodingZeroPageBitwiseTest, InstructionOptions({0x83}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(tst), 0, {patternImmU8, patternZeroPageIndexedByX}), encodingZeroPageBitwiseTest, InstructionOptions({0xA3}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(tst), 0, {patternImmU8, patternAbsolute}), encodingAbsoluteBitwiseTest, InstructionOptions({0x93}, {0, 1}, {}));
            builtins.createInstruction(InstructionSignature(InstructionType::VoidIntrinsic(tst), 0, {patternImmU8, patternAbsoluteIndexedByX}), encodingAbsoluteBitwiseTest, InstructionOptions({0xB3}, {0, 1}, {}));
        }
    }

    Definition* Mos6502Platform::getPointerSizedType() const {
        return pointerSizedType;
    }

    Definition* Mos6502Platform::getFarPointerSizedType() const {
        return farPointerSizedType;
    }

    std::unique_ptr<PlatformTestAndBranch> Mos6502Platform::getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const {
        static_cast<void>(compiler);
        static_cast<void>(distanceHint);
        
        switch (op) {
            case BinaryOperatorKind::Equal:
            case BinaryOperatorKind::NotEqual: {
                if (const auto& leftBinaryOperator = left->variant.tryGet<Expression::BinaryOperator>()) {                        
                    const auto innerOp = leftBinaryOperator->op;
                    const auto innerLeft = leftBinaryOperator->left.get();
                    const auto innerRight = leftBinaryOperator->right.get();

                    if (innerOp == BinaryOperatorKind::BitwiseAnd) {
                        if (const auto outerRightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                            if (outerRightImmediate->value.isZero()) {
                                // a & mem == 0 -> { bit(mem); } && zero
                                // mem & a == 0 -> { bit(mem); } && zero
                                if (const auto& leftRegister = innerLeft->variant.tryGet<Expression::ResolvedIdentifier>()) {
                                    if (leftRegister->definition == a) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(bit),
                                            std::vector<const Expression*> {innerRight},
                                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                        );
                                    }
                                }
                                if (const auto& rightRegister = innerRight->variant.tryGet<Expression::ResolvedIdentifier>()) {
                                    if (rightRegister->definition == a) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(bit),
                                            std::vector<const Expression*> {innerLeft},
                                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                        );
                                    }
                                }
                                if (revision == Revision::Huc6280) {
                                    // imm & mem == 0 -> { tst(imm, mem); } && zero
                                    // mem & imm == 0 -> { tst(mem, imm); } && zero
                                    if (innerLeft->variant.is<Expression::IntegerLiteral>()) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(tst),
                                            std::vector<const Expression*> {innerLeft, innerRight},
                                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                        );
                                    }
                                    if (innerRight->variant.is<Expression::IntegerLiteral>()) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(tst),
                                            std::vector<const Expression*> {innerRight, innerLeft},
                                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                        );
                                    }
                                }
                            }
                        }
                    }
                }

                // left == right -> { cmp(left, right); } && zero
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    if (leftRegister->definition == a || leftRegister->definition == x || leftRegister->definition == y) {
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
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isNegative()) {
                        // left < 0 -> { cmp(left, right); } && negative
                        // left >= 0 -> { cmp(left, right); } && !negative
                        if (const auto rightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                            if (rightImmediate->value.isZero()) {
                                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                                    const auto definition = leftRegister->definition;
                                    if (definition == a || definition == x || definition == y) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(cmp),
                                            std::vector<const Expression*> {left, right},
                                            std::vector<PlatformBranch> { PlatformBranch(negative, op == BinaryOperatorKind::LessThan, true) }
                                        );
                                    }
                                }

                                std::vector<InstructionOperandRoot> operandRoots;
                                operandRoots.push_back(InstructionOperandRoot(left, compiler.createOperandFromExpression(left, true)));

                                if (compiler.getBuiltins().selectInstruction(InstructionType::VoidIntrinsic(bit), compiler.getModeFlags(), operandRoots)) {
                                    return std::make_unique<PlatformTestAndBranch>(
                                        InstructionType::VoidIntrinsic(bit),
                                        std::vector<const Expression*> {left},
                                        std::vector<PlatformBranch> { PlatformBranch(negative, op == BinaryOperatorKind::LessThan, true) }
                                    );
                                }
                            }
                        }
                    } else {
                        // left < right -> { cmp(left, right); } && !carry
                        // left >= right -> { cmp(left, right); } && carry
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            const auto definition = leftRegister->definition;
                            if (definition == a || definition == x || definition == y) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> { PlatformBranch(carry, op == BinaryOperatorKind::GreaterThanOrEqual, true) }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::LessThanOrEqual: {
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isNegative()) {
                        // left <= 0 -> { cmp(left, 0); } && (zero || negative)
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a || leftRegister->definition == x || leftRegister->definition == y) {
                                if (const auto rightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                                    if (rightImmediate->value.isZero()) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(cmp),
                                            std::vector<const Expression*> {left, right},
                                            std::vector<PlatformBranch> {
                                                PlatformBranch(zero, true, true),
                                                PlatformBranch(negative, true, true)
                                            }
                                        );
                                    }
                                }
                            }
                        }
                    } else {
                        // left <= right -> { cmp(left, right); } && (zero || !carry)
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a || leftRegister->definition == x || leftRegister->definition == y) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> {
                                        PlatformBranch(zero, true, true),
                                        PlatformBranch(carry, false, true)
                                    }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::GreaterThan: {
                if (const auto integerType = type->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    if (integerType->min.isNegative()) {
                        // left > 0 -> { cmp(left, 0); } && !zero && !negative
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a || leftRegister->definition == x || leftRegister->definition == y) {
                                if (const auto rightImmediate = right->variant.tryGet<Expression::IntegerLiteral>()) {
                                    if (rightImmediate->value.isZero()) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(cmp),
                                            std::vector<const Expression*> {left, right},
                                            std::vector<PlatformBranch> {
                                                PlatformBranch(zero, true, false),
                                                PlatformBranch(negative, false, true)
                                            }
                                        );
                                    }
                                }
                            }
                        }
                    } else {
                        // left > right -> { cmp(left, right); } && !zero && carry
                        if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                            if (leftRegister->definition == a || leftRegister->definition == x || leftRegister->definition == y) {
                                return std::make_unique<PlatformTestAndBranch>(
                                    InstructionType::VoidIntrinsic(cmp),
                                    std::vector<const Expression*> {left, right},
                                    std::vector<PlatformBranch> {
                                        PlatformBranch(zero, true, false),
                                        PlatformBranch(carry, true, true)
                                    }
                                );
                            }
                        }
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::BitIndexing: {
                // left $ 6 -> { bit(left); } && overflow
                // left $ 7 -> { bit(left); } && negative
                if (const auto rightInteger = right->variant.tryGet<Expression::IntegerLiteral>()) {
                    const auto bitIndex = rightInteger->value;

                    if (bitIndex == Int128(6)) {
                        return std::make_unique<PlatformTestAndBranch>(
                            InstructionType::VoidIntrinsic(bit),
                            std::vector<const Expression*> {left},
                            std::vector<PlatformBranch> { PlatformBranch(overflow, true, true) }
                        );
                    } else if (bitIndex == Int128(7)) {
                        return std::make_unique<PlatformTestAndBranch>(
                            InstructionType::VoidIntrinsic(bit),
                            std::vector<const Expression*> {left},
                            std::vector<PlatformBranch> { PlatformBranch(negative, true, true) }
                        );
                    }
                }

                return nullptr;
            }
            default: return nullptr;
        }
    }

    Definition* Mos6502Platform::getZeroFlag() const {
        return zero;
    }

    Int128 Mos6502Platform::getPlaceholderValue() const {
        return Int128(UINT64_C(0xCCCCCCCCCCCCCCCC));
    }
}
