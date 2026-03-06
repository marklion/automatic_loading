#include "../lib/pid_control_lib.h"
#include "../../public/lib/ad_rpc.h"

#include <iostream>
#include <random>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <limits>

class TerminalPlotter {
private:
    int height;  // 绘图区域高度（行数）
    int width;   // 绘图区域宽度（字符数）
    std::vector<std::vector<char>> canvas;

    // 每列的绘图符号
    const std::vector<char> symbols = {'*', '+', '#', '@', '.', '^'};

public:
    TerminalPlotter(int h = 20, int w = 80) : height(h), width(w) {
        canvas.resize(height, std::vector<char>(width, ' '));
    }

    // 清空画布
    void clear() {
        for (auto& row : canvas) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }

    // 绘制坐标轴
    void drawAxes() {
        // 绘制Y轴
        for (int i = 0; i < height; i++) {
            canvas[i][0] = '|';
        }

        // 绘制X轴
        for (int j = 0; j < width; j++) {
            canvas[height-1][j] = '-';
        }

        // 绘制原点
        canvas[height-1][0] = '+';
    }

    // 归一化数据到画布坐标
    double normalize(double value, double minVal, double maxVal, int range) {
        if (maxVal - minVal < 1e-10) return range / 2.0;
        return (value - minVal) / (maxVal - minVal) * range;
    }

    // 绘制一列数据
    void plotColumn(const std::vector<double>& data, int colIndex,
                    double minVal, double maxVal) {
        if (data.empty()) return;

        int n = data.size();
        int symbol = symbols[colIndex % symbols.size()];

        // 计算每个数据点在画布上的位置
        std::vector<std::pair<int, int>> points;

        for (int i = 0; i < n; i++) {
            // 归一化到画布坐标
            double x = normalize(i, 0, n-1, width-1);
            double y = normalize(data[i], minVal, maxVal, height-1);

            // 反转Y轴（因为终端从上到下）
            int canvasX = static_cast<int>(x + 0.5);
            int canvasY = height - 1 - static_cast<int>(y + 0.5);

            // 确保在画布范围内
            canvasX = std::max(0, std::min(canvasX, width-1));
            canvasY = std::max(0, std::min(canvasY, height-1));

            points.emplace_back(canvasX, canvasY);
        }

        // 绘制点和连线
        for (size_t i = 0; i < points.size(); i++) {
            int x = points[i].first;
            int y = points[i].second;

            // 绘制点
            canvas[y][x] = symbol;

            // 绘制连线（如果有点相邻）
            if (i > 0) {
                int prevX = points[i-1].first;
                int prevY = points[i-1].second;

                // 使用 Bresenham 算法绘制直线
                drawLine(prevX, prevY, x, y, symbol);
            }
        }
    }

    // Bresenham 直线绘制算法
    void drawLine(int x0, int y0, int x1, int y1, char symbol) {
        bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }

        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        int dx = x1 - x0;
        int dy = std::abs(y1 - y0);
        int error = dx / 2;
        int ystep = (y0 < y1) ? 1 : -1;
        int y = y0;

        for (int x = x0; x <= x1; x++) {
            if (steep) {
                if (y >= 0 && y < width && x >= 0 && x < height) {
                    canvas[x][y] = symbol;
                }
            } else {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    canvas[y][x] = symbol;
                }
            }

            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
        }
    }

    // 显示图例
    void drawLegend(const std::vector<std::string>& columnNames) {
        std::cout << "\n图例:\n";
        for (size_t i = 0; i < std::min(columnNames.size(), symbols.size()); i++) {
            std::cout << "  " << symbols[i] << " : " << columnNames[i] << "\n";
        }
        std::cout << "\n";
    }

    // 打印画布
    void display() {
        std::cout << "\n";
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                std::cout << canvas[i][j];
            }
            std::cout << "\n";
        }
    }

    // 主绘图函数
    void plotMultiColumns(const std::vector<std::vector<double>>& data,
                         const std::vector<std::string>& columnNames = {}) {
        if (data.empty()) {
            std::cout << "错误: 数据为空!\n";
            return;
        }

        int n = data[0].size();  // 数据点数
        int cols = data.size();  // 列数

        // 检查所有列长度是否一致
        for (const auto& col : data) {
            if (col.size() != n) {
                std::cout << "错误: 所有列必须有相同的数据点数!\n";
                return;
            }
        }

        // 找出全局最小值和最大值
        double globalMin = std::numeric_limits<double>::max();
        double globalMax = std::numeric_limits<double>::lowest();

        for (const auto& col : data) {
            auto minmax = std::minmax_element(col.begin(), col.end());
            globalMin = std::min(globalMin, *minmax.first);
            globalMax = std::max(globalMax, *minmax.second);
        }

        // 添加一些边距
        double range = globalMax - globalMin;
        globalMin -= range * 0.1;
        globalMax += range * 0.1;

        // 清空并绘制坐标轴
        clear();
        drawAxes();

        // 绘制每一列
        for (int i = 0; i < cols; i++) {
            plotColumn(data[i], i, globalMin, globalMax);
        }

        // 显示
        display();

        // 显示图例
        if (!columnNames.empty()) {
            drawLegend(columnNames);
        }

        // 显示统计信息
        std::cout << "统计信息:\n";
        std::cout << "数据点数: " << n << ", 列数: " << cols << "\n";
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "数值范围: [" << globalMin + range * 0.1 << ", "
                  << globalMax - range * 0.1 << "]\n";
    }
};


