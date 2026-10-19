/**
 * @file test_table.cpp
 * @brief 通用文本表格工具类局部测试(自包含, 不引入第三方测试框架)
 *
 * 覆盖: 显示宽度计算、列宽自适应、对齐方式、单元格归一化与清空、标题与空表。
 */
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "table.h"

#ifdef _WIN32
#include <windows.h>
#endif

using os::Table;

namespace
{

    int g_passed = 0;
    int g_failed = 0;

    void expect(bool cond, const std::string &what)
    {
        if (cond)
        {
            ++g_passed;
            std::cout << "[PASS] " << what << "\n";
        }
        else
        {
            ++g_failed;
            std::cout << "[FAIL] " << what << "\n";
        }
    }

    bool contains(const std::string &text, const std::string &sub)
    {
        return text.find(sub) != std::string::npos;
    }

    // 1. 显示宽度: 全角字符按 2 列计
    void testDisplayWidth()
    {
        expect(os::displayWidth("abc") == 3, "ASCII 宽度按 1 列计");
        expect(os::displayWidth("进程") == 4, "中文宽度按 2 列计");
        expect(os::displayWidth("P0进程") == 6, "中英混合宽度正确");
        expect(os::displayWidth("") == 0, "空串宽度为 0");
    }

    // 2. 基本打印: 表头、数据行、按显示宽度对齐
    void testBasicPrint()
    {
        Table t({"进程", "Max"});
        t.addRow({"P0", "(7, 5)"});
        t.addRow({"P1", "(3, 2)"});

        std::ostringstream oss;
        t.print(oss);
        std::string out = oss.str();

        expect(contains(out, "进程"), "输出包含表头");
        expect(contains(out, "P0"), "输出包含数据行");
        expect(contains(out, "P0  "), "名称列按显示宽度补齐(\"进程\"宽 4, \"P0\"补 2 空格)");
        expect(t.rowCount() == 2 && t.columnCount() == 2, "行数与列数正确");
    }

    // 3. 右对齐: 短数字在左侧补空格
    void testRightAlign()
    {
        Table t({"数值"});
        t.setAlign(0, Table::Align::Right);
        t.addRow({"5"});
        t.addRow({"100"});

        std::ostringstream oss;
        t.print(oss);
        expect(contains(oss.str(), "  5"), "右对齐时短数字左侧补空格");
    }

    // 4. 单元格归一化: 不足补空、超出截断; 清空保留表头
    void testRowPaddingAndClear()
    {
        Table t({"A", "B", "C"});
        t.addRow({"1"});
        t.addRow({"1", "2", "3", "4"});
        expect(t.rowCount() == 2, "缺列/多列的行均被归一化");

        std::ostringstream oss;
        t.print(oss);
        expect(!contains(oss.str(), "4"), "超出列数的单元格被忽略");

        t.clearRows();
        expect(t.rowCount() == 0 && t.columnCount() == 3, "清空后保留表头");
    }

    // 5. 标题与空表
    void testTitleAndEmpty()
    {
        Table t({"列1"});
        t.setTitle("测试表");
        std::ostringstream oss;
        t.print(oss);
        expect(contains(oss.str(), "测试表"), "标题被打印");
        expect(contains(oss.str(), "列1"), "空表仍打印表头");
    }

} // namespace

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "===== 表格工具类局部测试 =====\n";
    testDisplayWidth();
    testBasicPrint();
    testRightAlign();
    testRowPaddingAndClear();
    testTitleAndEmpty();

    std::cout << "\n共 " << (g_passed + g_failed) << " 项断言, 通过 " << g_passed << " 项, 失败 "
              << g_failed << " 项\n";
    return g_failed == 0 ? 0 : 1;
}
