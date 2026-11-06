// SyncLens theme engine. Injected into the Resilio Sync WebUI (isolated world)
// at document creation; SyncLens then calls window.__synclensTheme.apply().
//
// Resilio's CSS has no custom properties: every colour is hard-coded, and its
// own dark mode is a set of `body.dark ...` overrides. So instead of overriding
// selectors, this rewrites colour values in place. A theme is a palette:
//
//   greys:   any number of neutral colours; Resilio's greys are mapped onto
//            them by lightness, so black→darkest and white→lightest.
//   accents: saturated colours; each Resilio colour takes the nearest hue,
//            keeping its relative lightness so hover/active shades differ.
//   primary: optional; replaces Resilio's brand blue (its highlight colour)
//            instead of the nearest-hue accent, e.g. mauve for Catppuccin.
//   colors:  optional exact {"#from": "#to"} pairs, checked first.
//   appearance: "dark" or "light"; sets Resilio's own `body.dark` class.
//
// The original value of every declaration is kept, so switching themes or
// going back to Resilio's default is exact.
(() => {
  if (window.__synclensTheme) return;

  const COLOUR_PROP = /color$|^fill$|^stroke$|shadow$|^background-image$/;
  const KEYWORDS =
    "white|black|gray|grey|silver|red|green|blue|yellow|orange|purple|navy|teal|maroon|olive|lime|aqua|fuchsia|" +
    "whitesmoke|gainsboro|lightgr[ae]y|darkgr[ae]y|dimgr[ae]y";
  const TOKEN_SRC =
    "#[0-9a-fA-F]{3,8}\\b|rgba?\\([^)]*\\)|hsla?\\([^)]*\\)|\\b(?:" +
    KEYWORDS +
    ")\\b";
  const HAS_TOKEN = new RegExp(TOKEN_SRC, "i");
  const URL_SPLIT = /(url\([^)]*\))/i;

  // ---- colour maths --------------------------------------------------------

  const ctx = document.createElement("canvas").getContext("2d");
  const parsed = new Map();
  function parse(token) {
    if (parsed.has(token)) return parsed.get(token);
    let out = null;
    ctx.fillStyle = "#010203";
    ctx.fillStyle = token;
    const v = ctx.fillStyle;
    if (v !== "#010203" || token.toLowerCase() === "#010203") {
      if (v[0] === "#")
        out = [
          parseInt(v.slice(1, 3), 16),
          parseInt(v.slice(3, 5), 16),
          parseInt(v.slice(5, 7), 16),
          1,
        ];
      else {
        const m = v.match(/[\d.]+/g);
        if (m) out = [+m[0], +m[1], +m[2], m[3] !== undefined ? +m[3] : 1];
      }
    }
    parsed.set(token, out);
    return out;
  }

  function toHsl([r, g, b]) {
    r /= 255;
    g /= 255;
    b /= 255;
    const max = Math.max(r, g, b),
      min = Math.min(r, g, b),
      l = (max + min) / 2;
    if (max === min) return [0, 0, l];
    const d = max - min;
    const s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    let h =
      max === r
        ? (g - b) / d + (g < b ? 6 : 0)
        : max === g
          ? (b - r) / d + 2
          : (r - g) / d + 4;
    return [h * 60, s, l];
  }

  function fromHsl(h, s, l) {
    const k = (n) => (n + h / 30) % 12;
    const a = s * Math.min(l, 1 - l);
    const f = (n) => l - a * Math.max(-1, Math.min(k(n) - 3, 9 - k(n), 1));
    return [
      Math.round(f(0) * 255),
      Math.round(f(8) * 255),
      Math.round(f(4) * 255),
    ];
  }

  function luminance([r, g, b]) {
    const c = (v) => {
      v /= 255;
      return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * c(r) + 0.7152 * c(g) + 0.0722 * c(b);
  }

  function contrast(a, b) {
    const x = luminance(a),
      y = luminance(b);
    return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05);
  }

  const css = ([r, g, b, a]) =>
    a >= 1 ? `rgb(${r}, ${g}, ${b})` : `rgba(${r}, ${g}, ${b}, ${a})`;
  const key = (c) => c.slice(0, 3).join(",");

  // ---- theme ---------------------------------------------------------------

  let theme = null; // compiled theme, or null for Resilio's own colours

  function compile(t) {
    if (!t) return null;
    const ramp = (t.greys || [])
      .map(parse)
      .filter(Boolean)
      .map((c) => ({ c, l: toHsl(c)[2] }))
      .sort((a, b) => a.l - b.l);
    const accents = (t.accents || [])
      .map(parse)
      .filter(Boolean)
      .map((c) => {
        const [h, s, l] = toHsl(c);
        return { h, s, l };
      });
    if (ramp.length < 2 || !accents.length) return null;
    const exact = new Map();
    for (const [from, to] of Object.entries(t.colors || {})) {
      const f = parse(from),
        x = parse(to);
      if (f && x) exact.set(key(f), x);
    }
    let primary = null;
    const pc = t.primary && parse(t.primary);
    if (pc) {
      const [h, s, l] = toHsl(pc);
      primary = { h, s, l };
    }
    return { ramp, accents, exact, primary, appearance: t.appearance };
  }

  function mapColour(c) {
    const [r, g, b, a] = c;
    if (a === 0) return c;
    const hit = theme.exact.get(key(c));
    if (hit) return [hit[0], hit[1], hit[2], a];

    const max = Math.max(r, g, b),
      min = Math.min(r, g, b);
    const l = (max + min) / 510;
    if ((max - min) / 255 < 0.12) {
      // Neutral: place it on the theme's ramp at the same relative lightness.
      const ramp = theme.ramp,
        lo = ramp[0].l,
        hi = ramp[ramp.length - 1].l;
      const target = lo + l * (hi - lo);
      for (let i = 1; i < ramp.length; i++) {
        if (target <= ramp[i].l) {
          const A = ramp[i - 1],
            B = ramp[i];
          const f = B.l === A.l ? 0 : (target - A.l) / (B.l - A.l);
          return [0, 1, 2]
            .map((j) => Math.round(A.c[j] + (B.c[j] - A.c[j]) * f))
            .concat(a);
        }
      }
      return ramp[ramp.length - 1].c.slice(0, 3).concat(a);
    }

    // Saturated: nearest accent by hue, shifted by the source's lightness.
    const h = toHsl(c)[0];
    let best = theme.accents[0],
      bestD = Infinity;
    for (const acc of theme.accents) {
      let d = Math.abs(acc.h - h);
      d = Math.min(d, 360 - d);
      if (d < bestD) {
        bestD = d;
        best = acc;
      }
    }
    // Resilio's highlight blues (hue ~205-215) all sit in this band.
    if (theme.primary && h >= 190 && h <= 250) best = theme.primary;
    const nl = Math.min(0.95, Math.max(0.05, best.l + (l - 0.5) * 0.6));
    return fromHsl(best.h, best.s, nl).concat(a);
  }

  function remapValue(value, base) {
    const tokens = new RegExp(TOKEN_SRC, "gi");
    return value
      .split(URL_SPLIT)
      .map((part) =>
        URL_SPLIT.test(part)
          ? themedUrl(part, base)
          : part.replace(tokens, (t) => {
              const c = parse(t);
              return c ? css(mapColour(c)) : t;
            }),
      )
      .join("");
  }

  // ---- SVG icons -----------------------------------------------------------
  // Resilio's icons are same-origin SVG files used as background images.
  // Fetch each once, recolour its fills and strokes with the same mapping and
  // swap in a data: URL. Until the fetch lands the original icon shows.

  const svgCache = new Map(); // absolute URL -> data: URL, or null while pending/failed
  const SVG_COLOUR = /(?<!url\()#[0-9a-fA-F]{3,8}\b|rgba?\([^)]*\)/g;
  let reapplyTimer = 0;

  // `base` is the stylesheet's URL: relative url()s resolve against it.
  function themedUrl(part, base) {
    const m = part.match(/url\(\s*(['"]?)([^'")]+)\1\s*\)/i);
    if (!m) return part;
    let abs;
    try {
      abs = new URL(m[2], base || document.baseURI);
    } catch (e) {
      return part;
    }
    if (abs.origin !== location.origin || !/\.svg$/i.test(abs.pathname))
      return part;
    const href = abs.href;
    if (svgCache.has(href)) {
      const data = svgCache.get(href);
      return data ? `url("${data}")` : part;
    }
    svgCache.set(href, null);
    const forTheme = theme;
    fetch(href)
      .then((r) => (r.ok ? r.text() : Promise.reject(r.status)))
      .then((text) => {
        if (theme !== forTheme) return; // theme changed while fetching
        const recoloured = text.replace(SVG_COLOUR, (t) => {
          const c = parse(t);
          return c ? css(mapColour(c)) : t;
        });
        svgCache.set(
          href,
          "data:image/svg+xml;charset=utf-8," + encodeURIComponent(recoloured),
        );
        clearTimeout(reapplyTimer);
        reapplyTimer = setTimeout(applyAll, 50);
      })
      .catch(() => {});
    return part;
  }

  // ---- applying to declarations --------------------------------------------

  const originals = new WeakMap(); // CSSStyleDeclaration -> Map(prop -> [value, priority])
  const written = new WeakMap(); // CSSStyleDeclaration -> Map(prop -> value we set)

  function processDecl(st, base) {
    let orig = originals.get(st),
      wrote = written.get(st);
    if (!orig) {
      orig = new Map();
      wrote = new Map();
      originals.set(st, orig);
      written.set(st, wrote);
    }

    for (let i = 0; i < st.length; i++) {
      const p = st[i];
      if (!COLOUR_PROP.test(p)) continue;
      const cur = st.getPropertyValue(p);
      // A value we didn't write is the page's own: (re)record it.
      const themable =
        HAS_TOKEN.test(cur) || (p === "background-image" && /url\(/i.test(cur));
      if (wrote.get(p) !== cur && themable)
        orig.set(p, [cur, st.getPropertyPriority(p)]);
    }
    if (!orig.size) return;

    const next = new Map();
    for (const [p, [v]] of orig) next.set(p, theme ? remapValue(v, base) : v);

    // Text on a coloured background (buttons, badges): if the remap made it
    // unreadable, use whichever end of the ramp reads better.
    if (theme && next.has("color") && next.has("background-color")) {
      const fg = parse(next.get("color")),
        bg = parse(next.get("background-color"));
      if (fg && bg && bg[3] > 0.5 && contrast(fg, bg) < 3) {
        const dark = theme.ramp[0].c,
          light = theme.ramp[theme.ramp.length - 1].c;
        const pick = contrast(dark, bg) >= contrast(light, bg) ? dark : light;
        next.set("color", css(pick.slice(0, 3).concat(fg[3])));
      }
    }

    for (const [p, v] of next) {
      if (st.getPropertyValue(p) !== v) st.setProperty(p, v, orig.get(p)[1]);
      wrote.set(p, st.getPropertyValue(p));
    }
  }

  function walk(rules, base) {
    for (const r of rules) {
      if (r.style) processDecl(r.style, base);
      if (r.cssRules) walk(r.cssRules, base);
    }
  }

  function applySheets() {
    for (const sheet of document.styleSheets) {
      if (sheet.ownerNode && sheet.ownerNode.id === "synclens-user-css")
        continue;
      let rules;
      try {
        rules = sheet.cssRules;
      } catch (e) {
        continue;
      }
      walk(rules, sheet.href || document.baseURI);
    }
  }

  function applyInline(root) {
    if (root.nodeType !== 1) return;
    if (root.hasAttribute("style")) processDecl(root.style);
    for (const el of root.querySelectorAll("[style]")) processDecl(el.style);
  }

  // ---- light/dark: drive Resilio's own body.dark class -----------------------

  // Resilio sets body.dark itself, sometimes after this script has run. Any
  // state we didn't set is Resilio's choice, restored when the theme goes.
  let resilioDark = false;
  let ourDark = null; // the state we last left the class in

  function syncBodyClass() {
    const body = document.body;
    if (!body) return;
    const isDark = body.classList.contains("dark");
    if (isDark !== ourDark) resilioDark = isDark;
    const want =
      theme && theme.appearance ? theme.appearance === "dark" : resilioDark;
    if (isDark !== want) body.classList.toggle("dark", want);
    ourDark = want;
  }

  // ---- user stylesheet -----------------------------------------------------

  let userCss = "";

  function applyUserCss() {
    let el = document.getElementById("synclens-user-css");
    if (!userCss) {
      if (el) el.remove();
      return;
    }
    if (!el) {
      el = document.createElement("style");
      el.id = "synclens-user-css";
    }
    el.textContent = userCss;
    (document.head || document.documentElement).appendChild(el); // keep it last
  }

  // ---- driving it ----------------------------------------------------------

  function applyAll() {
    if (!document.documentElement) return;
    syncBodyClass();
    applySheets();
    if (document.body) applyInline(document.body);
    applyUserCss();
  }

  let observer = null;
  function observe() {
    if (observer || !document.documentElement) return;
    observer = new MutationObserver((records) => {
      for (const rec of records) {
        if (rec.type === "attributes") {
          const el = rec.target;
          if (rec.attributeName === "style") processDecl(el.style);
          else if (el === document.body) syncBodyClass();
          continue;
        }
        for (const node of rec.addedNodes) {
          if (node.nodeType !== 1) continue;
          if (node.id === "synclens-user-css") continue;
          if (node.tagName === "LINK")
            node.addEventListener("load", applySheets, { once: true });
          else if (node.tagName === "STYLE") applySheets();
          else applyInline(node);
          if (node === document.body) applyAll();
        }
      }
    });
    observer.observe(document.documentElement, {
      subtree: true,
      childList: true,
      attributes: true,
      attributeFilter: ["style", "class"],
    });
  }

  function ready() {
    observe();
    applyAll();
  }

  if (document.readyState === "loading")
    document.addEventListener("DOMContentLoaded", ready, { once: true });
  else ready();
  window.addEventListener("load", applyAll);

  window.__synclensTheme = {
    apply(t, cssText) {
      theme = compile(t);
      svgCache.clear();
      userCss = cssText || "";
      if (document.readyState !== "loading") ready();
    },
  };
})();
