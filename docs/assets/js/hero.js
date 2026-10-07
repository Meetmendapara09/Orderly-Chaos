/*!
 * Orderly Chaos docs: animated order book on the landing page.
 * Copyright (c) 2026 Meet Mendapara. MIT License.
 *
 * A purely decorative random walk. Asks are listed worst-to-best (highest
 * price on top) and bids best-to-worst, so the best prices always sit next
 * to the mid line, like a real depth ladder. The animation pauses while the
 * tab is hidden and does not run when the user prefers reduced motion.
 */
(function () {
    "use strict";

    var card = document.querySelector(".order-book-card");
    if (!card) return;

    var asks = card.querySelectorAll(".asks .ob-row");
    var bids = card.querySelectorAll(".bids .ob-row");
    var midEl = card.querySelector(".ob-mid");
    var spreadEl = card.querySelector(".ob-spread");
    var volumeEl = card.querySelector(".ob-volume");
    var TICK = 0.01;

    var bestBid = 102.40;
    var spread = 0.02;
    var askSizes = [200, 134, 80];   /* index 0 = best ask */
    var bidSizes = [163, 225, 102];  /* index 0 = best bid */

    function randInt(min, max) { return Math.floor(Math.random() * (max - min + 1)) + min; }
    function fmt(price) { return price.toFixed(2); }

    function render(flash) {
        var bestAsk = bestBid + spread;
        var maxSize = Math.max.apply(null, askSizes.concat(bidSizes));
        for (var i = 0; i < 3; i++) {
            /* asks: row 0 (top) is the worst level, row 2 sits next to the mid */
            var askLevel = 2 - i;
            var askRow = asks[i];
            askRow.querySelector(".ob-price").textContent = fmt(bestAsk + askLevel * TICK * 2);
            askRow.querySelector(".ob-size").textContent = askSizes[askLevel];
            askRow.querySelector(".ob-depth").style.setProperty("--depth", Math.round(askSizes[askLevel] / maxSize * 100) + "%");
            /* bids: row 0 (top) is the best level, next to the mid */
            var bidRow = bids[i];
            bidRow.querySelector(".ob-price").textContent = fmt(bestBid - i * TICK * 2);
            bidRow.querySelector(".ob-size").textContent = bidSizes[i];
            bidRow.querySelector(".ob-depth").style.setProperty("--depth", Math.round(bidSizes[i] / maxSize * 100) + "%");
        }
        midEl.textContent = "mid " + fmt(bestBid + spread / 2);
        spreadEl.textContent = "spread " + fmt(spread);
        var total = askSizes.concat(bidSizes).reduce(function (a, b) { return a + b; }, 0);
        if (volumeEl) volumeEl.textContent = String(total);
        if (flash) {
            var rows = card.querySelectorAll(".ob-row");
            var row = rows[randInt(0, rows.length - 1)];
            row.classList.remove("flash");
            void row.offsetWidth; /* restart the animation */
            row.classList.add("flash");
        }
    }

    function tick() {
        if (document.hidden) return;
        bestBid = Math.max(100, bestBid + randInt(-1, 1) * TICK);
        spread = randInt(1, 4) * TICK;
        for (var i = 0; i < 3; i++) {
            askSizes[i] = Math.max(10, askSizes[i] + randInt(-25, 25));
            bidSizes[i] = Math.max(10, bidSizes[i] + randInt(-25, 25));
        }
        render(true);
    }

    render(false);
    if (!window.matchMedia || !window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
        setInterval(tick, 1200);
    }
})();
