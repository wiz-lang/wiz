#include <wiz/ast/expression.h>
#include <wiz/compiler/operations.h>

namespace wiz {
    bool isValidArithmeticOp(BinaryOperatorKind op) {
        switch (op) {
            case BinaryOperatorKind::Addition:
            case BinaryOperatorKind::BitwiseAnd:
            case BinaryOperatorKind::BitwiseOr:
            case BinaryOperatorKind::BitwiseXor:
            case BinaryOperatorKind::Division:
            case BinaryOperatorKind::Modulo:
            case BinaryOperatorKind::Multiplication:
            case BinaryOperatorKind::LeftShift:
            case BinaryOperatorKind::RightShift:
            case BinaryOperatorKind::LogicalLeftShift:
            case BinaryOperatorKind::LogicalRightShift:
            case BinaryOperatorKind::Subtraction:
                return true;
            default:
                return false;
        }
    }

    bool isValidComparisonOp(BinaryOperatorKind op) {
        switch (op) {
            case BinaryOperatorKind::LessThan:
            case BinaryOperatorKind::GreaterThan:
            case BinaryOperatorKind::LessThanOrEqual:
            case BinaryOperatorKind::GreaterThanOrEqual:
            case BinaryOperatorKind::Equal:
            case BinaryOperatorKind::NotEqual:
                return true;
            default:
                return false;
        }
    }

    std::pair<Int128::CheckedArithmeticResult, Int128> applyIntegerArithmeticOp(BinaryOperatorKind op, Int128 left, Int128 right) {
        switch (op) {
            case BinaryOperatorKind::Addition: return left.checkedAdd(right);
            case BinaryOperatorKind::BitwiseAnd: return {Int128::CheckedArithmeticResult::Success, left & right};
            case BinaryOperatorKind::BitwiseOr: return {Int128::CheckedArithmeticResult::Success, left | right};
            case BinaryOperatorKind::BitwiseXor: return {Int128::CheckedArithmeticResult::Success, left ^ right};
            case BinaryOperatorKind::Division: return left.checkedDivide(right);
            case BinaryOperatorKind::Modulo: return left.checkedModulo(right);
            case BinaryOperatorKind::Multiplication: return left.checkedMultiply(right);
            case BinaryOperatorKind::LeftShift:
            case BinaryOperatorKind::LogicalLeftShift: {
                std::size_t bits = right > Int128(SIZE_MAX) ? SIZE_MAX : static_cast<std::size_t>(right);
                return left.checkedLogicalLeftShift(bits);
            }
            case BinaryOperatorKind::RightShift: {
                std::size_t bits = right > Int128(SIZE_MAX) ? SIZE_MAX : static_cast<std::size_t>(right);
                return {Int128::CheckedArithmeticResult::Success, left.arithmeticRightShift(bits)};
            }
            case BinaryOperatorKind::LogicalRightShift: {
                std::size_t bits = right > Int128(SIZE_MAX) ? SIZE_MAX : static_cast<std::size_t>(right);
                return {Int128::CheckedArithmeticResult::Success, left.logicalRightShift(bits)};
            }
            case BinaryOperatorKind::Subtraction: return left.checkedSubtract(right);
            default: assert(false); return {};
        }
    }

    bool applyIntegerComparisonOp(BinaryOperatorKind op, Int128 left, Int128 right) {
        switch (op) {
            case BinaryOperatorKind::LessThan: return left < right;
            case BinaryOperatorKind::GreaterThan: return left > right;
            case BinaryOperatorKind::LessThanOrEqual: return left <= right;
            case BinaryOperatorKind::GreaterThanOrEqual: return left >= right;
            case BinaryOperatorKind::Equal: return left == right;
            case BinaryOperatorKind::NotEqual: return left != right;
            default: std::abort(); return false;
        }
    }

    bool applyBooleanComparisonOp(BinaryOperatorKind op, bool left, bool right) {
        switch (op) {
            case BinaryOperatorKind::LessThan: return left < right;
            case BinaryOperatorKind::GreaterThan: return left > right;
            case BinaryOperatorKind::LessThanOrEqual: return left <= right;
            case BinaryOperatorKind::GreaterThanOrEqual: return left >= right;
            case BinaryOperatorKind::Equal: return left == right;
            case BinaryOperatorKind::NotEqual: return left != right;
            default: std::abort(); return false;
        }
    }
}