"""把 Canvas2D 渲染模板改造成「数据外置」的独立 web 应用。

模板里 scene JSON 是内联的（__SCENE__ 占位），这里改成：
  1) <script id="scene">null</script>
  2) 主渲染脚本改为 type="text/plain"，不自动执行
  3) 末尾插入 loader：fetch(?scene=xxx.json) -> 填进 #scene -> 注入主脚本

用法：python make_web.py
产物：web/index.html
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TPL = os.path.join(ROOT, "examples", "cad2d", "webview_template.html")
OUT = os.path.join(HERE, "web", "index.html")

LOADER = """<script>
(function () {
  function hookShell() {
    if (!(window.chrome && chrome.webview)) return;
    var bar = document.getElementById('bar');
    if (!bar) return;
    function mk(label, cmd) {
      var a = document.createElement('a');
      a.textContent = label;
      a.style.cssText = 'color:#6BA5FF;text-decoration:none;cursor:pointer;';
      a.onclick = function () { chrome.webview.postMessage({ cmd: cmd }); };
      bar.insertBefore(a, bar.firstChild);
    }
    mk('\u91cd\u8f7d', 'reload');
    mk('\u6253\u5f00 JSON', 'open');
  }
  var q = new URLSearchParams(location.search).get('scene') || 'scene.json';
  fetch(q, { cache: 'no-store' }).then(function (r) {
    if (!r.ok) throw new Error('HTTP ' + r.status + ' ' + q);
    return r.json();
  }).then(function (j) {
    document.getElementById('scene').textContent = JSON.stringify(j);
    var s = document.createElement('script');
    s.textContent = document.getElementById('mainjs').textContent;
    document.body.appendChild(s);
    hookShell();
  }).catch(function (e) {
    var d = document.createElement('div');
    d.style.cssText = 'position:absolute;left:50%;top:44%;transform:translate(-50%,-50%);' +
      'color:#E6E8EB;font:14px/1.8 "Microsoft YaHei",sans-serif;text-align:center;z-index:9;line-height:1.8';
    d.innerHTML = '<div style="font-size:15px;margin-bottom:6px">没有加载到数据文件</div>' +
      '<div style="color:#8A94A6;font-size:12px">把 .scene.json 改名 scene.json 放在本目录，' +
      '或用 ?scene=xxx.json 指定<br>' + String((e && e.message) || e).replace(/</g, '&lt;') + '</div>';
    document.body.appendChild(d);
  });
})();
</script>
"""


def main():
    with open(TPL, "r", encoding="utf-8") as f:
        html = f.read()

    a = '<script id="scene" type="application/json">__SCENE__</script>'
    b = '<script id="scene" type="application/json">null</script>'
    if a not in html:
        sys.exit("模板结构已变化：找不到 scene 占位")
    html = html.replace(a, b, 1)

    c = "\n<script>\n(function () {\n  const D = JSON.parse"
    d = '\n<script id="mainjs" type="text/plain">\n(function () {\n  const D = JSON.parse'
    if c not in html:
        sys.exit("模板结构已变化：找不到主脚本起点")
    html = html.replace(c, d, 1)

    i = html.rindex("</body>")
    html = html[:i] + LOADER + html[i:]

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(html)
    print("生成 %s  %.1f KB" % (OUT, os.path.getsize(OUT) / 1024.0))


if __name__ == "__main__":
    main()
