#include "Scripts.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <set>
#include <unordered_map>

#include <libultraship.h>
#include <spdlog/spdlog.h>

#include "ScriptCommands.h"
#include "ScriptSymbolTable.h"
#include "port/ShipCompat.h"

namespace Scripts {

namespace {

struct SymbolTables {
    std::unordered_map<std::string, const void*> pointers;
    std::unordered_map<std::string, int64_t> constants;
};

std::recursive_mutex sSymbolMutex;

SymbolTables& Tables() {
    static SymbolTables* sTables = [] {
        auto* tables = new SymbolTables();
        for (size_t i = 0; i < gScriptPointerSymbolCount; i++) {
            tables->pointers.emplace(gScriptPointerSymbols[i].name, gScriptPointerSymbols[i].value);
        }
        for (size_t i = 0; i < gScriptConstantSymbolCount; i++) {
            tables->constants.emplace(gScriptConstantSymbols[i].name, gScriptConstantSymbols[i].value);
        }
        tables->constants.emplace("NULL", 0);
        tables->constants.emplace("TRUE", 1);
        tables->constants.emplace("FALSE", 0);
        return tables;
    }();
    return *sTables;
}

const void* FindArchivedBehavior(const std::string& name) {
    static thread_local std::set<std::string> sLoading;
    const std::string path = "scripts/behaviors/" + name;
    auto archives = ShipCompat::GetResourceManager()->GetArchiveManager();
    if (!archives->HasFile(path) || sLoading.contains(name)) {
        return nullptr;
    }
    sLoading.insert(name);
    const void* data = ResourceGetDataByName(path.c_str());
    sLoading.erase(name);
    return data;
}

} // namespace

const void* FindPointer(std::string_view name) {
    std::lock_guard lock(sSymbolMutex);
    auto& pointers = Tables().pointers;
    const std::string key(name);
    if (auto it = pointers.find(key); it != pointers.end()) {
        return it->second;
    }
    if (const void* behavior = FindArchivedBehavior(key)) {
        pointers.emplace(key, behavior);
        return behavior;
    }
    return nullptr;
}

std::optional<int64_t> FindConstant(std::string_view name) {
    std::lock_guard lock(sSymbolMutex);
    auto& constants = Tables().constants;
    if (auto it = constants.find(std::string(name)); it != constants.end()) {
        return it->second;
    }
    return std::nullopt;
}

void RegisterPointer(std::string name, const void* value) {
    std::lock_guard lock(sSymbolMutex);
    Tables().pointers[std::move(name)] = value;
}

void RegisterConstant(std::string name, int64_t value) {
    std::lock_guard lock(sSymbolMutex);
    Tables().constants[std::move(name)] = value;
}

namespace {

struct Token {
    enum class Type { End, Ident, Number, String, Punct };
    Type type = Type::End;
    std::string text;
    int64_t number = 0;
    uint32_t line = 1;
};

class Lexer {
  public:
    explicit Lexer(std::string_view text) : mText(text) {
    }

    std::optional<Token> Next(std::string& error) {
        SkipSpace();
        Token tok;
        tok.line = mLine;
        if (mPos >= mText.size()) {
            return tok;
        }
        const char c = mText[mPos];
        if (std::isalpha((unsigned char)c) || c == '_') {
            size_t start = mPos;
            while (mPos < mText.size() && (std::isalnum((unsigned char)mText[mPos]) || mText[mPos] == '_')) {
                mPos++;
            }
            tok.type = Token::Type::Ident;
            tok.text = std::string(mText.substr(start, mPos - start));
            return tok;
        }
        if (std::isdigit((unsigned char)c) || (c == '.' && mPos + 1 < mText.size() && std::isdigit((unsigned char)mText[mPos + 1]))) {
            size_t start = mPos;
            while (mPos < mText.size() && (std::isalnum((unsigned char)mText[mPos]) || mText[mPos] == '.')) {
                mPos++;
            }
            std::string lit(mText.substr(start, mPos - start));
            tok.type = Token::Type::Number;
            tok.text = lit;
            const bool hex = lit.size() > 1 && lit[0] == '0' && (lit[1] == 'x' || lit[1] == 'X');
            if (!hex && (lit.find('.') != std::string::npos || lit.find_first_of("eE") != std::string::npos)) {
                tok.number = (int64_t)std::trunc(std::strtod(lit.c_str(), nullptr));
                return tok;
            }
            while (!lit.empty() && std::strchr("uUlLfF", lit.back()) != nullptr && !(hex && std::isxdigit((unsigned char)lit.back()))) {
                lit.pop_back();
            }
            char* end = nullptr;
            tok.number = (int64_t)std::strtoull(lit.c_str(), &end, 0);
            if (end == nullptr || *end != '\0') {
                error = "line " + std::to_string(mLine) + ": bad number '" + tok.text + "'";
                return std::nullopt;
            }
            return tok;
        }
        if (c == '"') {
            mPos++;
            std::string out;
            while (mPos < mText.size() && mText[mPos] != '"') {
                if (mText[mPos] == '\\' && mPos + 1 < mText.size()) {
                    mPos++;
                }
                out += mText[mPos++];
            }
            if (mPos >= mText.size()) {
                error = "line " + std::to_string(tok.line) + ": unterminated string";
                return std::nullopt;
            }
            mPos++;
            tok.type = Token::Type::String;
            tok.text = out;
            return tok;
        }
        static const char* kTwoChar[] = { "<<", ">>" };
        for (const char* op : kTwoChar) {
            if (mText.substr(mPos, 2) == op) {
                mPos += 2;
                tok.type = Token::Type::Punct;
                tok.text = op;
                return tok;
            }
        }
        mPos++;
        tok.type = Token::Type::Punct;
        tok.text = std::string(1, c);
        return tok;
    }

