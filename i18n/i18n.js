/* ==================================================================
   C-IDS LANGUAGE SYSTEM
   ------------------------------------------------------------------
   LANGUAGES        list shown in the globe menu (order, RTL flag)
   LanguageSelector the globe trigger + panel: keyboard nav, RTL,
                    persistence in localStorage ("cids-lang")
   loadLanguage()   fetches i18n/<code>.js on demand

   The translations themselves are in i18n/<code>.js, one file per
   language. To add a language: add it to LANGUAGES and create its file
   with the same keys as i18n/en.js.
   ================================================================== */

const LANGUAGES = [
  {
    "code": "en",
    "name": "English",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "fil",
    "name": "Filipino",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "fr",
    "name": "Français",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "de",
    "name": "Deutsch",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "ha",
    "name": "Hausa",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "id",
    "name": "Bahasa Indonesia",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "it",
    "name": "Italiano",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "pt",
    "name": "Português",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "es",
    "name": "Español",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "tr",
    "name": "Türkçe",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "vi",
    "name": "Tiếng Việt",
    "rtl": false,
    "group": "latin"
  },
  {
    "code": "ar",
    "name": "العربية",
    "rtl": true,
    "group": "arabic"
  },
  {
    "code": "fa",
    "name": "فارسی",
    "rtl": true,
    "group": "arabic"
  },
  {
    "code": "ur",
    "name": "اردو",
    "rtl": true,
    "group": "arabic"
  },
  {
    "code": "ru",
    "name": "Русский",
    "rtl": false,
    "group": "cyrillic"
  },
  {
    "code": "zh",
    "name": "中文",
    "rtl": false,
    "group": "cjk"
  },
  {
    "code": "ja",
    "name": "日本語",
    "rtl": false,
    "group": "cjk"
  },
  {
    "code": "ko",
    "name": "한국어",
    "rtl": false,
    "group": "cjk"
  },
  {
    "code": "hi",
    "name": "हिन्दी",
    "rtl": false,
    "group": "devanagari"
  },
  {
    "code": "mr",
    "name": "मराठी",
    "rtl": false,
    "group": "devanagari"
  },
  {
    "code": "bn",
    "name": "বাংলা",
    "rtl": false,
    "group": "bengali"
  },
  {
    "code": "pa",
    "name": "ਪੰਜਾਬੀ",
    "rtl": false,
    "group": "gurmukhi"
  },
  {
    "code": "ta",
    "name": "தமிழ்",
    "rtl": false,
    "group": "tamil"
  },
  {
    "code": "te",
    "name": "తెలుగు",
    "rtl": false,
    "group": "telugu"
  }
];

const LANG_STORAGE_KEY = "cids-lang";

class LanguageSelector {
  constructor({ languages, translations, storageKey, defaultCode, onChange, root = document }) {
    this.languages = languages;
    this.translations = translations;
    this.storageKey = storageKey || "preferredLanguage";
    this.defaultCode = defaultCode || "en";
    this.onChange = onChange || (() => {});
    this.root = root;

    this.dom = {
      trigger: root.getElementById("lang-trigger"),
      panel: root.getElementById("lang-panel"),
      list: root.getElementById("lang-list"),
    };

    this.open = false;
    this.current = this._loadSavedLanguage();
    this._keydownHandler = null;
    this._outsideClickHandler = null;

    this._renderOptions();
    this._bindTriggerEvents();
    this._applyLanguage(this.current, { persist: false, silent: true });
  }

  t(key) {
    const dict = this.translations[this.current] || this.translations[this.defaultCode];
    const fallback = this.translations[this.defaultCode] || {};
    return (dict && dict[key]) || fallback[key] || key;
  }

  getCurrent() { return this.current; }

  isRtl(code) {
    const lang = this.languages.find((l) => l.code === code);
    return !!(lang && lang.rtl);
  }

  _loadSavedLanguage() {
    try {
      const saved = window.localStorage.getItem(this.storageKey);
      if (saved && this.languages.some((l) => l.code === saved)) return saved;
    } catch (err) {
      console.warn("LanguageSelector \u2014 localStorage unavailable:", err);
    }
    return this.defaultCode;
  }

  _saveLanguage(code) {
    try { window.localStorage.setItem(this.storageKey, code); }
    catch (err) { console.warn("LanguageSelector \u2014 could not persist language:", err); }
  }

  _renderOptions() {
    const { list } = this.dom;
    while (list.firstChild) list.removeChild(list.firstChild);

    this.languages.forEach((lang) => {
      const li = document.createElement("li");
      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "lang-option";
      btn.setAttribute("role", "option");
      btn.setAttribute("data-lang-code", lang.code);
      btn.setAttribute("aria-selected", String(lang.code === this.current));
      btn.setAttribute("lang", lang.code);

      const label = document.createElement("span");
      label.textContent = lang.name;

      const check = document.createElement("span");
      check.className = "lang-check";
      check.setAttribute("aria-hidden", "true");
      check.innerHTML =
        '<svg viewBox="0 0 20 20" width="14" height="14" fill="none" xmlns="http://www.w3.org/2000/svg">' +
        '<path d="M4 10.5l3.5 3.5L16 6" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>' +
        "</svg>";

      btn.appendChild(label);
      btn.appendChild(check);

      btn.addEventListener("click", () => {
        this._applyLanguage(lang.code, { persist: true, silent: false });
        this.close();
        this.dom.trigger.focus();
      });

      li.appendChild(btn);
      list.appendChild(li);
    });
  }

