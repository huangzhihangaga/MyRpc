/**
 * @file mprpccontroller.cpp
 * @brief 实现MprpcController的接口
 */

#include "mprpccontroller.h"

MprpcController::MprpcController() {
    m_failed = false;
    m_errText = "";
}

void MprpcController::Reset() {
    m_failed = false;
    m_errText = "";
}

bool MprpcController::Failed() const {
    return m_failed;
}

std::string MprpcController::ErrorText() const {
    return m_errText;
}

void MprpcController::SetFailed(const std::string &reason) {
    m_failed = true;
    m_errText = reason;
}

// 未实现
void MprpcController::StartCancel() {

}

// 未实现
bool MprpcController::IsCanceled() const {
    return false;
}

// 未实现
void MprpcController::NotifyOnCancel(google::protobuf::Closure *callback) {

}