  private:
    void SkipSpace() {
        bool lineStart = mPos == 0 || mText[mPos - 1] == '\n';
        while (mPos < mText.size()) {
            const char c = mText[mPos];
            if (c == '\n') {
                mLine++;
                mPos++;
                lineStart = true;
            } else if (std::isspace((unsigned char)c)) {
                mPos++;
            } else if (c == '#' && lineStart) {
                // Preprocessor lines, with their continuations.
                while (mPos < mText.size() && mText[mPos] != '\n') {
                    if (mText[mPos] == '\\' && mPos + 1 < mText.size() && mText[mPos + 1] == '\n') {
                        mPos++;
                        mLine++;
                    }
                    mPos++;
                }
            } else if (mText.substr(mPos, 2) == "//") {
                while (mPos < mText.size() && mText[mPos] != '\n') {
                    mPos++;
                }
            } else if (mText.substr(mPos, 2) == "/*") {
                size_t end = mText.find("*/", mPos + 2);
                end = end == std::string_view::npos ? mText.size() : end + 2;
                for (size_t i = mPos; i < end; i++) {
                    mLine += mText[i] == '\n';
                }
                mPos = end;
            } else {
                return;
            }
        }
    }

    std::string_view mText;
    size_t mPos = 0;
    uint32_t mLine = 1;
};

class Parser {
  public:
    Parser(Kind kind, std::string_view text) : mKind(kind), mLexer(text) {
    }

    std::optional<Program> Run(std::string& error) {
        Program program;
        program.kind = mKind;
        if (!Advance()) {
            return Fail(error);
        }
        const char* arrayType = mKind == Kind::Level ? "LevelScript" : "BehaviorScript";
        while (mTok.type != Token::Type::End) {
            if (IsIdent("extern")) {
                while (mTok.type != Token::Type::End && !IsPunct(";")) {
                    if (!Advance()) {
                        return Fail(error);
                    }
                }
                if (!Advance()) {
                    return Fail(error);
                }
                continue;
            }
            while (IsIdent("static") || IsIdent("const")) {
                if (!Advance()) {
                    return Fail(error);
                }
            }
            if (!IsIdent(arrayType)) {
                mError = "expected a " + std::string(arrayType) + " array, found '" + mTok.text + "'";
                return Fail(error);
            }
            Array array;
            if (!Advance() || !IsIdent()) {
                mError = "expected the array's name";
                return Fail(error);
            }
            array.name = mTok.text;
            if (!Advance() || !Expect("[") || !Expect("]") || !Expect("=") || !Expect("{")) {
                return Fail(error);
            }
            while (!IsPunct("}")) {
                Command cmd;
                if (!ParseCommand(cmd)) {
                    return Fail(error);
                }
                array.commands.push_back(std::move(cmd));
                if (IsPunct(",")) {
                    if (!Advance()) {
                        return Fail(error);
                    }
                } else if (!IsPunct("}")) {
                    mError = "expected ',' or '}' after " + array.commands.back().name;
                    return Fail(error);
                }
            }
            if (!Advance() || !Expect(";")) {
                return Fail(error);
            }
            program.arrays.push_back(std::move(array));
        }
        return program;
    }