  _applyLanguage(code, { persist, silent }) {
    if (!this.languages.some((l) => l.code === code)) return;
    this.current = code;

    const rtl = this.isRtl(code);
    document.documentElement.setAttribute("lang", code);
    document.documentElement.setAttribute("dir", rtl ? "rtl" : "ltr");

    this.dom.list.querySelectorAll(".lang-option").forEach((btn) => {
      btn.setAttribute("aria-selected", String(btn.getAttribute("data-lang-code") === code));
    });

    if (persist) this._saveLanguage(code);
    if (!silent) this.onChange(code);
  }

  _bindTriggerEvents() {
    const { trigger, panel } = this.dom;
    trigger.addEventListener("click", () => this.toggle());
    trigger.addEventListener("keydown", (e) => {
      if (e.key === "ArrowDown" && !this.open) { e.preventDefault(); this.openPanel(); }
    });
    panel.addEventListener("keydown", (e) => this._handlePanelKeydown(e));
  }

  _handlePanelKeydown(e) {
    const options = Array.from(this.dom.list.querySelectorAll(".lang-option"));
    const currentIndex = options.findIndex((el) => el === document.activeElement);

    if (e.key === "Escape") { e.preventDefault(); this.close(); this.dom.trigger.focus(); return; }
    if (e.key === "ArrowDown") { e.preventDefault(); (options[Math.min(options.length - 1, currentIndex + 1)] || options[0]).focus(); return; }
    if (e.key === "ArrowUp") { e.preventDefault(); (options[Math.max(0, currentIndex - 1)] || options[0]).focus(); return; }
    if ((e.key === "Enter" || e.key === " ") && document.activeElement && document.activeElement.classList.contains("lang-option")) {
      e.preventDefault(); document.activeElement.click(); return;
    }
    if (e.key === "Tab") {
      const first = options[0], last = options[options.length - 1];
      if (e.shiftKey && document.activeElement === first) { e.preventDefault(); last.focus(); }
      else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
    }
  }

  openPanel() {
    if (this.open) return;
    this.open = true;
    this.dom.panel.classList.add("is-open");
    this.dom.panel.setAttribute("aria-hidden", "false");
    this.dom.trigger.setAttribute("aria-expanded", "true");

    const selected = this.dom.list.querySelector('.lang-option[aria-selected="true"]') || this.dom.list.querySelector(".lang-option");
    if (selected) selected.focus();

    this._outsideClickHandler = (e) => {
      if (!this.dom.panel.contains(e.target) && !this.dom.trigger.contains(e.target)) this.close();
    };
    setTimeout(() => document.addEventListener("click", this._outsideClickHandler), 0);

    this._keydownHandler = (e) => {
      if (e.key === "Escape") { this.close(); this.dom.trigger.focus(); }
    };
    document.addEventListener("keydown", this._keydownHandler);
  }

  close() {
    if (!this.open) return;
    this.open = false;
    this.dom.panel.classList.remove("is-open");
    this.dom.panel.setAttribute("aria-hidden", "true");
    this.dom.trigger.setAttribute("aria-expanded", "false");
    if (this._outsideClickHandler) { document.removeEventListener("click", this._outsideClickHandler); this._outsideClickHandler = null; }
    if (this._keydownHandler) { document.removeEventListener("keydown", this._keydownHandler); this._keydownHandler = null; }
  }

  toggle() { this.open ? this.close() : this.openPanel(); }
}

/* ---------------- On-demand loading of language files ----------------
   Each language lives in its own file (i18n/<code>.js) that registers
   itself on window.CIDS_I18N. Only the language in use is downloaded. */

window.CIDS_I18N = window.CIDS_I18N || {};

const I18N_BASE_URL = (function () {
  const el = document.currentScript;
  return el && el.src ? el.src.replace(/[^/]*$/, "") : "i18n/";
})();
const _i18nPending = {};

function loadLanguage(code) {
  if (window.CIDS_I18N[code]) return Promise.resolve(window.CIDS_I18N[code]);
  if (_i18nPending[code]) return _i18nPending[code];
  if (!LANGUAGES.some((l) => l.code === code)) return Promise.reject(new Error("Unknown language: " + code));

  _i18nPending[code] = new Promise((resolve, reject) => {
    // The <head> may already have started this file (saved language).
    let el = document.querySelector('script[data-i18n-lang="' + code + '"]');
    if (!el) {
      el = document.createElement("script");
      el.src = I18N_BASE_URL + code + ".js";
      el.setAttribute("data-i18n-lang", code);
      document.head.appendChild(el);
    }
    const settle = () => window.CIDS_I18N[code] ? resolve(window.CIDS_I18N[code])
                                                : reject(new Error("Language file failed: " + code));
    el.addEventListener("load", settle, { once: true });
    el.addEventListener("error", settle, { once: true });
    if (window.CIDS_I18N[code]) settle();
  }).catch((err) => { delete _i18nPending[code]; throw err; });

  return _i18nPending[code];
}
