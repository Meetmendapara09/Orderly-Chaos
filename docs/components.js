/*!
 * Orderly Chaos docs — shared header/footer component loader.
 *
 * Every page includes this script with:
 *   <script src="components.js" defer></script>          (docs root)
 *   <script src="../components.js" defer></script>       (api/, examples/)
 * and provides empty mount points:
 *   <header id="header-slot"></header>
 *   <footer id="footer-slot"></footer>
 *
 * The fragments live in components/header.html and components/footer.html
 * so the chrome is defined exactly once. "@/" inside a fragment is
 * rewritten to the relative path from the current page back to the docs
 * root, so the same fragment works at any directory depth. The active
 * nav link is derived from <body data-page="...">.
 */
(function () {
    "use strict";

    var script = document.currentScript;
    if (!script) {
        var all = document.getElementsByTagName("script");
        for (var i = all.length - 1; i >= 0; i--) {
            if (/components\.js($|\?)/.test(all[i].src)) { script = all[i]; break; }
        }
    }

    /* Absolute URL of the docs root directory (where components.js lives). */
    var docsRootUrl = script && script.src
        ? script.src.replace(/components\.js.*$/, "")
        : "";

    function pageDirUrl() {
        var path = window.location.href;
        var last = path.lastIndexOf("/");
        return last >= 0 ? path.slice(0, last + 1) : path;
    }

    /* Relative prefix ("", "../", "../../") from the page back to docs root. */
    function basePrefix() {
        try {
            var page = decodeURIComponent(pageDirUrl());
            var root = decodeURIComponent(docsRootUrl);
            if (root && page.indexOf(root) === 0) {
                var depth = page.slice(root.length).split("/").filter(Boolean).length;
                return new Array(depth + 1).join("../");
            }
        } catch (e) { /* fall through */ }
        return "";
    }

    function inject(slotId, fragmentUrl, base) {
        var slot = document.getElementById(slotId);
        if (!slot) return Promise.resolve();
        return fetch(fragmentUrl, { cache: "no-cache" })
            .then(function (res) { if (!res.ok) throw new Error(res.status); return res.text(); })
            .then(function (html) {
                slot.innerHTML = html.split("@/").join(base);
            })
            .catch(function () {
                /* Fragment unavailable (e.g. file:// CORS): leave slot empty. */
            });
    }

    function markActive() {
        var page = document.body.getAttribute("data-page");
        if (!page) return;
        var link = document.querySelector('#header-slot a[data-page="' + page + '"]');
        if (link) {
            link.classList.add("active");
            link.setAttribute("aria-current", "page");
        }
    }

    function init() {
        var base = basePrefix();
        Promise.all([
            inject("header-slot", docsRootUrl + "components/header.html", base),
            inject("footer-slot", docsRootUrl + "components/footer.html", base)
        ]).then(markActive);
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
