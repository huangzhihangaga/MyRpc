/**
 * @file MprpcController.cpp
 * @brief 实现MprpcController的接口
 */

#include "MprpcController.h"

MprpcController::MprpcController() {
    failed_ = false;
    errorText_ = "";
}

void MprpcController::Reset() {
    failed_ = false;
    errorText_.clear();
}

bool MprpcController::Failed() const {
    return failed_;
}

std::string MprpcController::ErrorText() const {
    return errorText_;
}

void MprpcController::SetFailed(const std::string &reason) {
    failed_ = true;
    errorText_ = reason;
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
