/*!
 * Orderly Chaos docs — copy-to-clipboard for code blocks.
 * Self-contained: injects its own styles, no dependencies.
 * Include with: <script src="copy-code.js" defer></script>
 */
(function () {
    "use strict";

    var STYLE =
        "pre{position:relative}" +
        ".copy-btn{position:absolute;top:10px;right:10px;padding:5px 12px;" +
        "font:600 11px/1 ui-sans-serif,system-ui,-apple-system,sans-serif;letter-spacing:.03em;" +
        "color:#c7c9d9;background:rgba(255,255,255,.07);border:1px solid rgba(255,255,255,.16);" +
        "border-radius:7px;cursor:pointer;opacity:0;z-index:2;" +
        "transition:opacity .15s ease,background .15s ease,color .15s ease,border-color .15s ease;" +
        "-webkit-backdrop-filter:blur(8px);backdrop-filter:blur(8px)}" +
        "pre:hover>.copy-btn,.copy-btn:focus-visible,.copy-btn.copied{opacity:1}" +
        ".copy-btn:hover{background:rgba(99,102,245,.28);border-color:rgba(129,140,248,.55);color:#fff}" +
        ".copy-btn.copied{background:rgba(16,185,129,.18);border-color:rgba(52,211,153,.55);color:#34d399}" +
        ".copy-btn svg{width:12px;height:12px;vertical-align:-2px;margin-right:5px;fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}" +
        "@media (hover:none){.copy-btn{opacity:.85}}" +
        "@media (prefers-reduced-motion:reduce){.copy-btn{transition:none}}";

    var ICON_COPY = '<svg viewBox="0 0 24 24" aria-hidden="true"><rect x="9" y="9" width="13" height="13" rx="2"/><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"/></svg>';
    var LABEL_IDLE = ICON_COPY + "Copy";
    var LABEL_DONE = ICON_COPY + "Copied!";
    var LABEL_FAIL = ICON_COPY + "Ctrl+C";

    function flash(btn, label, cls) {
        btn.innerHTML = label;
        if (cls) btn.classList.add(cls);
        setTimeout(function () {
            btn.innerHTML = LABEL_IDLE;
            btn.classList.remove("copied");
        }, 2000);
    }

    function legacyCopy(text) {
        var ta = document.createElement("textarea");
        ta.value = text;
        ta.setAttribute("readonly", "");
        ta.style.position = "fixed";
        ta.style.opacity = "0";
        document.body.appendChild(ta);
        ta.select();
        var ok = false;
        try { ok = document.execCommand("copy"); } catch (e) { ok = false; }
        document.body.removeChild(ta);
        return ok;
    }

    function wire(pre) {
        if (pre.querySelector(".copy-btn")) return;
        var code = pre.querySelector("code");
        if (!code) return;

        var btn = document.createElement("button");
        btn.type = "button";
        btn.className = "copy-btn";
        btn.setAttribute("aria-label", "Copy code to clipboard");
        btn.innerHTML = LABEL_IDLE;

        btn.addEventListener("click", function () {
            // innerText keeps rendered line breaks, drops syntax-highlight spans,
            // and excludes this button because we read from <code>, not <pre>.
            var text = code.innerText.replace(/\s+$/, "");
            if (!text) return;
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(text).then(
                    function () { flash(btn, LABEL_DONE, "copied"); },
                    function () {
                        if (legacyCopy(text)) flash(btn, LABEL_DONE, "copied");
                        else flash(btn, LABEL_FAIL);
                    }
                );
            } else if (legacyCopy(text)) {
                flash(btn, LABEL_DONE, "copied");
            } else {
                flash(btn, LABEL_FAIL);
            }
        });

        pre.appendChild(btn);
    }

    function init() {
        var style = document.createElement("style");
        style.textContent = STYLE;
        document.head.appendChild(style);

        var pres = document.querySelectorAll("pre");
        for (var i = 0; i < pres.length; i++) wire(pres[i]);
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
