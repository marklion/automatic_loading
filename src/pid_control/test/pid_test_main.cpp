#include "../lib/pid_control_lib.h"
#include "../../public/lib/ad_rpc.h"

#include <iostream>
#include <random>
#include <chrono>

class ItObject{
    double m_self = -2.8;
    double m_delta_time = 0.2;
public:
    ItObject(double _delta_time):m_delta_time(_delta_time){}
    double update(double _input){
        auto ret = m_self + _input * m_delta_time;
        m_self = ret;
        return ret;
    }
    double getOutput(){
        return m_self;
    }
};

class ControlledSystem {
private:
    double output;      // 当前输出
    double k;           // 比例系数
    double dt;          // 时间步长（用于数值积分）

public:
    // 构造函数
    ControlledSystem(double initialOutput = 1.0, double gain = 1.0, double timeStep = 0.01)
        : output(initialOutput), k(gain), dt(timeStep) {
        if (initialOutput <= 0) {
            output = 0.0001;  // 避免初始值为0时系统不工作
        }
    }

    // 设置输入并更新输出
    void setInput(int u) {
        // 只接受-1, 0, 1三种输入
        if (u != -1 && u != 0 && u != 1) {
            return;
        }

        // 使用欧拉法数值积分
        double derivative = k * output * u;
        output += derivative * dt;

        // 防止输出变为0或负数（根据实际需求调整）
        if (output < 1e-10) {
            output = 1e-10;
        }
    }

    // 获取当前输出
    double getOutput() const {
        return output;
    }

    // 设置参数
    void setParameters(double gain, double timeStep) {
        k = gain;
        dt = timeStep;
    }

    // 重置系统
    void reset(double initialOutput = 1.0) {
        output = initialOutput;
        if (output <= 0) {
            output = 1.0;
        }
    }

    // 析构函数
    ~ControlledSystem() = default;
};

#include <cmath>
#include <stdexcept>

class ControlledObject {
private:
    double output;   // 当前输出值
    double k1;        // 比例系数，控制变化速度
    double k2;
    double dt;       // 时间步长（离散模拟的时间间隔）
    const double EPS = 1e-9;  // 防止除零和过小的阈值

public:
    // 构造函数：初始化输出值、比例系数和时间步长
    ControlledObject(double initialOutput = 1.0, double coefficient = 1.0, double timeStep = 0.01)
        : k1(coefficient), k2(1), dt(timeStep) {
        if (initialOutput <= 0) {
            throw std::invalid_argument("Initial output must be positive.");
        }
        if (coefficient <= 0 || timeStep <= 0) {
            throw std::invalid_argument("Coefficient and time step must be positive.");
        }
        output = initialOutput;
    }

    // 根据输入更新输出值
    void update(int input) {
        if (input == 0) {
            // 输入为0，输出保持不动
            return;
        } else if (input == 1) {
            // 输入为1，输出变大，变化速度与当前输出成正比
            output += k1 * output * dt;
        } else if (input == -1) {
            // 输入为-1，输出变小，变化速度与当前输出成反比
            // 当输出非常小时，避免除零和过大变化
            if (output > EPS) {
                output -= k2 / output * dt;
                // 防止输出变为负数或过小
                if (output < EPS) {
                    output = EPS;
                }
            }
        } else {
            throw std::invalid_argument("Input must be -1, 0, or 1.");
        }
    }

    // 获取当前输出值
    double getOutput() const {
        return output;
    }

    // 设置时间步长
    void setTimeStep(double timeStep) {
        if (timeStep <= 0) {
            throw std::invalid_argument("Time step must be positive.");
        }
        dt = timeStep;
    }
};

int main(int argc, char const *argv[])
{
    auto Skp = atof(argv[1]);
    auto Ski = atof(argv[2]);
    auto Skd = atof(argv[3]);
    auto SDZ = atof(argv[4]);
    auto Ckp = atof(argv[5]);
    auto Cki = atof(argv[6]);
    auto Ckd = atof(argv[7]);
    ControlledObject co(0.000001, 10, 0.08);
    ItObject io(0.08);
    pid_control::DiscretePID pc_inner(Skp, Ski, Skd, SDZ, 10, 0.08);
    pid_control::DiscretePID pc_outer(Ckp, Cki, Ckd, 0, 10, 0.08);

    std::vector<double> expect_array;
    for (int i = 0; i < 200; i++)
    {
        expect_array.push_back(-0.6);
    }
    for (auto spec : expect_array)
    {
        auto expect_output = spec;
        auto outer_output = pc_outer.execute_continuous(io.getOutput(), expect_output);
        auto control_signal = pc_inner.execute(co.getOutput(), outer_output);
        co.update(control_signal);

        auto co_output = co.getOutput();
        auto output = io.update(co_output);
        std::cout << expect_output <<","<<outer_output << ","<<control_signal<<","<<  co_output <<","<< output << std::endl;
    }
    return 0;
}
