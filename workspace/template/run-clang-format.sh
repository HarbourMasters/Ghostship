if command -v clang-format-14 &> /dev/null; then
    CLANG_FORMAT="clang-format-14"
else
    CLANG_FORMAT="clang-format"
fi

find src -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | xargs -0 $CLANG_FORMAT -i --verbose
