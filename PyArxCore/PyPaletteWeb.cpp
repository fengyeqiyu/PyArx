//-----------------------------------------------------------------------------
// PyPaletteWeb.cpp — 侧栏页的 Web 宿主实现（依赖 MFCWebView）。
//
// 除了把页面渲染出来，这里还架了一条「网页 → CAD」的命令桥：
// 页面 postMessage({cmd:'cad.line', ...}) → 本文件 → ObjectARX 直接建实体，
// 回执再以 cad.result 推回页面。用来证明 web 按钮真的能调 CAD API。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "PyPaletteWeb.h"
#include "PyPaletteSet.h"
#include "DbTrans.h"

#include <pybind11/eval.h>   // py::eval_file / py::exec

#include "WebShell.h"     // <MFCWebView>/WebShell.h —— DLL 门面的 C++ 包装

namespace PyArx
{
    namespace
    {
        // 先声明后定义：JsonValue → Python 的转换在下方，转换函数定义在其后
        std::string toUtf8(const std::wstring& text);
        std::wstring fromUtf8(const char* text);

        // WebView2 的 user data 目录：AutoCAD 多装在 Program Files 下，
        // exe 同级不可写，统一落到 %LOCALAPPDATA%\PyArx\WebView2Data。
        std::wstring webUserDataDir()
        {
            wchar_t base[MAX_PATH] = { 0 };
            if (::GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH) == 0 || base[0] == 0)
            {
                wchar_t tmp[MAX_PATH] = { 0 };
                ::GetTempPathW(MAX_PATH, tmp);
                wcscpy_s(base, MAX_PATH, tmp);
            }
            std::wstring dir = std::wstring(base) + L"\\PyArx\\WebView2Data";
            ::CreateDirectoryW((std::wstring(base) + L"\\PyArx").c_str(), nullptr);
            ::CreateDirectoryW(dir.c_str(), nullptr);
            return dir;
        }

        // JSON 字符串转义（不含两侧引号）。
        // DLL 化之后 webmfc::escapeJson 留在 MFCWebView.dll 内部，而这边只需要转义这一件事，
        // 不值得为它单开一个导出符号，就地实现一份。
        std::wstring jsonEscape(const std::wstring& text)
        {
            std::wstring out;
            out.reserve(text.size() + 8);
            for (const wchar_t ch : text)
            {
                switch (ch)
                {
                case L'"':  out += L"\\\""; break;
                case L'\\': out += L"\\\\"; break;
                case L'\b': out += L"\\b";  break;
                case L'\f': out += L"\\f";  break;
                case L'\n': out += L"\\n";  break;
                case L'\r': out += L"\\r";  break;
                case L'\t': out += L"\\t";  break;
                default:
                    if (ch < 0x20)
                    {
                        wchar_t buf[8] = { 0 };
                        swprintf_s(buf, L"\\u%04x", static_cast<unsigned>(ch));
                        out += buf;
                    }
                    else
                    {
                        out += ch;
                    }
                    break;
                }
            }
            return out;
        }

