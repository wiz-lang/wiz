#include <wiz/ast/expression.h>
#include <wiz/utility/report.h>
#include <wiz/compiler/config.h>

namespace wiz {
    Config::Config() {}
    Config::~Config() {}

    bool Config::add(Report* report, StringView key, FwdUniquePtr<const Expression> value) {
        const auto match = items.find(key);
        if (match != items.end()) {
            report->error("duplicate config entry for `" + key.toString() + "`", value->location, ReportErrorFlags::Continued);
            report->error("previous entry for `" + key.toString() + "` appeared here", match->second->location);
            return false;
        } else {
            items[key] = std::move(value);
            return true;
        }
    }

    const Expression* Config::get(StringView key) const {
        const auto match = items.find(key);
        if (match != items.end()) {
            return match->second.get();
        } else {
            return nullptr;
        }
    }

    bool Config::has(StringView key) const {
        return get(key) != nullptr;
    }

    const Expression* Config::checkValue(Report* report, StringView key, bool required) const {
        if (const auto value = get(key)) {
            return value;
        } else if (required) {
            report->error("missing required config entry `" + key.toString() + "`", SourceLocation());
        }
        return nullptr;
    }

    Optional<std::pair<const Expression*, bool>> Config::checkBoolean(Report* report, StringView key, bool required) const {
        if (const auto value = checkValue(report, key, required)) {
            if (const auto lit = value->tryGet<Expression::BooleanLiteral>()) {
                return std::make_pair(value, lit->value);
            } else {
                report->error("config entry `" + key.toString() + "` must be a compile-time boolean literal", value->location);
            }
        }
        return {};
    }

    Optional<std::pair<const Expression*, Int128>> Config::checkInteger(Report* report, StringView key, bool required) const {
        if (const auto value = checkValue(report, key, required)) {
            if (const auto lit = value->tryGet<Expression::IntegerLiteral>()) {
                return std::make_pair(value, lit->value);
            } else {
                report->error("config entry `" + key.toString() + "` must be a compile-time integer literal", value->location);
            }
        }
        return {};
    }

    Optional<std::pair<const Expression*, StringView>> Config::checkString(Report* report, StringView key, bool required) const {
        if (const auto value = checkValue(report, key, required)) {
            if (const auto lit = value->tryGet<Expression::StringLiteral>()) {
                return std::make_pair(value, lit->value);
            } else {
                report->error("config entry `" + key.toString() + "` must be a compile-time string literal", value->location);
            }
        }
        return {};
    }

    Optional<std::pair<const Expression*, StringView>> Config::checkFixedString(Report* report, StringView key, std::size_t maxLength, bool required) const {
        if (const auto value = checkString(report, key, required)) {
            if (value->second.getLength() <= maxLength) {
                return value;
            } else {
                report->error("config entry `" + key.toString() + "` of " + value->second.toString()
                    + " is too long (must be at most " + std::to_string(maxLength)
                    + " characters, but got " + std::to_string(value->second.getLength()) + " characters)",
                    value->first->location);
            }
        }
        return {};
    }
}