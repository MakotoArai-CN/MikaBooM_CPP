#pragma once

// PID控制器 - 用于平滑稳定的负载调整
class PIDController {
private:
    double kp;  // 比例系数
    double ki;  // 积分系数
    double kd;  // 微分系数
    
    double lastError;
    double integral;
    double setpoint;
    
    double minOutput;
    double maxOutput;
    
    // 积分饱和限制
    double integralMax;
    
public:
    PIDController(double p, double i, double d) 
        : kp(p), ki(i), kd(d), lastError(0), integral(0), 
          setpoint(0), minOutput(0), maxOutput(100), integralMax(50) {
    }
    
    void SetTarget(double target) {
        setpoint = target;
    }
    
    void SetOutputLimits(double min, double max) {
        minOutput = min;
        maxOutput = max;
    }
    
    double Compute(double current, double deltaTime) {
        // 计算误差
        double error = setpoint - current;
        
        // 积分项（带抗饱和）
        integral += error * deltaTime;
        if (integral > integralMax) integral = integralMax;
        if (integral < -integralMax) integral = -integralMax;
        
        // 微分项
        double derivative = (error - lastError) / deltaTime;
        
        // PID输出
        double output = kp * error + ki * integral + kd * derivative;
        
        // 限制输出范围
        if (output < minOutput) output = minOutput;
        if (output > maxOutput) output = maxOutput;
        
        lastError = error;
        
        return output;
    }
    
    void Reset() {
        lastError = 0;
        integral = 0;
    }
    
    // 动态调整参数
    void Tune(double p, double i, double d) {
        kp = p;
        ki = i;
        kd = d;
    }
};