#include <cstdio>
#include <limits>
#include <algorithm>
#include <wiz/ast/statement.h>
#include <wiz/utility/report.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/symbol_table.h>

namespace wiz {
    std::string SymbolTable::generateBlockName() {
        static unsigned int blockIndex = 0;
        char buffer[std::numeric_limits<unsigned int>::digits + 8];
        std::sprintf(buffer, "%%%X%%", blockIndex++);
        return std::string(buffer);
    }

    SymbolTable::SymbolTable()
    : parent(nullptr) {}

    SymbolTable::SymbolTable(
        SymbolTable* parent,
        StringView namespaceName)
    : parent(parent),
    namespaceName(namespaceName) {}

    SymbolTable::~SymbolTable() {}

    SymbolTable* SymbolTable::getParent() const {
        return parent;
    }

    std::string SymbolTable::getFullName() const {
        if (!parent) {
            return namespaceName.toString();
        } else {
            if (namespaceName.getLength() > 0 && namespaceName[0] == '%') {
                return namespaceName.toString();
            }
            std::string parentName = parent->getFullName();
            return (parentName.length() ? parentName + "." : "") + namespaceName.toString();
        }
    }

    void SymbolTable::printKeys(Report* report) const {
        for (const auto& item : definitions) {
            const auto& decl = item.second->declaration;
            report->log(item.first.toString() + ": " + decl->getDescription().toString() + " (declared: " + decl->location.toString() + ")");
        }
    }

    std::unordered_map<StringView, FwdUniquePtr<Definition>>& SymbolTable::getDefinitions() {
        return definitions;
    }

    const std::unordered_map<StringView, FwdUniquePtr<Definition>>& SymbolTable::getDefinitions() const {
        return definitions;
    }

    Definition* SymbolTable::addDefinition(Report* report, FwdUniquePtr<Definition> def) {
        const auto match = findLocalMemberDefinition(def->name);
        if (match != nullptr) {
            if (report) {
                report->error("redefinition of symbol `" + def->name.toString() + "`", def->declaration->location, ReportErrorFlags(ReportErrorFlagType::Continued));
                report->error("`" + def->name.toString() + "` was previously defined here, by " + match->declaration->getDescription().toString(), match->declaration->location);
            }
            return nullptr;
        } else {
            def->parentScope = this;

            auto result = def.get();
            definitions[def->name] = std::move(def);
            return result;
        }
    }

    bool SymbolTable::addImport(SymbolTable* import) {
        if (import == this) {
            return false;
        }
        if (std::find(imports.begin(), imports.end(), import) == imports.end()) {
            imports.push_back(import);
            return true;
        }
        return false;
    }

    bool SymbolTable::addRecursiveImport(SymbolTable* import) {
        if (addImport(import)) {
            for (const auto& it : definitions) {
                const auto name = it.first;
                const auto& def = it.second;
                if (auto ns = def->variant.tryGet<Definition::Namespace>()) {
                    if (const auto importedDef = import->findLocalMemberDefinition(name)) {
                        if (const auto importedNS = importedDef->variant.tryGet<Definition::Namespace>()) {
                            ns->environment->addRecursiveImport(importedNS->environment);
                        }
                    }
                }
            }

            return true;
        }
        return false;
    }

    Definition* SymbolTable::findLocalMemberDefinition(StringView name) const {
        const auto match = definitions.find(name);
        if (match != definitions.end()) {
            return match->second.get();
        }
        return nullptr;
    }

    std::vector<Definition*> SymbolTable::findImportedMemberDefinitions(StringView name) const {
        std::vector<Definition*> results;
        for (const auto import : imports) {
            if (const auto result = import->findLocalMemberDefinition(name)) {
                if (std::find(results.begin(), results.end(), result) == results.end()) {
                    results.push_back(result);
                }
            }
        }
        return results;
    }

    std::vector<Definition*> SymbolTable::findMemberDefinitions(StringView name) const {
        auto results = findImportedMemberDefinitions(name);
        if (const auto result = findLocalMemberDefinition(name)) {
            if (std::find(results.begin(), results.end(), result) == results.end()) {
                results.insert(results.begin(), result);
            }
        }
        return results;
    }

    std::vector<Definition*> SymbolTable::findUnqualifiedDefinitions(StringView name) const {
        auto results = findMemberDefinitions(name);
        if (results.size() == 0 && parent != nullptr) {
            return parent->findUnqualifiedDefinitions(name);
        }
        return results;
    }
}

