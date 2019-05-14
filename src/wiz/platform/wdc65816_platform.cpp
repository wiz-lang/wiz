#include <string>
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <tuple>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/utility/report.h>
#include <wiz/platform/wdc65816_platform.h>

namespace wiz {
    Wdc65816Platform::Wdc65816Platform() {}
    Wdc65816Platform::~Wdc65816Platform() {}

    void Wdc65816Platform::reserveDefinitions(Builtins& builtins) {
        // https://wiki.superfamicom.org/65816-reference
        // http://www.defence-force.org/computing/oric/coding/annexe_2/

        builtins.addDefineBoolean("__cpu_wdc65816"_sv, true);

        auto stringPool = builtins.getStringPool();
        auto scope = builtins.getBuiltinScope();
        const auto decl = builtins.getBuiltinDeclaration();
        const auto u8Type = builtins.getDefinition(Builtins::DefinitionType::U8);
        const auto u16Type = builtins.getDefinition(Builtins::DefinitionType::U16);
        const auto u24Type = builtins.getDefinition(Builtins::DefinitionType::U24);
        const auto boolType = builtins.getDefinition(Builtins::DefinitionType::Bool);

        pointerSizedType = u16Type;
        farPointerSizedType = u24Type;

        // Modes.
        modeMem8 = 1 << builtins.addModeAttribute(stringPool->intern("mem8"), 0);
        modeMem16 = 1 << builtins.addModeAttribute(stringPool->intern("mem16"), 0);
        modeIdx8 = 1 << builtins.addModeAttribute(stringPool->intern("idx8"), 1);
        modeIdx16 = 1 << builtins.addModeAttribute(stringPool->intern("idx16"), 1);
        modeRel = 1 << builtins.addModeAttribute(stringPool->intern("rel"), 2);
        modeAbs = 1 << builtins.addModeAttribute(stringPool->intern("abs"), 2);

        // Registers.
        a = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("a"), decl);
        x = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("x"), decl);
        y = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("y"), decl);
        const auto patternA = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(a));
        const auto patternX = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(x));
        const auto patternY = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(y));
        const auto patternS = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("s"), decl)));
        const auto patternP = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("p"), decl)));
        const auto patternDirectPageRegister = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("direct_page"), decl)));
        const auto patternProgramBank = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("program_bank"), decl)));
        const auto patternDataBank = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u8Type), stringPool->intern("data_bank"), decl)));

        aa = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("aa"), decl);
        xx = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("xx"), decl);
        yy = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("yy"), decl);
        const auto patternAA = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(aa));
        const auto patternXX = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(xx));
        const auto patternYY = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(yy));
        const auto patternSS = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(u16Type), stringPool->intern("ss"), decl)));

        carry = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("carry"), decl);
        zero = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("zero"), decl);
        nointerrupt = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("nointerrupt"), decl);
        decimal = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("decimal"), decl);
        overflow = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("overflow"), decl);
        negative = scope->emplaceDefinition(nullptr, Definition::BuiltinRegister(boolType), stringPool->intern("negative"), decl);
        const auto patternCarry = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(carry));
        const auto patternZero = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(zero));
        const auto patternNoInterrupt = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(nointerrupt));
        const auto patternDecimal = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(decimal));
        const auto patternOverflow = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(overflow));
        const auto patternNegative = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Register(negative));

        // Intrinsics.
        cmp = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("cmp"), decl);
        bit = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("bit"), decl);
        const auto push = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("push"), decl);
        const auto pop = scope->emplaceDefinition(nullptr, Definition::BuiltinLoadIntrinsic(u8Type), stringPool->intern("pop"), decl);
        const auto irqcall = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("irqcall"), decl);
        const auto copcall = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("copcall"), decl);
        const auto nop = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("nop"), decl);
        const auto stop_until_reset = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("stop_until_reset"), decl);
        const auto wait_until_interrupt = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("wait_until_interrupt"), decl);
        const auto test_and_reset = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("test_and_reset"), decl);
        const auto test_and_set = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("test_and_set"), decl);
        const auto swap_bytes = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap_bytes"), decl);
        const auto swap = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("swap_carry_emulation"), decl);
        const auto transfer_increment_from_increment = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_increment_from_increment"), decl);
        const auto transfer_decrement_from_decrement = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("transfer_decrement_from_increment"), decl);
        const auto mem8 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem8"), decl);
        const auto mem16 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem16"), decl);
        const auto idx8 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("idx8"), decl);
        const auto idx16 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("idx16"), decl);
        const auto mem8_idx8 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem8_idx8"), decl);
        const auto mem8_idx16 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem8_idx16"), decl);
        const auto mem16_idx8 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem16_idx8"), decl);
        const auto mem16_idx16 = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("mem16_idx16"), decl);
        const auto debug_break = scope->emplaceDefinition(nullptr, Definition::BuiltinVoidIntrinsic(), stringPool->intern("debug_break"), decl);

        // Non-register operands.
        const auto patternFalse = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Boolean(false));
        const auto patternTrue = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Boolean(true));
        const auto patternAtLeast0 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(0)));
        const auto patternAtLeast1 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerAtLeast(Int128(1)));
        const auto pattern0 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0)));
        const auto patternImmU8 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFF)));
        const auto patternImmU16 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFFFF)));
        const auto patternImmU24 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(0), Int128(0xFFFFFF)));
        const auto patternImmI8 = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::IntegerRange(Int128(-0x80), Int128(0x7F)));
        const auto patternDirectU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                1));
        const auto patternDirectIndexedByXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternX->clone(),
                1, 1));
        const auto patternDirectIndexedByXXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternXX->clone(),
                1, 1));
        const auto patternDirectIndexedByYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternY->clone(),
                1, 1));
        const auto patternDirectIndexedByXIndirectU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    patternX->clone(),
                    1, 2)),
                1));
        const auto patternDirectIndexedByXXIndirectU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    patternXX->clone(),
                    1, 2)),
                1));
        const auto patternDirectIndirectIndexedByYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                patternY->clone(),
                1, 1));
        const auto patternDirectIndirectIndexedByYYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                patternYY->clone(),
                1, 1));
        const auto patternDirectIndirectLongIndexedByYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                patternY->clone(),
                1, 1));
        const auto patternDirectIndirectLongIndexedByYYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                patternYY->clone(),
                1, 1));
        const auto patternDirectIndirectU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                1));
        const auto patternDirectIndirectLongU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                1));
        const auto patternAbsoluteU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                1));
        const auto patternAbsoluteLongU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                1));
        const auto patternAbsoluteIndexedByXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternX->clone(),
                1, 1));
        const auto patternAbsoluteIndexedByXXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternXX->clone(),
                1, 1));
        const auto patternAbsoluteLongIndexedByXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                patternX->clone(),
                1, 1));
        const auto patternAbsoluteLongIndexedByXXU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                patternXX->clone(),
                1, 1));
        const auto patternAbsoluteIndexedByYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternY->clone(),
                1, 1));
        const auto patternAbsoluteIndexedByYYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternYY->clone(),
                1, 1));
        const auto patternStackRelativeU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternSS->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmI8->clone())),
                1, 1));
        const auto patternStackRelativeIndirectIndexedByYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    patternSS->clone(),
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmI8->clone())),
                    1, 2)),
                patternY->clone(),
                1, 1));
        const auto patternStackRelativeIndirectIndexedByYYU8
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    patternSS->clone(),
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmI8->clone())),
                    1, 2)),
                patternYY->clone(),
                1, 1));

        const auto patternDirectU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                2));
        const auto patternDirectIndexedByXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternX->clone(),
                1, 2));
        const auto patternDirectIndexedByXXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternXX->clone(),
                1, 2));
        const auto patternDirectIndexedByYYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU8->clone())),
                patternYY->clone(),
                1, 2));
        const auto patternDirectIndexedByXIndirectU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    patternX->clone(),
                    1, 2)),
                2));
        const auto patternDirectIndexedByXXIndirectU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    patternXX->clone(),
                    1, 2)),
                2));
        const auto patternDirectIndirectIndexedByYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                patternY->clone(),
                1, 2));
        const auto patternDirectIndirectIndexedByYYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                patternYY->clone(),
                1, 2));
        const auto patternDirectIndirectLongIndexedByYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                patternY->clone(),
                1, 2));
        const auto patternDirectIndirectLongIndexedByYYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                patternYY->clone(),
                1, 2));
        const auto patternDirectIndirectU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    2)),
                2));
        const auto patternDirectIndirectLongU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    false,
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmU8->clone())),
                    3)),
                2));
        const auto patternAbsoluteU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                2));
        const auto patternAbsoluteLongU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                2));
        const auto patternAbsoluteIndexedByXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternX->clone(),
                1, 2));
        const auto patternAbsoluteIndexedByXXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternXX->clone(),
                1, 2));
        const auto patternAbsoluteLongIndexedByXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                patternX->clone(),
                1, 2));
        const auto patternAbsoluteLongIndexedByXXU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                true,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU24->clone())),
                patternXX->clone(),
                1, 2));
        const auto patternAbsoluteIndexedByYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternY->clone(),
                1, 2));
        const auto patternAbsoluteIndexedByYYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternYY->clone(),
                1, 2));
        const auto patternStackRelativeU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                patternSS->clone(),
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmI8->clone())),
                1, 2));
        const auto patternStackRelativeIndirectIndexedByYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    patternSS->clone(),
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmI8->clone())),
                    1, 2)),
                patternY->clone(),
                1, 2));
        const auto patternStackRelativeIndirectIndexedByYYU16
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    false,
                    patternSS->clone(),
                    makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                        patternImmI8->clone())),
                    1, 2)),
                patternYY->clone(),
                1, 2));

        const auto patternIndirectJump
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                2));
        const auto patternIndirectLongJump
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Dereference(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                3));
        const auto patternIndirectJumpIndexedByX
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternX->clone(),
                1, 2));
        const auto patternIndirectJumpIndexedByXX
            = builtins.emplaceInstructionOperandPattern(InstructionOperandPattern::Index(
                false,
                makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(
                    patternImmU16->clone())),
                patternXX->clone(),
                1, 2));

        // Instruction encodings.
        const auto encodingImplicit = builtins.emplaceInstructionEncoding(
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
        const auto encodingU8Operand = builtins.emplaceInstructionEncoding(
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
        const auto encodingInvertedU8Operand = builtins.emplaceInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 1;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                buffer.push_back(static_cast<std::uint8_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value) ^ 0xFF);
                return true;
            });
        const auto encodingU16Operand = builtins.emplaceInstructionEncoding(
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
        const auto encodingU24Operand = builtins.emplaceInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 3;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                static_cast<void>(report);
                static_cast<void>(bank);
                static_cast<void>(location);

                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());
                const auto value = static_cast<std::uint32_t>(captureLists[options.parameter[0]][0]->variant.get<InstructionOperand::Integer>().value);
                buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
                buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
                buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
                return true;
            });
        const auto encodingPCRelativeI8Operand = builtins.emplaceInstructionEncoding(
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
        const auto encodingPCRelativeI16Operand = builtins.emplaceInstructionEncoding(
            [](const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists) {
                static_cast<void>(captureLists);
                return options.opcode.size() + 2;
            },
            [](Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location) {
                buffer.insert(buffer.end(), options.opcode.begin(), options.opcode.end());

                const auto base = static_cast<std::int32_t>(bank->getAddress().absolutePosition.get() & 0xFFFF);
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
        const auto encodingRepeatedImplicit = builtins.emplaceInstructionEncoding(
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
        const auto encodingRepeatedU8Operand = builtins.emplaceInstructionEncoding(
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
        const auto encodingRepeatedU16Operand = builtins.emplaceInstructionEncoding(
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
        const auto encodingU8OperandU8Operand = builtins.emplaceInstructionEncoding(
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

        struct ArithmeticOperandSignature {
            ArithmeticOperandSignature(
                InstructionOperandPattern* mem8Idx8Pattern,
                InstructionOperandPattern* mem8Idx16Pattern,
                const InstructionEncoding* mem8Encoding,
                InstructionOperandPattern* mem16Idx8Pattern,
                InstructionOperandPattern* mem16Idx16Pattern,
                const InstructionEncoding* mem16Encoding,
                std::uint8_t baseOpcode)
            : mem8Idx8Pattern(mem8Idx8Pattern),
            mem8Idx16Pattern(mem8Idx16Pattern),
            mem8Encoding(mem8Encoding),
            mem16Idx8Pattern(mem16Idx8Pattern),
            mem16Idx16Pattern(mem16Idx16Pattern),
            mem16Encoding(mem16Encoding),
            baseOpcode(baseOpcode) {}

            InstructionOperandPattern* mem8Idx8Pattern;
            InstructionOperandPattern* mem8Idx16Pattern;
            const InstructionEncoding* mem8Encoding;
            InstructionOperandPattern* mem16Idx8Pattern;
            InstructionOperandPattern* mem16Idx16Pattern;
            const InstructionEncoding* mem16Encoding;
            std::uint8_t baseOpcode;
        };

        const ArithmeticOperandSignature arithmeticOperandSignatures[] {
            ArithmeticOperandSignature {patternImmU8, nullptr, encodingU8Operand, patternImmU16, nullptr, encodingU16Operand, 0x09},
            ArithmeticOperandSignature {patternDirectU8, nullptr, encodingU8Operand, patternDirectU16, nullptr, encodingU8Operand, 0x05},
            ArithmeticOperandSignature {patternDirectIndexedByXU8, patternDirectIndexedByXXU8, encodingU8Operand, patternDirectIndexedByXU16, patternDirectIndexedByXXU16, encodingU8Operand, 0x15},
            ArithmeticOperandSignature {patternDirectIndirectU8, nullptr, encodingU8Operand, patternDirectIndirectU16, nullptr, encodingU8Operand, 0x12},
            ArithmeticOperandSignature {patternDirectIndirectLongU8, nullptr, encodingU8Operand, patternDirectIndirectLongU16, nullptr, encodingU8Operand, 0x07},
            ArithmeticOperandSignature {patternDirectIndexedByXIndirectU8, patternDirectIndexedByXXIndirectU8, encodingU8Operand, patternDirectIndexedByXIndirectU16, patternDirectIndexedByXXIndirectU16, encodingU8Operand, 0x01},
            ArithmeticOperandSignature {patternDirectIndirectIndexedByYU8, patternDirectIndirectIndexedByYYU8, encodingU8Operand, patternDirectIndirectIndexedByYU16, patternDirectIndirectIndexedByYYU16, encodingU8Operand, 0x11},
            ArithmeticOperandSignature {patternDirectIndirectLongIndexedByYU8, patternDirectIndirectLongIndexedByYYU8, encodingU8Operand, patternDirectIndirectLongIndexedByYU16, patternDirectIndirectLongIndexedByYYU16, encodingU8Operand, 0x17},
            ArithmeticOperandSignature {patternAbsoluteU8, nullptr, encodingU16Operand, patternAbsoluteU16, nullptr, encodingU16Operand, 0x0D},
            ArithmeticOperandSignature {patternAbsoluteLongU8, nullptr, encodingU24Operand, patternAbsoluteLongU16, nullptr, encodingU24Operand, 0x0F},
            ArithmeticOperandSignature {patternAbsoluteIndexedByXU8, patternAbsoluteIndexedByXXU8, encodingU16Operand, patternAbsoluteIndexedByXU16, patternAbsoluteIndexedByXXU16, encodingU16Operand, 0x1D},
            ArithmeticOperandSignature {patternAbsoluteLongIndexedByXU8, patternAbsoluteLongIndexedByXXU8, encodingU24Operand, patternAbsoluteLongIndexedByXU16, patternAbsoluteLongIndexedByXXU16, encodingU24Operand, 0x1F},
            ArithmeticOperandSignature {patternAbsoluteIndexedByYU8, patternAbsoluteIndexedByYYU8, encodingU16Operand, patternAbsoluteIndexedByYU16, patternAbsoluteIndexedByYYU16, encodingU16Operand, 0x19},
            ArithmeticOperandSignature {patternStackRelativeU8, nullptr, encodingU8Operand, patternStackRelativeU16, nullptr, encodingU8Operand, 0x03},
            ArithmeticOperandSignature {patternStackRelativeIndirectIndexedByYU8, patternStackRelativeIndirectIndexedByYYU8, encodingU8Operand, patternStackRelativeIndirectIndexedByYU16, patternStackRelativeIndirectIndexedByYYU16, encodingU8Operand, 0x13},
        };
        //  lda, accumulator arithmetic
        for (const auto& op : arithmeticOperators) {
            for (const auto& sig : arithmeticOperandSignatures) {
                std::vector<std::uint8_t> opcode = op.second;
                opcode[opcode.size() - 1] |= sig.baseOpcode;

                builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | (sig.mem8Idx16Pattern != nullptr ? modeIdx8 : 0), {patternA, sig.mem8Idx8Pattern}), sig.mem8Encoding, InstructionOptions(std::move(opcode), {1}, {}));
                if (sig.mem8Idx16Pattern != nullptr) { 
                    builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | modeIdx16, {patternA, sig.mem8Idx16Pattern}), sig.mem8Encoding, InstructionOptions(std::move(opcode), {1}, {}));
                }
                builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | (sig.mem16Idx16Pattern != nullptr ? modeIdx8 : 0), {patternAA, sig.mem16Idx8Pattern}), sig.mem16Encoding, InstructionOptions(std::move(opcode), {1}, {}));
                if (sig.mem16Idx16Pattern != nullptr) { 
                    builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | modeIdx16, {patternAA, sig.mem16Idx16Pattern}), sig.mem16Encoding, InstructionOptions(std::move(opcode), {1}, {}));
                }
            }
        }
        // sta
        for (const auto& sig : arithmeticOperandSignatures) {
            if (sig.mem8Idx8Pattern == patternImmU8) {
                continue;
            }

            std::vector<std::uint8_t> opcode {static_cast<std::uint8_t>(0x80 | sig.baseOpcode)};
            builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | (sig.mem8Idx16Pattern != nullptr ? modeIdx8 : 0), {sig.mem8Idx8Pattern, patternA}), sig.mem8Encoding, InstructionOptions(std::move(opcode), {0}, {}));
            if (sig.mem8Idx16Pattern != nullptr) { 
                builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | modeIdx16, {sig.mem8Idx16Pattern, patternA}), sig.mem8Encoding, InstructionOptions(std::move(opcode), {0}, {}));
            }
            builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | (sig.mem16Idx16Pattern != nullptr ? modeIdx8 : 0), {sig.mem16Idx8Pattern, patternAA}), sig.mem16Encoding, InstructionOptions(std::move(opcode), {0}, {}));
            if (sig.mem16Idx16Pattern != nullptr) { 
                builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | modeIdx16, {sig.mem16Idx16Pattern, patternAA}), sig.mem16Encoding, InstructionOptions(std::move(opcode), {0}, {}));
            }
        }
        // bit - overflow = mem $ 6, negative = mem $ 7, zero = mem & a
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8, {patternDirectU8}), encodingU8Operand, InstructionOptions({0x24}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8, {patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0x2C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8, {patternImmU8}), encodingU8Operand, InstructionOptions({0x89}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8 | modeIdx8, {patternDirectIndexedByXU8}), encodingU8Operand, InstructionOptions({0x34}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8 | modeIdx16, {patternDirectIndexedByXXU8}), encodingU8Operand, InstructionOptions({0x34}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8 | modeIdx8, {patternAbsoluteIndexedByXU8}), encodingU16Operand, InstructionOptions({0x3C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem8 | modeIdx16, {patternAbsoluteIndexedByXXU8}), encodingU16Operand, InstructionOptions({0x3C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16, {patternDirectU16}), encodingU8Operand, InstructionOptions({0x24}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16, {patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0x2C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16, {patternImmU16}), encodingU16Operand, InstructionOptions({0x89}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16 | modeIdx8, {patternDirectIndexedByXU16}), encodingU8Operand, InstructionOptions({0x34}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16 | modeIdx16, {patternDirectIndexedByXXU16}), encodingU8Operand, InstructionOptions({0x34}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16 | modeIdx8, {patternAbsoluteIndexedByXU16}), encodingU16Operand, InstructionOptions({0x3C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(bit), modeMem16 | modeIdx16, {patternAbsoluteIndexedByXXU16}), encodingU16Operand, InstructionOptions({0x3C}, {0}, {}));
        // ldx
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternX, patternImmU8}), encodingU8Operand, InstructionOptions({0xA2}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternX, patternDirectU8}), encodingU8Operand, InstructionOptions({0xA6}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternX, patternDirectIndexedByYU8}), encodingU8Operand, InstructionOptions({0xB6}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternX, patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xAE}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternX, patternAbsoluteIndexedByYU8}), encodingU16Operand, InstructionOptions({0xBE}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternXX, patternImmU16}), encodingU16Operand, InstructionOptions({0xA2}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternXX, patternDirectU16}), encodingU8Operand, InstructionOptions({0xA6}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternXX, patternDirectIndexedByYYU16}), encodingU8Operand, InstructionOptions({0xB6}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternXX, patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xAE}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternXX, patternAbsoluteIndexedByYYU16}), encodingU16Operand, InstructionOptions({0xBE}, {1}, {}));
        // ldy
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternY, patternImmU8}), encodingU8Operand, InstructionOptions({0xA0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternY, patternDirectU8}), encodingU8Operand, InstructionOptions({0xA4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternY, patternDirectIndexedByXU8}), encodingU8Operand, InstructionOptions({0xB4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternY, patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xAC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternY, patternAbsoluteIndexedByXU8}), encodingU16Operand, InstructionOptions({0xBC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternYY, patternImmU16}), encodingU16Operand, InstructionOptions({0xA0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternYY, patternDirectU16}), encodingU8Operand, InstructionOptions({0xA4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternYY, patternDirectIndexedByXXU16}), encodingU8Operand, InstructionOptions({0xB4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternYY, patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xAC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternYY, patternAbsoluteIndexedByXXU16}), encodingU16Operand, InstructionOptions({0xBC}, {1}, {}));
        // stx
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternDirectU8, patternX}), encodingU8Operand, InstructionOptions({0x86}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternDirectIndexedByYU8, patternX}), encodingU8Operand, InstructionOptions({0x96}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternAbsoluteU8, patternX}), encodingU16Operand, InstructionOptions({0x8E}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternDirectU16, patternXX}), encodingU8Operand, InstructionOptions({0x86}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternDirectIndexedByYYU16, patternXX}), encodingU8Operand, InstructionOptions({0x96}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternAbsoluteU16, patternXX}), encodingU16Operand, InstructionOptions({0x8E}, {0}, {}));
        // sty
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternDirectU8, patternY}), encodingU8Operand, InstructionOptions({0x84}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternDirectIndexedByXU8, patternY}), encodingU8Operand, InstructionOptions({0x94}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx8, {patternAbsoluteU8, patternY}), encodingU16Operand, InstructionOptions({0x8C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternDirectU16, patternYY}), encodingU8Operand, InstructionOptions({0x84}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternDirectIndexedByXXU16, patternYY}), encodingU8Operand, InstructionOptions({0x94}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeIdx16, {patternAbsoluteU16, patternYY}), encodingU16Operand, InstructionOptions({0x8C}, {0}, {}));
        // stz
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8, {patternDirectU8, pattern0}), encodingU8Operand, InstructionOptions({0x64}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | modeIdx8, {patternDirectIndexedByXU8, pattern0}), encodingU8Operand, InstructionOptions({0x74}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | modeIdx16, {patternDirectIndexedByXXU8, pattern0}), encodingU8Operand, InstructionOptions({0x74}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8, {patternAbsoluteU8, pattern0}), encodingU16Operand, InstructionOptions({0x9C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | modeIdx8, {patternAbsoluteIndexedByXU8, pattern0}), encodingU16Operand, InstructionOptions({0x9E}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem8 | modeIdx16, {patternAbsoluteIndexedByXXU8, pattern0}), encodingU16Operand, InstructionOptions({0x9E}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16, {patternDirectU16, pattern0}), encodingU8Operand, InstructionOptions({0x64}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | modeIdx8, {patternDirectIndexedByXU16, pattern0}), encodingU8Operand, InstructionOptions({0x74}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | modeIdx16, {patternDirectIndexedByXXU16, pattern0}), encodingU8Operand, InstructionOptions({0x74}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16, {patternAbsoluteU16, pattern0}), encodingU16Operand, InstructionOptions({0x9C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | modeIdx8, {patternAbsoluteIndexedByXU16, pattern0}), encodingU16Operand, InstructionOptions({0x9E}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, modeMem16 | modeIdx16, {patternAbsoluteIndexedByXXU16, pattern0}), encodingU16Operand, InstructionOptions({0x9E}, {0}, {}));
        // cpx
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternX, patternImmU8}), encodingU8Operand, InstructionOptions({0xE0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternX, patternDirectU8}), encodingU8Operand, InstructionOptions({0xE4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternX, patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xEC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternXX, patternImmU16}), encodingU16Operand, InstructionOptions({0xE0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternXX, patternDirectU16}), encodingU8Operand, InstructionOptions({0xE4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternXX, patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xEC}, {1}, {}));
        // cpy
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternY, patternImmU8}), encodingU8Operand, InstructionOptions({0xC0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternY, patternDirectU8}), encodingU8Operand, InstructionOptions({0xC4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx8, {patternY, patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xCC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternYY, patternImmU16}), encodingU16Operand, InstructionOptions({0xC0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternYY, patternDirectU16}), encodingU8Operand, InstructionOptions({0xC4}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(cmp), modeIdx16, {patternYY, patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xCC}, {1}, {}));
        // transfer instructions
        using TransferOp = std::tuple<InstructionOperandPattern*, InstructionOperandPattern*, std::uint32_t, std::uint8_t>;
        const TransferOp transferOps[] {
            TransferOp {patternA, patternX, modeMem8 | modeIdx8, 0x8A},
            TransferOp {patternX, patternA, modeMem8 | modeIdx8, 0xAA},
            TransferOp {patternA, patternX, modeMem8 | modeIdx16, 0x8A},
            TransferOp {patternXX, patternAA, modeMem8 | modeIdx16, 0xAA},
            TransferOp {patternAA, patternXX, modeMem16 | modeIdx8, 0x8A},
            TransferOp {patternX, patternA, modeMem16 | modeIdx8, 0xAA},
            TransferOp {patternAA, patternXX, modeMem16 | modeIdx16, 0x8A},
            TransferOp {patternXX, patternAA, modeMem16 | modeIdx16, 0xAA},

            TransferOp {patternA, patternY, modeMem8 | modeIdx8, 0x98},
            TransferOp {patternY, patternA, modeMem8 | modeIdx8, 0xA8},
            TransferOp {patternA, patternY, modeMem8 | modeIdx16, 0x98},
            TransferOp {patternYY, patternAA, modeMem8 | modeIdx16, 0xA8},
            TransferOp {patternAA, patternYY, modeMem16 | modeIdx8, 0x98},
            TransferOp {patternY, patternA, modeMem16 | modeIdx8, 0xA8},
            TransferOp {patternAA, patternYY, modeMem16 | modeIdx16, 0x98},
            TransferOp {patternYY, patternAA, modeMem16 | modeIdx16, 0xA8},

            TransferOp {patternX, patternS, modeIdx8, 0xBA},
            TransferOp {patternS, patternX, modeIdx8, 0x9A},
            TransferOp {patternXX, patternSS, modeIdx16, 0xBA},
            TransferOp {patternSS, patternXX, modeIdx16, 0x9A},

            TransferOp {patternAA, patternSS, 0, 0x3B},
            TransferOp {patternSS, patternAA, 0, 0x1B},

            TransferOp {patternAA, patternDirectPageRegister, 0, 0x7B},
            TransferOp {patternDirectPageRegister, patternAA, 0, 0x5B},

            TransferOp {patternX, patternY, modeIdx8, 0xBB},
            TransferOp {patternY, patternX, modeIdx8, 0x9B},
            TransferOp {patternXX, patternYY, modeIdx16, 0xBB},
            TransferOp {patternYY, patternXX, modeIdx16, 0x9B},
        };
        for (const auto& op : transferOps) {
            auto left = std::get<0>(op);
            auto right = std::get<1>(op);

            builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, std::get<2>(op), {left, right}), encodingImplicit, InstructionOptions({std::get<3>(op)}, {}, {}));
        }
        // push
        // TODO: separate into push8 and push16, so the type checker works
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternImmU16}), encodingU16Operand, InstructionOptions({0xF4}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternDirectU16}), encodingU8Operand, InstructionOptions({0xD4}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternImmU16}), encodingPCRelativeI16Operand, InstructionOptions({0x62}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x48}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x48}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternP}), encodingImplicit, InstructionOptions({0x08}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeIdx8, {patternX}), encodingImplicit, InstructionOptions({0xDA}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeIdx16, {patternXX}), encodingImplicit, InstructionOptions({0xDA}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeIdx8, {patternY}), encodingImplicit, InstructionOptions({0x5A}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), modeIdx16, {patternYY}), encodingImplicit, InstructionOptions({0x5A}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternDirectPageRegister}), encodingImplicit, InstructionOptions({0x0B}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternDataBank}), encodingImplicit, InstructionOptions({0x8B}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(push), 0, {patternProgramBank}), encodingImplicit, InstructionOptions({0x4B}, {}, {}));
        // pop
        // TODO: separate into pop8 and pop16, so the type checker works
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x68}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x68}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternP}), encodingImplicit, InstructionOptions({0x28}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeIdx8, {patternX}), encodingImplicit, InstructionOptions({0xFA}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeIdx16, {patternXX}), encodingImplicit, InstructionOptions({0xFA}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeIdx8, {patternY}), encodingImplicit, InstructionOptions({0x7A}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), modeIdx16, {patternYY}), encodingImplicit, InstructionOptions({0x7A}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternDirectPageRegister}), encodingImplicit, InstructionOptions({0x2B}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::LoadIntrinsic(pop), 0, {patternDataBank}), encodingImplicit, InstructionOptions({0xAB}, {}, {}));
        // increment
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8, {patternDirectU8}), encodingU8Operand, InstructionOptions({0xE6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8 | modeIdx8, {patternDirectIndexedByXU8}), encodingU8Operand, InstructionOptions({0xF6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8 | modeIdx16, {patternDirectIndexedByXXU8}), encodingU8Operand, InstructionOptions({0xF6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8, {patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xEE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8 | modeIdx8, {patternAbsoluteIndexedByXU8}), encodingU16Operand, InstructionOptions({0xFE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8 | modeIdx16, {patternAbsoluteIndexedByXXU8}), encodingU16Operand, InstructionOptions({0xFE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16, {patternDirectU16}), encodingU8Operand, InstructionOptions({0xE6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16 | modeIdx8, {patternDirectIndexedByXU16}), encodingU8Operand, InstructionOptions({0xF6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16 | modeIdx16, {patternDirectIndexedByXXU16}), encodingU8Operand, InstructionOptions({0xF6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16, {patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xEE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16 | modeIdx8, {patternAbsoluteIndexedByXU16}), encodingU16Operand, InstructionOptions({0xFE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16 | modeIdx16, {patternAbsoluteIndexedByXXU16}), encodingU16Operand, InstructionOptions({0xFE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeIdx8, {patternX}), encodingImplicit, InstructionOptions({0xE8}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeIdx16, {patternXX}), encodingImplicit, InstructionOptions({0xE8}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeIdx8, {patternY}), encodingImplicit, InstructionOptions({0xC8}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeIdx16, {patternYY}), encodingImplicit, InstructionOptions({0xC8}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x1A}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreIncrement, modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x1A}, {}, {zero}));
        // decrement
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8, {patternDirectU8}), encodingU8Operand, InstructionOptions({0xC6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8 | modeIdx8, {patternDirectIndexedByXU8}), encodingU8Operand, InstructionOptions({0xD6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8 | modeIdx16, {patternDirectIndexedByXXU8}), encodingU8Operand, InstructionOptions({0xD6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8, {patternAbsoluteU8}), encodingU16Operand, InstructionOptions({0xCE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8 | modeIdx8, {patternAbsoluteIndexedByXU8}), encodingU16Operand, InstructionOptions({0xDE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8 | modeIdx16, {patternAbsoluteIndexedByXXU8}), encodingU16Operand, InstructionOptions({0xDE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16, {patternDirectU16}), encodingU8Operand, InstructionOptions({0xC6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16 | modeIdx8, {patternDirectIndexedByXU16}), encodingU8Operand, InstructionOptions({0xD6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16 | modeIdx16, {patternDirectIndexedByXXU16}), encodingU8Operand, InstructionOptions({0xD6}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16, {patternAbsoluteU16}), encodingU16Operand, InstructionOptions({0xCE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16 | modeIdx8, {patternAbsoluteIndexedByXU16}), encodingU16Operand, InstructionOptions({0xDE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16 | modeIdx16, {patternAbsoluteIndexedByXXU16}), encodingU16Operand, InstructionOptions({0xDE}, {0}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeIdx8, {patternX}), encodingImplicit, InstructionOptions({0xCA}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeIdx16, {patternXX}), encodingImplicit, InstructionOptions({0xCA}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeIdx8, {patternY}), encodingImplicit, InstructionOptions({0x88}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeIdx16, {patternYY}), encodingImplicit, InstructionOptions({0x88}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x3A}, {}, {zero}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::PreDecrement, modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x3A}, {}, {zero}));
        // bitwise negation
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::BitwiseNegation, modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x49, 0xFF}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::BitwiseNegation, modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x49, 0xFF, 0xFF}, {}, {}));
        // signed negation
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::SignedNegation, modeMem8, {patternA}), encodingImplicit, InstructionOptions({0x49, 0xFF, 0x18, 0x69, 0x01}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(UnaryOperatorKind::SignedNegation, modeMem16, {patternAA}), encodingImplicit, InstructionOptions({0x49, 0xFF, 0xFF, 0x18, 0x69, 0x01, 0x00}, {}, {}));
        // bitshifts
        const std::pair<InstructionType, std::uint8_t> shiftOperators[] {
            {BinaryOperatorKind::LeftShift, 0x00},
            {BinaryOperatorKind::LogicalLeftShift, 0x00},
            {BinaryOperatorKind::LeftRotateWithCarry, 0x20},
            {BinaryOperatorKind::LogicalRightShift, 0x40},
            {BinaryOperatorKind::RightRotateWithCarry, 0x60},
        };
        for (const auto& op : shiftOperators) {
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8, {patternA, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0A)}, {1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8, {patternDirectU8, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x06)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | modeIdx8, {patternDirectIndexedByXU8, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x16)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | modeIdx16, {patternDirectIndexedByXXU8, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x16)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8, {patternAbsoluteU8, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0E)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | modeIdx8, {patternAbsoluteIndexedByXU8, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x1E)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem8 | modeIdx16, {patternAbsoluteIndexedByXXU8, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x1E)}, {0, 1}, {}));

            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16, {patternAA, patternImmU8}), encodingRepeatedImplicit, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0A)}, {1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16, {patternDirectU16, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x06)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | modeIdx8, {patternDirectIndexedByXU16, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x16)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | modeIdx16, {patternDirectIndexedByXXU16, patternImmU8}), encodingRepeatedU8Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x16)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16, {patternAbsoluteU16, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x0E)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | modeIdx8, {patternAbsoluteIndexedByXU16, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x1E)}, {0, 1}, {}));
            builtins.emplaceInstruction(InstructionSignature(op.first, modeMem16 | modeIdx16, {patternAbsoluteIndexedByXXU16, patternImmU8}), encodingRepeatedU16Operand, InstructionOptions({static_cast<std::uint8_t>(op.second | 0x1E)}, {0, 1}, {}));
        }
        // jump / branch instructions
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16}), encodingPCRelativeI8Operand, InstructionOptions({0x80}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16}), encodingPCRelativeI8Operand, InstructionOptions({0x80}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectJump}), encodingU16Operand, InstructionOptions({0x6C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectJumpIndexedByX}), encodingU16Operand, InstructionOptions({0x7C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectJumpIndexedByXX}), encodingU16Operand, InstructionOptions({0x7C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::FarGoto, 0, {patternAtLeast0, patternImmU24}), encodingU24Operand, InstructionOptions({0x5C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast0, patternIndirectJump}), encodingU16Operand, InstructionOptions({0x6C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast0, patternIndirectJumpIndexedByX}), encodingU16Operand, InstructionOptions({0x7C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast0, patternIndirectJumpIndexedByXX}), encodingU16Operand, InstructionOptions({0x7C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::FarGoto, modeAbs, {patternAtLeast0, patternImmU24}), encodingU24Operand, InstructionOptions({0x5C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternIndirectLongJump}), encodingU16Operand, InstructionOptions({0xDC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x90}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternCarry, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xB0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0xD0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternZero, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xF0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternNegative, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x10}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternNegative, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x30}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternOverflow, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x50}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast0, patternImmU16, patternOverflow, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x70}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternCarry, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x90}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternCarry, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xB0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternZero, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0xD0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternZero, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0xF0}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternNegative, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x10}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternNegative, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x30}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternOverflow, patternFalse}), encodingPCRelativeI8Operand, InstructionOptions({0x50}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast0, patternImmU16, patternOverflow, patternTrue}), encodingPCRelativeI8Operand, InstructionOptions({0x70}, {1}, {}));
        // long branch instructions
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16}), encodingU16Operand, InstructionOptions({0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16}), encodingU16Operand, InstructionOptions({0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16}), encodingPCRelativeI16Operand, InstructionOptions({0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternFalse}), encodingU16Operand, InstructionOptions({0xB0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternCarry, patternTrue}), encodingU16Operand, InstructionOptions({0x90, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternFalse}), encodingU16Operand, InstructionOptions({0xF0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternZero, patternTrue}), encodingU16Operand, InstructionOptions({0xD0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternNegative, patternFalse}), encodingU16Operand, InstructionOptions({0x30, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternNegative, patternTrue}), encodingU16Operand, InstructionOptions({0x10, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternOverflow, patternFalse}), encodingU16Operand, InstructionOptions({0x70, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, 0, {patternAtLeast1, patternImmU16, patternOverflow, patternTrue}), encodingU16Operand, InstructionOptions({0x50, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternCarry, patternFalse}), encodingPCRelativeI16Operand, InstructionOptions({0xB0, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternCarry, patternTrue}), encodingPCRelativeI16Operand, InstructionOptions({0x90, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternZero, patternFalse}), encodingPCRelativeI16Operand, InstructionOptions({0xF0, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternZero, patternTrue}), encodingPCRelativeI16Operand, InstructionOptions({0xD0, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternNegative, patternFalse}), encodingPCRelativeI16Operand, InstructionOptions({0x30, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternNegative, patternTrue}), encodingPCRelativeI16Operand, InstructionOptions({0x10, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternOverflow, patternFalse}), encodingPCRelativeI16Operand, InstructionOptions({0x70, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeRel, {patternAtLeast1, patternImmU16, patternOverflow, patternTrue}), encodingPCRelativeI16Operand, InstructionOptions({0x50, 3, 0x82}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternCarry, patternFalse}), encodingU16Operand, InstructionOptions({0xB0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternCarry, patternTrue}), encodingU16Operand, InstructionOptions({0x90, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternZero, patternFalse}), encodingU16Operand, InstructionOptions({0xF0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternZero, patternTrue}), encodingU16Operand, InstructionOptions({0xD0, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternNegative, patternFalse}), encodingU16Operand, InstructionOptions({0x30, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternNegative, patternTrue}), encodingU16Operand, InstructionOptions({0x10, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternOverflow, patternFalse}), encodingU16Operand, InstructionOptions({0x70, 3, 0x4C}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Goto, modeAbs, {patternAtLeast1, patternImmU16, patternOverflow, patternTrue}), encodingU16Operand, InstructionOptions({0x50, 3, 0x4C}, {1}, {}));
        // call instructions
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternImmU16}), encodingU16Operand, InstructionOptions({0x20}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::FarCall, 0, {patternAtLeast0, patternImmU24}), encodingU24Operand, InstructionOptions({0x22}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternIndirectJumpIndexedByX}), encodingU16Operand, InstructionOptions({0xFC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, 0, {patternAtLeast0, patternIndirectJumpIndexedByXX}), encodingU16Operand, InstructionOptions({0xFC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, modeAbs, {patternAtLeast0, patternImmU16}), encodingU16Operand, InstructionOptions({0x20}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::FarCall, modeAbs, {patternAtLeast0, patternImmU24}), encodingU24Operand, InstructionOptions({0x22}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, modeAbs, {patternAtLeast0, patternIndirectJumpIndexedByX}), encodingU16Operand, InstructionOptions({0xFC}, {1}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Call, modeAbs, {patternAtLeast0, patternIndirectJumpIndexedByXX}), encodingU16Operand, InstructionOptions({0xFC}, {1}, {}));
        // return instructions
        builtins.emplaceInstruction(InstructionSignature(BranchKind::Return, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x60}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::FarReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x6B}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::IrqReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x40}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BranchKind::NmiReturn, 0, {patternAtLeast0}), encodingImplicit, InstructionOptions({0x40}, {}, {}));
        // brk
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(irqcall), 0, {}), encodingImplicit, InstructionOptions({0x00}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(irqcall), 0, {patternImmU8}), encodingU8Operand, InstructionOptions({0x00}, {0}, {}));
        // cop
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(copcall), 0, {patternImmU8}), encodingU8Operand, InstructionOptions({0x00}, {0}, {}));
        // nop
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(nop), 0, {}), encodingImplicit, InstructionOptions({0xEA}, {}, {}));
        // carry - clc/sec
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternCarry, patternFalse}), encodingImplicit, InstructionOptions({0x18}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternCarry, patternTrue}), encodingImplicit, InstructionOptions({0x38}, {}, {}));
        // decimal - cld/sed
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternDecimal, patternFalse}), encodingImplicit, InstructionOptions({0xD8}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternDecimal, patternTrue}), encodingImplicit, InstructionOptions({0xF8}, {}, {}));
        // interrupt - cli/sei
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternNoInterrupt, patternFalse}), encodingImplicit, InstructionOptions({0x58}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternNoInterrupt, patternTrue}), encodingImplicit, InstructionOptions({0x78}, {}, {}));
        // overflow - clv
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::Assignment, 0, {patternOverflow, patternFalse}), encodingImplicit, InstructionOptions({0xB8}, {}, {}));
        // trb
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), modeMem8, {patternDirectU8, patternA}), encodingU8Operand, InstructionOptions({0x14}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), modeMem8, {patternAbsoluteU8, patternA}), encodingU16Operand, InstructionOptions({0x1C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), modeMem16, {patternDirectU16, patternAA}), encodingU8Operand, InstructionOptions({0x14}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_reset), modeMem16, {patternAbsoluteU16, patternAA}), encodingU16Operand, InstructionOptions({0x1C}, {0}, {}));
        // tsb
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), modeMem8, {patternDirectU8, patternA}), encodingU8Operand, InstructionOptions({0x04}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), modeMem8, {patternAbsoluteU8, patternA}), encodingU16Operand, InstructionOptions({0x0C}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), modeMem16, {patternDirectU16, patternAA}), encodingU8Operand, InstructionOptions({0x04}, {0}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(test_and_set), modeMem16, {patternAbsoluteU16, patternAA}), encodingU16Operand, InstructionOptions({0x0C}, {0}, {}));
        // sep
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::BitwiseOr, 0, {patternP, patternImmU8}), encodingU8Operand, InstructionOptions({0xE2}, {1}, {}));
        // rep
        builtins.emplaceInstruction(InstructionSignature(BinaryOperatorKind::BitwiseAnd, 0, {patternP, patternImmU8}), encodingInvertedU8Operand, InstructionOptions({0xC2}, {1}, {}));
        // mvp
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_increment_from_increment), 0, {patternImmU8, patternX, patternImmU8, patternY, patternA}), encodingU8OperandU8Operand, InstructionOptions({0x44}, {0, 1}, {}));
        // mvn
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(transfer_decrement_from_decrement), 0, {patternImmU8, patternX, patternImmU8, patternY, patternA}), encodingU8OperandU8Operand, InstructionOptions({0x54}, {0, 1}, {}));
        // stp
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(stop_until_reset), 0, {}), encodingImplicit, InstructionOptions({0xDB}, {}, {}));
        // wai
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(wait_until_interrupt), 0, {}), encodingImplicit, InstructionOptions({0xCB}, {}, {}));
        // xba
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(swap_bytes), 0, {patternAA}), encodingImplicit, InstructionOptions({0xEB}, {}, {}));
        // xce
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(swap), 0, {}), encodingImplicit, InstructionOptions({0xFB}, {}, {}));
        // rep/sep instrinsics for convenience.
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem8), 0, {}), encodingImplicit, InstructionOptions({0xE2, 0x20}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem16), 0, {}), encodingImplicit, InstructionOptions({0xC2, 0x20}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(idx8), 0, {}), encodingImplicit, InstructionOptions({0xE2, 0x10}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(idx16), 0, {}), encodingImplicit, InstructionOptions({0xC2, 0x10}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem8_idx8), 0, {}), encodingImplicit, InstructionOptions({0xE2, 0x30}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem8_idx16), 0, {}), encodingImplicit, InstructionOptions({0xE2, 0x20, 0xC2, 0x10}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem16_idx8), 0, {}), encodingImplicit, InstructionOptions({0xE2, 0x10, 0xC2, 0x20}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(mem16_idx16), 0, {}), encodingImplicit, InstructionOptions({0xC2, 0x30}, {}, {}));
        // wdm
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(debug_break), 0, {}), encodingImplicit, InstructionOptions({0x42, 0xEA}, {}, {}));
        builtins.emplaceInstruction(InstructionSignature(InstructionType::VoidIntrinsic(debug_break), 0, {patternImmU8}), encodingU8Operand, InstructionOptions({0x42}, {}, {}));
    }

    Definition* Wdc65816Platform::getPointerSizedType() const {
        return pointerSizedType;
    }

    Definition* Wdc65816Platform::getFarPointerSizedType() const {
        return farPointerSizedType;
    }

    std::unique_ptr<PlatformTestAndBranch> Wdc65816Platform::getTestAndBranch(const Compiler& compiler, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const {
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
                                    if (leftRegister->definition == a || leftRegister->definition == aa) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(bit),
                                            std::vector<const Expression*> {innerRight},
                                            std::vector<PlatformBranch> { PlatformBranch(zero, op == BinaryOperatorKind::Equal, true) }
                                        );
                                    }
                                } else if (const auto& rightRegister = innerRight->variant.tryGet<Expression::ResolvedIdentifier>()) {
                                    if (rightRegister->definition == a || rightRegister->definition == aa) {
                                        return std::make_unique<PlatformTestAndBranch>(
                                            InstructionType::VoidIntrinsic(bit),
                                            std::vector<const Expression*> {innerLeft},
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
                    const auto definition = leftRegister->definition;
                    if (definition == a || definition == x || definition == y || definition == aa || definition == xx || definition == yy) {
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
                // left == right -> { cmp(left, right); } && !carry
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    const auto definition = leftRegister->definition;
                    if (definition == a || definition == x || definition == y || definition == aa || definition == xx || definition == yy) {
                        return std::make_unique<PlatformTestAndBranch>(
                            InstructionType::VoidIntrinsic(cmp),
                            std::vector<const Expression*> {left, right},
                            std::vector<PlatformBranch> { PlatformBranch(carry, op == BinaryOperatorKind::GreaterThanOrEqual, true) }
                        );
                    }
                }

                return nullptr;
            }
            case BinaryOperatorKind::LessThanOrEqual: {
                // left == right -> { cmp(left, right); } && (zero || !carry)
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    const auto definition = leftRegister->definition;
                    if (definition == a || definition == x || definition == y || definition == aa || definition == xx || definition == yy) {
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

                return nullptr;
            }
            case BinaryOperatorKind::GreaterThan: {
                // left == right -> { cmp(left, right); } && !zero && carry
                if (const auto leftRegister = left->variant.tryGet<Expression::ResolvedIdentifier>()) {
                    const auto definition = leftRegister->definition;
                    if (definition == a || definition == x || definition == y || definition == aa || definition == xx || definition == yy) {
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

    Definition* Wdc65816Platform::getZeroFlag() const {
        return zero;
    }

    Int128 Wdc65816Platform::getPlaceholderValue() const {
        return Int128(UINT64_C(0xCCCCCCCCCCCCCCCC));
    }
}