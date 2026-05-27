/**
 * @file MprpcController.h
 * @brief 定义MprpcController的接口
 */

#ifndef RPC_MPRPCCONTROLLER_H
#define RPC_MPRPCCONTROLLER_H

#include <iostream>
#include <string>
#include <google/protobuf/service.h>

/**
 * @brief 自定义的MprpcController，用于管理rpc的调用状态
 * @details 继承自google::protobuf::RpcController，实现其中的虚函数
 */
class MprpcController:public google::protobuf::RpcController {
public:
    /**
     * @brief MprpcController构造函数，初始化失败状态为false，错误信息为空字符串
     */
    MprpcController();

    /**
     * @brief 重置MprpcController到初始状态
     * @details 客户端调用
     */
    void Reset() override;

    /**
     * @brief 检查rpc调用是否失败
     * @return true 或 false
     * @retval true 表示调用失败，false表示调用成功
     * @details 客户端调用
     */
    bool Failed() const override;

    /**
     * @brief 获取rpc调用失败时的错误信息
     * @return 错误描述字符串，如果没失败返回空字符串
     * @details 客户端调用
     */
    std::string ErrorText() const override;

    /**
     * @brief 设置rpc调用失败的状态及错误原因
     * @param reason 错误描述信息
     * @details 服务端调用
     */
    void SetFailed(const std::string &reason) override;

    /**
     * @brief 尝试取消rpc调用
     * @details 客户端调用
     * @note 未实现
     */
    void StartCancel() override;

    /**
     * @brief 检查rpc调用是否被取消
     * @return true 或 false
     * @details 服务端调用
     * @note 未实现，始终返回false
     */
    bool IsCanceled() const override;

    /**
     * @brief 注册取消时的回调函数
     * @param callback 当rpc被取消时调用的回调函数
     * @details 服务端调用
     * @note 未实现
     */
    void NotifyOnCancel(google::protobuf::Closure *callback) override;

private:
    /// rpc方法执行中的失败标记
    bool failed_;

    /// rpc方法执行中的错误信息
    std::string errorText_;
};

#endif //RPC_MPRPCCONTROLLER_H