  private:
    std::optional<Program> Fail(std::string& error) {
        error = "line " + std::to_string(mTok.line) + ": " + mError;
        return std::nullopt;
    }

    bool Advance() {
        auto tok = mLexer.Next(mError);
        if (!tok) {
            return false;
        }
        mTok = std::move(*tok);
        return true;
    }

    bool IsIdent(const char* text = nullptr) const {
        return mTok.type == Token::Type::Ident && (text == nullptr || mTok.text == text);
    }

    bool IsPunct(const char* text) const {
        return mTok.type == Token::Type::Punct && mTok.text == text;
    }

    bool Expect(const char* text) {
        if (!IsPunct(text)) {
            mError = std::string("expected '") + text + "', found '" + mTok.text + "'";
            return false;
        }
        return Advance();
    }

    bool ParseCommand(Command& cmd) {
        if (!IsIdent()) {
            mError = "expected a command, found '" + mTok.text + "'";
            return false;
        }
        cmd.name = mTok.text;
        cmd.line = mTok.line;
        if (!Advance() || !Expect("(")) {
            return false;
        }
        while (!IsPunct(")")) {
            Value value;
            if (!ParseArg(value)) {
                return false;
            }
            cmd.args.push_back(std::move(value));
            if (IsPunct(",")) {
                if (!Advance()) {
                    return false;
                }
            } else if (!IsPunct(")")) {
                mError = "expected ',' or ')' in " + cmd.name;
                return false;
            }
        }
        return Advance();
    }

    bool ParseArg(Value& value) {
        if (mTok.type == Token::Type::String) {
            value.type = Value::Type::Path;
            value.text = mTok.text;
            return Advance();
        }
        if (IsPunct("&")) {
            if (!Advance() || !IsIdent()) {
                mError = "expected a name after '&'";
                return false;
            }
            value.type = Value::Type::Symbol;
            value.text = mTok.text;
            return Advance();
        }
        if (IsIdent() && !FindConstant(mTok.text).has_value()) {
            value.type = Value::Type::Symbol;
            value.text = mTok.text;
            return Advance();
        }
        value.type = Value::Type::Number;
        return ParseExpr(0, value.number);
    }

    static int Precedence(const std::string& op) {
        if (op == "|") return 1;
        if (op == "^") return 2;
        if (op == "&") return 3;
        if (op == "<<" || op == ">>") return 4;
        if (op == "+" || op == "-") return 5;
        if (op == "*" || op == "/" || op == "%") return 6;
        return -1;
    }

    bool ParseExpr(int minPrec, int64_t& out) {
        if (!ParseUnary(out)) {
            return false;
        }
        while (mTok.type == Token::Type::Punct) {
            const std::string op = mTok.text;
            const int prec = Precedence(op);
            if (prec < minPrec || prec < 0) {
                break;
            }
            int64_t rhs = 0;
            if (!Advance() || !ParseExpr(prec + 1, rhs)) {
                return false;
            }
            if ((op == "/" || op == "%") && rhs == 0) {
                mError = "division by zero";
                return false;
            }
            switch (op[0]) {
                case '|': out |= rhs; break;
                case '^': out ^= rhs; break;
                case '&': out &= rhs; break;
                case '<': out <<= rhs; break;
                case '>': out >>= rhs; break;
                case '+': out += rhs; break;
                case '-': out -= rhs; break;
                case '*': out *= rhs; break;
                case '/': out /= rhs; break;
                case '%': out %= rhs; break;
            }
        }
        return true;
    }

    bool ParseUnary(int64_t& out) {
        if (IsPunct("-") || IsPunct("~") || IsPunct("+") || IsPunct("!")) {
            const char op = mTok.text[0];
            if (!Advance() || !ParseUnary(out)) {
                return false;
            }
            out = op == '-' ? -out : op == '~' ? ~out : op == '!' ? !out : out;
            return true;
        }
        if (IsPunct("(")) {
            if (!Advance()) {
                return false;
            }
            static const std::set<std::string> kCasts = { "u8", "s8", "u16", "s16", "u32", "s32", "u64", "s64",
                                                          "f32", "int", "uintptr_t", "intptr_t" };
            if (IsIdent() && kCasts.contains(mTok.text)) {
                if (!Advance() || !Expect(")")) {
                    return false;
                }
                return ParseUnary(out);
            }
            return ParseExpr(0, out) && Expect(")");
        }
        if (mTok.type == Token::Type::Number) {
            out = mTok.number;
            return Advance();
        }
        if (IsIdent()) {
            auto value = FindConstant(mTok.text);
            if (!value) {
                mError = "unknown constant '" + mTok.text + "'";
                return false;
            }
            out = *value;
            return Advance();
        }
        mError = "unexpected '" + mTok.text + "'";
        return false;
    }

