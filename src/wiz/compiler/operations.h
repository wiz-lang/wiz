#ifndef WIZ_COMPILER_OPERATIONS_H
#define WIZ_COMPILER_OPERATIONS_H

#include <wiz/utility/int128.h>

namespace wiz {
    enum class BinaryOperatorKind;

    bool isValidArithmeticOp(BinaryOperatorKind op);
    bool isValidComparisonOp(BinaryOperatorKind op);
    std::pair<Int128::CheckedArithmeticResult, Int128> applyIntegerArithmeticOp(BinaryOperatorKind op, Int128 left, Int128 right);
    bool applyIntegerComparisonOp(BinaryOperatorKind op, Int128 left, Int128 right);
    bool applyBooleanComparisonOp(BinaryOperatorKind op, bool left, bool right);
}

#endif