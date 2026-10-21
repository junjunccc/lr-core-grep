#!/bin/bash

cd "$(dirname "$0")/.." 
fail=0

t() {
    if [ "$2" = "$3" ]; then
        echo "  ok   $1"
    else
        echo "  FAIL $1   期望[$2]   实际[$3]"
        fail=1
    fi
}

t "基本匹配" "boot ok
boot ok again" "$(./lrg boot tests/sample.log)"
t "带行号 -n" "1:boot ok
4:boot ok again" "$(./lrg -n boot tests/sample.log)"
t "忽略大小写 -i" "3:ERROR: retry later
5:error: give up" "$(./lrg -in error tests/sample.log)"
t "多文件带前缀" "tests/sample.log:error: give up" \
                 "$(./lrg error tests/sample.log no-such-file 2>/dev/null)"
t "空 pattern 匹配全部" "$(cat tests/sample.log)" "$(./lrg "" tests/sample.log)"
t "-c 单个输入" "2" "$(./lrg -c -i error tests/sample.log)"
t "-c 多个文件" "tests/sample.log:2
tests/notes.txt:1" "$(./lrg -ci error tests/sample.log tests/notes.txt)"
t "stdin 不加前缀" "1:boot ok
4:boot ok again" "$(./lrg -n boot < tests/sample.log)"

t "有匹配 0" "0" "$(./lrg boot tests/sample.log >/dev/null 2>&1; echo $?)"
t "无匹配 1" "1" "$(./lrg zzz tests/sample.log >/dev/null 2>&1; echo $?)"
t "缺 pattern 2" "2" "$(./lrg >/dev/null 2>&1; echo $?)"
t "非法选项 2" "2" "$(./lrg -x a b >/dev/null 2>&1; echo $?)"
t "打不开 2" "2" "$(./lrg x no-such-file >/dev/null 2>&1; echo $?)"

t "空文件 0 行" "" "$(./lrg x tests/t_empty.txt)"
t "两个空行 2 字节" "2" "$(./lrg "" tests/t_blank.txt | wc -c)"
t "混合空行 3 行" "a

bbb" "$(./lrg "" tests/t_mixed.txt)"

t "1 MiB 不截断" "1048577" "$(./lrg x tests/t_longline.txt | wc -c)"
t "1 MiB 只有 1 行" "1" "$(./lrg -c x tests/t_longline.txt)"

echo
if [ "$fail" = 0 ]; then echo "全部通过"; else echo "有测试失败"; fi
exit "$fail"
