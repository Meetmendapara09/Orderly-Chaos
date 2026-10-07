/*!
 * Orderly Chaos docs: shared header and footer loader.
 * Copyright (c) 2026 Meet Mendapara. MIT License.
 *
 * Every page includes this script and provides empty mount points:
 *   <header id="header-slot" class="navbar"></header>
 *   <footer id="footer-slot" class="footer"></footer>
 *
 * The fragments live in components/header.html and components/footer.html
 * so the site chrome is defined once. "@/" inside a fragment is rewritten
 * to the relative path from the current page back to the docs root, so the
 * same fragment works at any directory depth. The active navigation link is
 * derived from <body data-page="...">. When the fragments cannot be fetched
 * (for example when opening files directly with file://), a minimal fallback
 * navigation is rendered instead so pages are never left without links.
 */
(function () {
    "use strict";

    var SCRIPT_PATH = /assets\/js\/components\.js(\?.*)?$/;

    var script = document.currentScript;
    if (!script) {
        var all = document.getElementsByTagName("script");
        for (var i = all.length - 1; i >= 0; i--) {
            if (SCRIPT_PATH.test(all[i].src)) { script = all[i]; break; }
        }
    }

    /* Absolute URL of the docs root directory. */
    var docsRootUrl = script && script.src ? script.src.replace(SCRIPT_PATH, "") : "";

    /* Relative prefix ("", "../", ...) from the current page to the docs root. */
    function basePrefix() {
        try {
            var href = window.location.href.split("#")[0].split("?")[0];
            var page = decodeURIComponent(href.slice(0, href.lastIndexOf("/") + 1));
            var root = decodeURIComponent(docsRootUrl);
            if (root && page.indexOf(root) === 0) {
                var depth = page.slice(root.length).split("/").filter(Boolean).length;
                return new Array(depth + 1).join("../");
            }
        } catch (e) { /* fall through */ }
        return "";
    }

    var FALLBACK = {
        "header-slot":
            '<div class="nav-inner"><a href="@/index.html" class="brand">Orderly Chaos</a>' +
            '<nav class="nav-links" style="display:flex;flex-wrap:wrap" aria-label="Main navigation">' +
            '<a href="@/overview.html">Overview</a><a href="@/getting-started.html">Getting started</a>' +
            '<a href="@/architecture.html">Architecture</a><a href="@/api/python.html">API</a></nav></div>',
        "footer-slot":
            '<div class="footer-legal">Copyright &copy; 2026 Meet Mendapara. Released under the MIT License.</div>'
    };

    function inject(slotId, fragmentUrl, base) {
        var slot = document.getElementById(slotId);
        if (!slot) return Promise.resolve();
        function render(html) { slot.innerHTML = html.split("@/").join(base); }
        return fetch(fragmentUrl, { cache: "no-cache" })
            .then(function (res) { if (!res.ok) throw new Error(String(res.status)); return res.text(); })
            .then(render)
            .catch(function () { render(FALLBACK[slotId] || ""); });
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
        ]).then(function () {
            markActive();
            document.dispatchEvent(new CustomEvent("oc:components-ready"));
        });
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
