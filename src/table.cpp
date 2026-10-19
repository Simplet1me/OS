/**
 * @file table.cpp
 * @brief 通用终端文本表格工具类实现
 */
#include "table.h"

#include <algorithm>
#include <cstdint>

namespace os
{

    namespace
    {

        /// 在左侧补空格, 使显示宽度达到 width
        std::string padLeft(const std::string &text, std::size_t width)
        {
            std::size_t w = displayWidth(text);
            return w >= width ? text : std::string(width - w, ' ') + text;
        }

        /// 在右侧补空格, 使显示宽度达到 width
        std::string padRight(const std::string &text, std::size_t width)
        {
            std::size_t w = displayWidth(text);
            return w >= width ? text : text + std::string(width - w, ' ');
        }

    } // namespace

    std::size_t displayWidth(const std::string &text)
    {
        std::size_t width = 0;
        std::size_t i = 0;
        while (i < text.size())
        {
            unsigned char c = static_cast<unsigned char>(text[i]);
            std::uint32_t cp = 0;
            int len = 1;
            if (c < 0x80)
            {
                cp = c;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                cp = c & 0x1F;
                len = 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                cp = c & 0x0F;
                len = 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                cp = c & 0x07;
                len = 4;
            }
            else
            {
                // 非法 UTF-8 字节, 按 1 列宽处理
                ++i;
                ++width;
                continue;
            }
            if (i + len > text.size())
            {
                // 截断的多字节序列, 按 1 列宽处理
                ++i;
                ++width;
                continue;
            }
            for (int k = 1; k < len; ++k)
            {
                cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
            }
            // 中文、日文假名、韩文及全角符号等按 2 列宽计
            bool wide = (cp >= 0x1100 && cp <= 0x115F) || cp >= 0x2E80;
            width += wide ? 2 : 1;
            i += len;
        }
        return width;
    }

    Table::Table(std::vector<std::string> headers)
        : headers_(std::move(headers))
    {
        if (headers_.empty())
        {
            headers_.push_back(""); // 至少保证 1 列
        }
        aligns_.assign(headers_.size(), Align::Left);
    }

    void Table::setTitle(std::string title)
    {
        title_ = std::move(title);
    }

    void Table::addRow(std::vector<std::string> cells)
    {
        cells.resize(headers_.size()); // 不足补空串, 超出忽略
        rows_.push_back(std::move(cells));
    }

    void Table::setAlign(std::size_t col, Align align)
    {
        if (col < aligns_.size())
        {
            aligns_[col] = align;
        }
    }

    void Table::clearRows()
    {
        rows_.clear();
    }

    std::size_t Table::rowCount() const
    {
        return rows_.size();
    }

    std::size_t Table::columnCount() const
    {
        return headers_.size();
    }

    void Table::print(std::ostream &out) const
    {
        const std::size_t cols = headers_.size();
        const std::size_t gutter = 2; // 列间距

        // 计算每列显示宽度
        std::vector<std::size_t> widths(cols, 0);
        for (std::size_t c = 0; c < cols; ++c)
        {
            widths[c] = displayWidth(headers_[c]);
        }
        for (const std::vector<std::string> &row : rows_)
        {
            for (std::size_t c = 0; c < cols; ++c)
            {
                widths[c] = std::max(widths[c], displayWidth(row[c]));
            }
        }

        // 边框总宽 = 各列宽度之和 + 列间距
        std::size_t borderW = 0;
        for (std::size_t c = 0; c < cols; ++c)
        {
            borderW += widths[c];
            if (c > 0)
            {
                borderW += gutter;
            }
        }
        auto border = [borderW](char ch)
        {
            return std::string(borderW, ch);
        };

        // 标题与上边框
        out << "\n";
        if (!title_.empty())
        {
            out << border('=') << "\n"
                << title_ << "\n"
                << border('-') << "\n";
        }
        else
        {
            out << border('-') << "\n";
        }

        // 表头(左对齐)
        for (std::size_t c = 0; c < cols; ++c)
        {
            if (c > 0)
            {
                out << std::string(gutter, ' ');
            }
            out << padRight(headers_[c], widths[c]);
        }
        out << "\n"
            << border('-') << "\n";

        // 数据行(按各列设置对齐)
        for (const std::vector<std::string> &row : rows_)
        {
            for (std::size_t c = 0; c < cols; ++c)
            {
                if (c > 0)
                {
                    out << std::string(gutter, ' ');
                }
                out << (aligns_[c] == Align::Right ? padLeft(row[c], widths[c])
                                                   : padRight(row[c], widths[c]));
            }
            out << "\n";
        }
        out << border('-') << "\n";
    }

} // namespace os