class ItObject
{
    double m_self = -2.8;
    double m_delta_time = 0.2 * 0.08;
    int m_state = 0;
public:
    ItObject(double _init_value, double _delta_time) : m_self(_init_value), m_delta_time(_delta_time) {}
    double update(double _input)
    {
        auto ret = m_self + _input * m_delta_time;
        if (ret > m_self)
        {
            m_state = 1;
        }
        else if (ret < m_self)
        {
            m_state = -1;
        }
        else
        {
            m_state = 0;
        }
        m_self = ret;
        return ret;
    }
    double cur_value()
    {
        return m_self;
    }
    int cur_state()
    {
        return m_state;
    }
};
class SegFunction
{
    std::map<int, double> m_output_map;
    double m_min = 0;
    double m_cur_value = 0;
public:
    SegFunction(double _min) : m_min(_min) {}
    void add_seg(int _input, double _output)
    {
        m_output_map[_input] = _output;
    }
    double update(double _input)
    {
        double target = m_min;
        for (const auto &seg : m_output_map)
        {
            if (_input >= seg.first)
            {
                target = seg.second;
            }
            else
            {
                break;
            }
        }
        m_cur_value = target;
        return target;
    }
    double cur_value()
    {
        return m_cur_value;
    }
};

class ButtonSim{
    double m_period = 0;
    double m_work_state = 0;
    double m_state_count = 0;
    double m_output = 0;
public:
    ButtonSim(double period) : m_period(period) {}
    void set_work_state(double _input) {
        auto lack = _input - m_work_state;
        if (lack != 0)
        {
            m_state_count += lack;
        }
        m_work_state = _input;
    }
    double loop()
    {
        double ret = 0;

        if (m_state_count > m_period)
        {
            m_state_count -= m_period;
            ret = 1;
        }
        else if (m_state_count < -m_period)
        {
            m_state_count+= m_period;
            ret = -1;
        }
        m_output = ret;
        return ret;
    }
    double cur_output()
    {
        return m_output;
    }
    double cur_state()
    {
        return m_work_state;
    }
};

int main(int argc, char const *argv[])
{
    auto Skp = atof(argv[1]);
    auto Ski = atof(argv[2]);
    auto Skd = atof(argv[3]);

    ItObject controlled_object(0, 0.2 * 0.08);
    ItObject afetr_button_press(0, 0.08*0.24);
    ButtonSim button_object(0.08);
    SegFunction sf(0);
    sf.add_seg(1, 1);
    sf.add_seg(2, 2);
    sf.add_seg(3, 3);
    pid_control::DiscretePID core_pid(Skp, Ski, Skd, 0, 10, 0.08);

    std::vector<double> expect_array;
    expect_array.push_back(0);
    expect_array.push_back(0);
    for (int i = 0; i < 400; i++)
    {
        expect_array.push_back(1.2);
    }
    std::vector<std::vector<double>> data(4);
    for (auto spec : expect_array)
    {
        auto expect_output = spec;
        auto measured_value = controlled_object.cur_value();
        auto pid_output = core_pid.execute_continuous(measured_value, expect_output);
        auto seg_output = sf.update(pid_output);
        button_object.set_work_state(seg_output);
        auto button_output = button_object.loop();
        auto press_output = afetr_button_press.update(button_object.cur_output());
        auto final_output = controlled_object.update(press_output);
        data[0].push_back(expect_output);
        data[1].push_back(button_output);
        data[2].push_back(press_output);
        data[3].push_back(final_output);
        // data[4].push_back(seg_output);
        // data[5].push_back(pid_output);
    }
    TerminalPlotter tp(30, 120);
    tp.plotMultiColumns(data, {"期望值", "按钮状态", "速度", "实际值", "分段输出", "pid输出"});
    return 0;
}
