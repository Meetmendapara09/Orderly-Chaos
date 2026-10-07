/*!
 * Orderly Chaos docs: page enhancements.
 * Copyright (c) 2026 Meet Mendapara. MIT License.
 *
 * Loaded as an ES module on every page:
 *   - syntax highlighting with highlight.js (loaded by the page)
 *   - diagrams with Mermaid (imported on demand, only on pages that use it)
 *   - mobile navigation toggle
 *   - heading permalinks and table-of-contents scrollspy
 */

const MERMAID_URL = "https://cdn.jsdelivr.net/npm/mermaid@12.1.0/dist/mermaid.esm.min.mjs";

/* Mermaid theme matched to the site's design tokens. */
const MERMAID_CONFIG = {
    startOnLoad: false,
    securityLevel: "strict",
    theme: "base",
    fontFamily: "'IBM Plex Sans', -apple-system, 'Segoe UI', sans-serif",
    themeVariables: {
        fontFamily: "'IBM Plex Sans', -apple-system, 'Segoe UI', sans-serif",
        fontSize: "14px",
        background: "#ffffff",
        primaryColor: "#fdf3dc",
        primaryBorderColor: "#c9a24a",
        primaryTextColor: "#23251d",
        secondaryColor: "#e5f0ea",
        secondaryBorderColor: "#7fae95",
        secondaryTextColor: "#23251d",
        tertiaryColor: "#f4f5f0",
        tertiaryBorderColor: "#bfc1b7",
        tertiaryTextColor: "#23251d",
        lineColor: "#6c6e63",
        textColor: "#23251d",
        mainBkg: "#fdf3dc",
        nodeBorder: "#c9a24a",
        clusterBkg: "#f7f8f3",
        clusterBorder: "#bfc1b7",
        edgeLabelBackground: "#ffffff",
        actorBkg: "#fdf3dc",
        actorBorder: "#c9a24a",
        actorTextColor: "#23251d",
        signalColor: "#4d4f46",
        signalTextColor: "#23251d",
        labelBoxBkgColor: "#f4f5f0",
        labelBoxBorderColor: "#bfc1b7",
        noteBkgColor: "#e7d8ee",
        noteBorderColor: "#a07cbf",
        noteTextColor: "#23251d",
        activationBkgColor: "#e5f0ea",
        activationBorderColor: "#7fae95",
    },
    flowchart: { curve: "basis", htmlLabels: true, useMaxWidth: true },
    sequence: { useMaxWidth: true, mirrorActors: false },
    class: { useMaxWidth: true },
    state: { useMaxWidth: true },
};

function highlightCode() {
    if (!window.hljs) return;
    document.querySelectorAll("pre code[class*='language-']").forEach((block) => {
        window.hljs.highlightElement(block);
    });
}

async function renderDiagrams() {
    const diagrams = document.querySelectorAll("pre.mermaid");
    if (diagrams.length === 0) return;
    try {
        const { default: mermaid } = await import(MERMAID_URL);
        mermaid.initialize(MERMAID_CONFIG);
        await mermaid.run({ nodes: diagrams });
    } catch (error) {
        console.warn("Diagrams could not be rendered:", error);
        diagrams.forEach((node) => {
            node.style.color = "inherit";
            node.style.textAlign = "left";
            node.setAttribute("data-processed", "fallback");
        });
    }
}

function addHeadingAnchors() {
    document.querySelectorAll(".doc-content h2[id], .doc-content h3[id]").forEach((heading) => {
        if (heading.querySelector(".heading-anchor")) return;
        const anchor = document.createElement("a");
        anchor.className = "heading-anchor";
        anchor.href = `#${heading.id}`;
        anchor.setAttribute("aria-label", `Link to section: ${heading.textContent.trim()}`);
        anchor.textContent = "#";
        heading.appendChild(anchor);
    });
}

function setupScrollspy() {
    const links = [...document.querySelectorAll(".toc a[href^='#']")];
    if (links.length === 0 || !("IntersectionObserver" in window)) return;
    const byId = new Map(links.map((link) => [decodeURIComponent(link.hash.slice(1)), link]));
    const visible = new Set();
    const observer = new IntersectionObserver((entries) => {
        entries.forEach((entry) => {
            if (entry.isIntersecting) visible.add(entry.target.id);
            else visible.delete(entry.target.id);
        });
        const first = [...byId.keys()].find((id) => visible.has(id));
        if (!first) return;
        links.forEach((link) => link.classList.remove("active"));
        byId.get(first).classList.add("active");
    }, { rootMargin: "-70px 0px -65% 0px" });
    byId.forEach((_link, id) => {
        const target = document.getElementById(id);
        if (target) observer.observe(target);
    });
}

function setupNavigation() {
    document.addEventListener("click", (event) => {
        const toggle = event.target.closest(".nav-toggle");
        const header = document.getElementById("header-slot");
        if (!header) return;
        if (toggle) {
            const open = header.classList.toggle("nav-open");
            toggle.setAttribute("aria-expanded", String(open));
        } else if (event.target.closest(".nav-links a")) {
            header.classList.remove("nav-open");
        }
    });
    document.addEventListener("keydown", (event) => {
        const header = document.getElementById("header-slot");
        if (event.key === "Escape" && header && header.classList.contains("nav-open")) {
            header.classList.remove("nav-open");
            const toggle = header.querySelector(".nav-toggle");
            if (toggle) {
                toggle.setAttribute("aria-expanded", "false");
                toggle.focus();
            }
        }
    });
}

setupNavigation();
highlightCode();
addHeadingAnchors();
setupScrollspy();
renderDiagrams();
