#pragma once

namespace dwr {

/// 库内统一错误状态（ErrorStatus 类枚举，成员 e 前缀）。
enum class ErrorStatus : int {
    eOk = 0,              /// 成功
    eOpenFail,            /// 无法打开文件
    eBadVersion,          /// 不支持的 DWG 版本（非 AC1018 / AC1021）
    eDecryptFail,         /// 文件头 magic 校验失败（解密错位）
    eSectionNotFound,     /// 节地图中找不到指定节
    eDecompressFail,      /// LZ77 解压越界/非法 opcode
    eReadOverrun,         /// 位流/字节流越界
    eBadDataPage,         /// 数据页掩码后类型签名不符
    eUnknownType,         /// 未注册的对象类型码
};

}  // namespace dwr