    Kind mKind;
    Lexer mLexer;
    Token mTok;
    std::string mError;
};

} // namespace

std::optional<Program> ParseText(Kind kind, std::string_view text, std::string& error) {
    return Parser(kind, text).Run(error);
}

bool Build(const Program& program, Built& out, std::string& error) {
    const bool level = program.kind == Kind::Level;
    auto argCount = level ? ScriptCommands_LevelArgCount : ScriptCommands_BehaviorArgCount;
    auto emit = level ? ScriptCommands_EmitLevel : ScriptCommands_EmitBehavior;

    std::unordered_map<std::string, size_t> locals;
    for (size_t i = 0; i < program.arrays.size(); i++) {
        locals.emplace(program.arrays[i].name, i);
    }

    out.arrays.assign(program.arrays.size(), {});
    out.paths.clear();

    for (int pass = 0; pass < 2; pass++) {
        for (size_t a = 0; a < program.arrays.size(); a++) {
            const Array& array = program.arrays[a];
            std::vector<uintptr_t> words;
            words.reserve(pass == 0 ? array.commands.size() * 4 : out.arrays[a].size());
            for (const Command& cmd : array.commands) {
                const auto where = [&] { return array.name + " line " + std::to_string(cmd.line) + ": " + cmd.name; };
                const int expected = argCount(cmd.name.c_str());
                if (expected < 0) {
                    error = where() + " isn't a " + (level ? "level" : "behavior") + " script command";
                    return false;
                }
                if ((size_t)expected != cmd.args.size()) {
                    error = where() + " takes " + std::to_string(expected) + " arguments, got " +
                            std::to_string(cmd.args.size());
                    return false;
                }
                uintptr_t args[SCRIPT_COMMAND_MAX_WORDS] = {};
                for (size_t i = 0; i < cmd.args.size(); i++) {
                    const Value& value = cmd.args[i];
                    switch (value.type) {
                        case Value::Type::Number:
                            args[i] = (uintptr_t)value.number;
                            break;
                        case Value::Type::Path:
                            if (pass == 1) {
                                out.paths.push_back("__OTR__" + value.text);
                                args[i] = (uintptr_t)out.paths.back().c_str();
                            }
                            break;
                        case Value::Type::Symbol:
                            if (pass == 0) {
                                break;
                            }
                            if (auto it = locals.find(value.text); it != locals.end()) {
                                args[i] = (uintptr_t)out.arrays[it->second].data();
                            } else if (const void* ptr = FindPointer(value.text)) {
                                args[i] = (uintptr_t)ptr;
                            } else {
                                error = where() + ": nothing is called '" + value.text + "'";
                                return false;
                            }
                            break;
                    }
                }
                uintptr_t buf[SCRIPT_COMMAND_MAX_WORDS];
                const int n = emit(cmd.name.c_str(), args, buf);
                words.insert(words.end(), buf, buf + n);
            }
            if (pass == 0) {
                // Written in place after this so pointers into it stay valid.
                out.arrays[a].resize(words.size());
            } else {
                std::copy(words.begin(), words.end(), out.arrays[a].begin());
            }
        }
    }

    out.entry = 0;
    if (!program.entry.empty()) {
        auto it = locals.find(program.entry);
        if (it == locals.end()) {
            error = "the entry array '" + program.entry + "' doesn't exist";
            return false;
        }
        out.entry = it->second;
    } else if (level) {
        for (size_t i = 0; i < program.arrays.size(); i++) {
            const std::string& name = program.arrays[i].name;
            if (name.size() >= 6 && name.compare(name.size() - 6, 6, "_entry") == 0) {
                out.entry = i;
            }
        }
    }
    return true;
}

} // namespace Scripts

extern "C" void Scripts_RegisterPointer(const char* name, const void* value) {
    Scripts::RegisterPointer(name, value);
}

extern "C" void Scripts_RegisterConstant(const char* name, int64_t value) {
    Scripts::RegisterConstant(name, value);
}