        // 断点日志：每行都 fopen/fflush/close，崩溃也不会丢最后一步（写 G:\PyArx\docs\_bridge.log）
        void bridgeLog(const std::wstring& step)
        {
            const std::wstring path = L"G:\\PyArx\\docs\\_bridge.log";
            FILE* f = nullptr;
            if (_wfopen_s(&f, path.c_str(), L"ab") != 0 || f == nullptr)
                return;
            SYSTEMTIME st;
            ::GetLocalTime(&st);
            char head[64] = { 0 };
            sprintf_s(head, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            const std::string body = head + toUtf8(step) + "\r\n";
            fwrite(body.data(), 1, body.size(), f);
            fflush(f);
            fclose(f);
        }

        // 页面命令读取：直接用 Python 自己的 json.loads 做真解析。
        // （早先这里自己扫字符串踩过 \n 变字面反斜杠、路径反斜杠被吃；换成 MFCWebView 的
        //  JsonValue 之后在 ARX 上下文里又稳定抛异常，所以干脆交给解释器解析，
        //  解析结果本来就是 Python 对象，args 可以直接塞给脚本，不用再转一层。）
        py::dict parseCommandPy(const std::wstring& json)
        {
            py::object loads = py::module_::import("json").attr("loads");
            py::object parsed = loads(py::str(toUtf8(json)));
            if (!py::isinstance<py::dict>(parsed))
                throw std::runtime_error("报文不是 JSON 对象");
            return parsed.cast<py::dict>();
        }

        // 从解析好的 dict 里按 key 取字符串 / 数字，取不到给默认值
        std::wstring dictStr(const py::dict& d, const char* key)
        {
            if (!d.contains(key))
                return std::wstring();
            py::object v = d[key];
            if (v.is_none())
                return std::wstring();
            return fromUtf8(py::str(v).cast<std::string>().c_str());
        }

        double dictNum(const py::dict& d, const char* key, double dflt)
        {
            if (!d.contains(key))
                return dflt;
            py::object v = d[key];
            if (v.is_none())
                return dflt;
            try
            {
                return v.cast<double>();
            }
            catch (py::error_already_set&)
            {
                return dflt;
            }
        }

        // 侧栏按钮回调属于 modeless 上下文：此时 acad 其实没持写锁，但 myLockMode()
        // 仍可能返回 kWrite，DbTrans 据此跳过加锁，currentSpace() 就取不到（eLockViolation）。
        // 这里显式锁一次当前文档，锁不住就直接回失败，不再往下写图形库。
        class WebDocLock
        {
        public:
            WebDocLock()
            {
                m_doc = acDocManagerPtr()->curDocument();
                if (m_doc == nullptr)
                {
                    m_es = Acad::eNullObjectPointer;   // 无当前文档
                    return;
                }
                m_es = acDocManagerPtr()->lockDocument(m_doc, AcAp::kWrite);
                if (m_es == Acad::eOk)
                    m_locked = true;
                else if (m_es == Acad::eLockViolation)
                {
                    // 命令上下文已由 acad 持锁，属预期；本实例不负责解锁
                    m_es = Acad::eOk;
                    m_locked = false;
                }
            }

            ~WebDocLock()
            {
                if (m_locked && m_doc != nullptr)
                    acDocManagerPtr()->unlockDocument(m_doc);
            }

            WebDocLock(const WebDocLock&) = delete;
            WebDocLock& operator=(const WebDocLock&) = delete;

            bool IsOk() const { return m_doc != nullptr && m_es == Acad::eOk; }

        private:
            AcApDocument* m_doc = nullptr;
            Acad::ErrorStatus m_es = Acad::eNullObjectPointer;
            bool m_locked = false;
        };

        // 模型空间实体数
        int modelSpaceCount(AcDbDatabase* db)
        {
            if (db == nullptr)
                return -1;
            AcDbBlockTableRecord* btr = nullptr;
            if (acdbOpenObject(btr, db->currentSpaceId(), AcDb::kForRead) != Acad::eOk || btr == nullptr)
                return -1;
            int n = 0;
            AcDbBlockTableRecordIterator* it = nullptr;
            if (btr->newIterator(it) == Acad::eOk && it != nullptr)
            {
                for (; !it->done(); it->step())
                    ++n;
                delete it;
            }
            btr->close();
            return n;
        }

        std::string toUtf8(const std::wstring& text)
        {
            if (text.empty())
                return std::string();
            const int len = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                                                  nullptr, 0, nullptr, nullptr);
            std::string out(static_cast<size_t>(len > 0 ? len : 0), '\0');
            if (len > 0)
                ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                                      &out[0], len, nullptr, nullptr);
            return out;
        }

        std::wstring fromUtf8(const char* text)
        {
            if (text == nullptr || text[0] == '\0')
                return std::wstring();
            const int len = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
            std::wstring out(static_cast<size_t>(len > 0 ? len - 1 : 0), L'\0');
            if (len > 0)
                ::MultiByteToWideChar(CP_UTF8, 0, text, -1, &out[0], len);
            return out;
        }

        // 页面命令 py.run / py.exec 的执行体：在 CAD 进程内那个 Python 解释器里跑脚本。
        // 解释器由 PyArxEngine 创建并常驻；PyArxCore.pyd 只是它 import 进来的模块，
        // 所以这里必须先拿 GIL 才能碰任何 Python 对象。
        struct PyScriptOutcome
        {
            bool ok = false;
            std::wstring text;      // 回执正文（脚本里的 result 变量）
            std::wstring printed;   // 脚本里 print() 出来的内容（UTF-8 转宽字符）
        };

        PyScriptOutcome runPythonScript(const std::wstring& filePath, const std::wstring& code,
                                        py::object args = py::none())
        {
            PyScriptOutcome out;

            if (!::Py_IsInitialized())
            {
                out.text = L"Python 解释器还没起来（先跑一次 PYRUNFILE 或 PYPIP 让引擎初始化）";
                return out;
            }

            bridgeLog(L"runPythonScript：拿 GIL");
            py::gil_scoped_acquire gil;
            bridgeLog(L"runPythonScript：GIL 到手");

            // 脚本跑在独立命名空间里：不污染 __main__，也便于回读 result
            py::dict globals;
            globals["__name__"] = py::cast(std::string("__palette_script__"));
            if (!filePath.empty())
                globals["__file__"] = py::cast(toUtf8(filePath));
            // 页面传的 args（已经是解析好的 Python 对象）原样变成脚本里的 args 变量
            globals["args"] = args;

            // print 的去处：完全交给 Python 引擎自己的重定向
            // （sys.stdout 已是 PyArxEngine.Redir.ArxOutPut，print 直接进 CAD 命令行）。
            // 桥这边不再套任何 stdout 包装，回执里只带脚本的 result 变量。
            std::string captured;
            try
            {
                bridgeLog(L"runPythonScript：准备执行脚本");
                if (!filePath.empty())
                    py::eval_file(toUtf8(filePath), globals, globals);
                else
                    py::exec(toUtf8(code), globals, globals);
                bridgeLog(L"runPythonScript：脚本执行完");

                out.ok = true;
                if (globals.contains("result"))
                    out.text = fromUtf8(py::str(globals["result"]).cast<std::string>().c_str());
                if (out.text.empty())
                    out.text = filePath.empty() ? L"代码已执行（没有 result 变量）"
                                                : L"脚本已执行：" + filePath;
            }
            catch (py::error_already_set& err)
            {
                bridgeLog(L"runPythonScript：脚本抛 Python 异常");
                out.ok = false;
                out.text = fromUtf8(err.what());
            }

            out.printed = fromUtf8(captured.c_str());
            return out;
        }
    }

    PyPaletteWeb::PyPaletteWeb() = default;
    PyPaletteWeb::~PyPaletteWeb() = default;

    bool PyPaletteWeb::attachPage(PyPaletteImpl* page, std::wstring& err)
    {
        err.clear();
        if (page == nullptr || !::IsWindow(page->Hwnd()))
        {
            err = L"侧栏页窗口无效";
            return false;
        }
        if (m_shell)
            return true;

        m_shell = std::make_unique<webmfc::WebShell>();
        if (!m_shell->attach(page->Hwnd(), webUserDataDir(), err))
        {
            m_shell.reset();
            return false;
        }
        // 页面消息先给这条命令桥；返回 false 才落回 WebShell 的内置路由
        m_shell->setCommandHandler([this](const std::wstring& json) { return this->onCommand(json); });
        return true;
    }

    bool PyPaletteWeb::loadPage(const std::wstring& htmlPath, std::wstring& err)
    {
        err.clear();
        if (!m_shell)
        {
            err = L"WebView 尚未挂到侧栏页";
            return false;
        }
        if (!m_shell->loadPage(htmlPath))
        {
            err = L"HTML 载入失败：" + htmlPath;
            return false;
        }
        return true;
    }

    bool PyPaletteWeb::emit(const std::wstring& json)
    {
        return m_shell ? m_shell->emit(json) : false;
    }

    bool PyPaletteWeb::eval(const std::wstring& js)
    {
        return m_shell ? m_shell->eval(js) : false;
    }

    void PyPaletteWeb::detach()
    {
        if (m_shell)
            m_shell->detach();
    }

    bool PyPaletteWeb::valid() const
    {
        return m_shell != nullptr && m_shell->host() != nullptr;
    }

    void PyPaletteWeb::reply(bool ok, const std::wstring& text)
    {
        const std::wstring json = L"{\"event\":\"cad.result\",\"ok\":" + std::wstring(ok ? L"true" : L"false") +
                                  L",\"text\":\"" + jsonEscape(text) + L"\"}";
        emit(json);
    }

    bool PyPaletteWeb::onCommand(const std::wstring& json)
    {
        // WebView2 的消息回调里不能有异常逃出去：一旦穿过 COM 边界，CAD 的命令通道
        // 会被打成 RPC_E_SERVERFAULT（进程还在，但后续 PYRUNFILE 全失败）。
        try
        {
            bridgeLog(L"onCommand 进入，长度=" + std::to_wstring(json.size()));
            const bool handled = onCommandInner(json);
            bridgeLog(L"onCommand 返回 " + std::wstring(handled ? L"true" : L"false"));
            return handled;
        }
        catch (const std::exception& e)
        {
            bridgeLog(L"onCommand 抛异常(std)：");
            reply(false, L"桥内部异常：" + fromUtf8(e.what()));
            return true;
        }
        catch (CException* e)
        {
            // MFC 的异常不是 std::exception，必须单独接住，否则只能落到 catch(...) 看不出内容
            wchar_t msg[512] = { 0 };
            e->GetErrorMessage(msg, 512);
            bridgeLog(L"onCommand 抛 MFC 异常：");
            bridgeLog(msg);
            e->Delete();
            reply(false, L"桥内部异常（MFC）：" + std::wstring(msg));
            return true;
        }
        catch (...)
        {
            bridgeLog(L"onCommand 抛异常(未知)");
            reply(false, L"桥内部异常（未知类型）");
            return true;
        }
    }

    bool PyPaletteWeb::onCommandInner(const std::wstring& json)
    {
        // 回调里传进来的 json 可能是临时对象，先复制一份再持有
        std::wstring body(json);
        bridgeLog(L"报文内容=" + body.substr(0, 90));
        if (!::Py_IsInitialized())
        {
            reply(false, L"Python 解释器还没起来");
            return true;
        }

        // 解析交给解释器自己的 json.loads：拿到的直接是 Python 对象，args 可以原样喂给脚本
        py::gil_scoped_acquire gil;
        py::dict root;
        try
        {
            root = parseCommandPy(body);
        }
        catch (py::error_already_set& err)
        {
            bridgeLog(L"parseCommandPy 抛 Python 异常");
            reply(false, L"报文解析失败：" + fromUtf8(err.what()));
            return true;
        }

        const std::wstring cmd = dictStr(root, "cmd");
        bridgeLog(L"解析成功，cmd=" + cmd);
        if (cmd.empty())
            return false;
        if (cmd.rfind(L"cad.", 0) != 0 && cmd.rfind(L"py.", 0) != 0)
            return false;   // 不是本桥的命令：交回 WebShell 内置路由

        if (cmd == L"cad.line")
        {
            const double x1 = dictNum(root, "x1", 0.0);
            const double y1 = dictNum(root, "y1", 0.0);
            const double x2 = dictNum(root, "x2", 100.0);
            const double y2 = dictNum(root, "y2", 0.0);

            // 侧栏回调里先拿文档写锁，再开事务
            WebDocLock lock;
            if (!lock.IsOk())
            {
                reply(false, L"文档加锁失败，无法写图形库");
                return true;
            }

            DbTrans trans;
            if (!trans.IsOk())
            {
                reply(false, L"文档加锁失败，无法写图形库");
                return true;
            }
            AcDbBlockTableRecord* btr = trans.currentSpace();
            if (btr == nullptr)
            {
                reply(false, L"取模型空间失败");
                return true;
            }
            auto* line = new AcDbLine(AcGePoint3d(x1, y1, 0.0), AcGePoint3d(x2, y2, 0.0));
            const AcDbObjectId id = trans.addEntity(btr, line);
            if (id == AcDbObjectId::kNull)
            {
                reply(false, L"追加直线失败");
                return true;
            }
            wchar_t buf[256] = { 0 };
            swprintf_s(buf, L"已在模型空间创建直线 (%.1f,%.1f)→(%.1f,%.1f)，当前实体 %d 个",
                       x1, y1, x2, y2, modelSpaceCount(trans.pDb));
            reply(true, buf);
            acutPrintf(L"\n[PyArx Web] %s\n", buf);   // 命令行也留一条痕（结尾换行，避免下一条输出接在同一行）
            return true;
        }

        if (cmd == L"cad.count")
        {
            WebDocLock lock;   // 只读也先锁，modeless 上下文里不锁打不开块表
            AcDbDatabase* db = lock.IsOk() ? acdbHostApplicationServices()->workingDatabase() : nullptr;
            const int n = modelSpaceCount(db);
            wchar_t buf[128] = { 0 };
            swprintf_s(buf, L"当前模型空间实体数：%d", n);
            reply(n >= 0, buf);
            return true;
        }

        if (cmd == L"cad.info")
        {
            WebDocLock lock;
            AcDbDatabase* db = lock.IsOk() ? acdbHostApplicationServices()->workingDatabase() : nullptr;
            const int n = modelSpaceCount(db);
            wchar_t buf[256] = { 0 };
            swprintf_s(buf, L"已接 ObjectARX：当前文档实体 %d 个", n);
            reply(n >= 0, buf);
            return true;
        }

        if (cmd == L"py.run" || cmd == L"py.exec")
        {
            const std::wstring file = dictStr(root, "file");
            const std::wstring code = dictStr(root, "code");
            bridgeLog(L"py 分支：code 长度=" + std::to_wstring(code.size()) +
                      L"，file=" + (file.empty() ? L"(空)" : file));
            if (file.empty() && code.empty())
            {
                reply(false, L"py.run 需要 file 或 code 参数（路径的反斜杠要写成 \\\\ 或用正斜杠）");
                return true;
            }
            py::object args = root.contains("args") ? py::object(root["args"]) : py::none();
            bridgeLog(L"py 分支：args " + std::wstring(!args.is_none() ? L"有" : L"无"));

            // 脚本自己开 PyDbTrans 时由它管文档锁，这里不再套一层，否则嵌套加锁语义混乱
            PyScriptOutcome outcome = runPythonScript(file, code, args);
            bridgeLog(L"py 分支：脚本返回 ok=" + std::wstring(outcome.ok ? L"true" : L"false"));

            // print 已经由引擎的 ArxOutPut 打进 CAD 命令行，这里只把它并进回执给页面
            if (!outcome.printed.empty())
                outcome.text += L"\n--- print ---\n" + outcome.printed;
            reply(outcome.ok, outcome.text);

            {
                wchar_t buf[512] = { 0 };
                swprintf_s(buf, L"py 命令执行：%ls → %ls\n", file.empty() ? L"<内联代码>" : file.c_str(),
                           outcome.ok ? L"成功" : L"失败");
                acutPrintf(L"\n[PyArx Web] %ls", buf);
            }
            return true;
        }

        reply(false, L"未知的 CAD 命令：" + cmd);
        return true;
    }
}
