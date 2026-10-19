/**
 * @file table.h
 * @brief 通用终端文本表格工具类
 *
 * 用于在终端按列对齐地打印文本表格:
 *  - 自动计算每列显示宽度, 中文等全角字符按 2 列计
 *  - 支持可选标题、边框线、每列独立的对齐方式
 * 项目中打印表的需求(如资源分配表)统一使用本类实现。
 */
#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace os
{

    /**
     * 终端文本表格
     *
     * 典型用法:
     *   Table t({"进程", "Max", "Need"});
     *   t.setTitle("资源分配表");
     *   t.addRow({"P0", "(7, 5, 3)", "(7, 4, 3)"});
     *   t.print();
     */
    class Table
    {
    public:
        /// 列对齐方式
        enum class Align
        {
            Left, ///< 左对齐(默认)
            Right ///< 右对齐
        };

        /// 构造: headers 为表头, 列数由表头个数确定(至少 1 列)
        explicit Table(std::vector<std::string> headers);

        /// 设置表格标题(显示在表格上方, 空串表示无标题)
        void setTitle(std::string title);

        /// 追加一行数据; 单元格不足补空串, 超出忽略
        void addRow(std::vector<std::string> cells);

        /// 设置某列对齐方式(默认左对齐), 越界的列号被忽略
        void setAlign(std::size_t col, Align align);

        /// 清空全部数据行(保留表头与列设置)
        void clearRows();

        /// 当前数据行数(不含表头)
        std::size_t rowCount() const;

        /// 列数
        std::size_t columnCount() const;

        /// 将表格打印到 out(默认标准输出)
        void print(std::ostream &out = std::cout) const;

    private:
        std::string title_;                          ///< 表格标题
        std::vector<std::string> headers_;           ///< 表头
        std::vector<std::vector<std::string>> rows_; ///< 数据行
        std::vector<Align> aligns_;                  ///< 各列对齐方式
    };

    /// 计算字符串显示宽度: 中文等全角字符按 2 列计, 其余按 1 列
    std::size_t displayWidth(const std::string &text);

} // namespace os
