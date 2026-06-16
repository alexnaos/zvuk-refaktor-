// Custom CSS Injection for Modern Dark Theme
(function() {
  var s = document.createElement('style');
  s.textContent = `
    /* ============================================
       MODERN DARK THEME - Custom Overrides
       ============================================ */

    /* --- Override CSS Variables --- */
    body.theme_light,
    body.theme_dark {
      --accent: #00b4d8;
      --accent_glow: rgba(0, 180, 216, 0.25);
      --accent_soft: rgba(0, 180, 216, 0.1);
      --back: #0f1117;
      --tab: #1a1d27;
      --font: #e8eaed;
      --font_tint: #8b8fa3;
      --font_inv: #ffffff;
      --shadow: rgba(0, 0, 0, 0.4);
      --shadow_light: rgba(255, 255, 255, 0.03);
      --dark: #2a2d3a;
      --error: #ff6b6b;
      --border: rgba(255, 255, 255, 0.06);
      --font_fam: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    }

    /* --- Body & Base --- */
    body {
      background: #080a10 !important;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
    }

    body.theme_light {
      background: #080a10 !important;
    }

    /* --- Main Container --- */
    .main {
      max-width: 480px;
    }

    /* --- Group Cards --- */
    .group_col {
      background: var(--tab) !important;
      border: 1px solid var(--border);
      border-radius: 14px !important;
      box-shadow: 0 2px 16px rgba(0, 0, 0, 0.3), 0 0 1px rgba(255, 255, 255, 0.05) !important;
      margin: 0 8px !important;
      overflow: hidden;
    }

    .group_col > .row,
    .group_col > .widget {
      border-color: var(--border) !important;
    }

    /* --- Group Titles --- */
    .group_title span {
      color: var(--font_tint) !important;
      font-size: 13px !important;
      font-weight: 600 !important;
      letter-spacing: 0.8px;
      text-transform: uppercase;
      padding: 14px 14px 6px !important;
    }

    /* --- Group Rows (cards inside groups) --- */
    .page > .row > .group_row {
      background: transparent !important;
      box-shadow: none !important;
      border-radius: 0 !important;
      margin: 0 !important;
      padding: 10px 14px !important;
    }

    .page > .row > .group_row.line > .widget:not(:first-child) {
      border-left: 1px solid var(--border) !important;
    }

    /* --- Widget Labels --- */
    .widget_row label {
      color: var(--font) !important;
      font-weight: 400;
    }

    /* --- Values --- */
    .value {
      color: var(--font_tint) !important;
    }

    .value.bold {
      color: var(--font) !important;
      font-weight: 600 !important;
    }

    .label_tint {
      color: var(--font_tint) !important;
    }

    /* --- Accent Text (log, active elements) --- */
    .log {
      color: var(--accent) !important;
      background: rgba(0, 180, 216, 0.05) !important;
      border: 1px solid var(--border);
      border-radius: 8px !important;
    }

    /* --- Buttons --- */
    .button {
      background: var(--accent) !important;
      color: var(--font_inv) !important;
      border-radius: 10px !important;
      font-weight: 600 !important;
      letter-spacing: 0.3px;
      box-shadow: 0 2px 8px var(--accent_glow) !important;
      transition: all 0.2s ease !important;
      border: none !important;
    }

    .button:active {
      filter: brightness(0.85) !important;
      transform: scale(0.98);
      box-shadow: 0 1px 4px var(--accent_glow) !important;
    }

    .buttons > .button {
      border-radius: 10px !important;
    }

    .group_col > .buttons:last-child:not(:only-child) > .button:first-child {
      border-bottom-left-radius: 0 !important;
    }

    .group_col > .buttons:last-child:not(:only-child) > .button:last-child {
      border-bottom-right-radius: 0 !important;
    }

    .group_col > .buttons:last-child:not(:only-child) {
      background: transparent !important;
      box-shadow: none !important;
      border-top: 1px solid var(--border) !important;
      gap: 0 !important;
      padding: 0 !important;
    }

    .group_col > .buttons:last-child:not(:only-child) > .button {
      background: transparent !important;
      box-shadow: none !important;
      color: var(--accent) !important;
      border-radius: 0 !important;
    }

    .group_col > .buttons:last-child:not(:only-child) > .button:active {
      background: var(--accent_soft) !important;
    }

    /* --- Sliders --- */
    .slider {
      background: var(--dark) !important;
      background-image: linear-gradient(var(--accent), var(--accent)) !important;
      border-radius: 4px !important;
      height: 5px !important;
    }

    .slider::-webkit-slider-thumb {
      background: var(--accent) !important;
      border: 3px solid var(--tab) !important;
      box-shadow: 0 0 0 2px var(--accent), 0 2px 8px rgba(0, 0, 0, 0.3) !important;
      height: 22px !important;
      width: 22px !important;
      transition: box-shadow 0.2s ease !important;
    }

    .slider::-webkit-slider-thumb:hover {
      box-shadow: 0 0 0 2px var(--accent), 0 0 12px var(--accent_glow) !important;
    }

    .slider::-moz-range-thumb {
      background: var(--accent) !important;
      border: 3px solid var(--tab) !important;
      box-shadow: 0 0 0 2px var(--accent), 0 2px 8px rgba(0, 0, 0, 0.3) !important;
      height: 22px !important;
      width: 22px !important;
    }

    /* --- Switches/Toggles --- */
    .switch {
      background-color: var(--dark) !important;
      border-radius: 16px !important;
      height: 28px !important;
      width: 48px !important;
    }

    .switch:before {
      background: var(--font_tint) !important;
      height: 22px !important;
      width: 22px !important;
      top: 3px !important;
      left: 3px !important;
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3) !important;
    }

    .switch:checked {
      background-color: var(--accent) !important;
      box-shadow: 0 0 8px var(--accent_glow) !important;
    }

    .switch:checked:before {
      background: #fff !important;
      left: 23px !important;
    }

    /* --- Select/Dropdown --- */
    .select {
      background: var(--tab);
      border-radius: 8px;
      border: 1px solid var(--border);
    }

    .option {
      color: var(--font);
      border-bottom: 1px solid var(--border);
    }

    .option.active {
      background: var(--accent) !important;
      color: var(--font_inv) !important;
    }

    .select .title {
      background: transparent !important;
      color: var(--font_tint);
      font-size: 14px;
    }

    /* --- Input Fields --- */
    .input_cont {
      border-bottom: 1px solid var(--border);
    }

    /* --- Page Widget Cards --- */
    .page > .widget {
      background: var(--tab) !important;
      border: 1px solid var(--border) !important;
      border-radius: 12px !important;
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2) !important;
    }

    /* --- Tabs --- */
    .tab {
      border-radius: 8px !important;
      font-weight: 500;
      transition: all 0.2s ease;
    }

    .tab.active {
      background: var(--accent) !important;
      color: var(--font_inv) !important;
      box-shadow: 0 2px 8px var(--accent_glow) !important;
    }

    /* --- Dialog/Popup --- */
    .dialog {
      background: var(--tab) !important;
      border: 1px solid var(--border) !important;
      border-radius: 16px !important;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5) !important;
    }

    .dialog_back {
      backdrop-filter: blur(12px) brightness(0.5) saturate(0.6) !important;
    }

    .dialog textarea {
      background: var(--back) !important;
      border: 1px solid var(--border) !important;
      border-radius: 8px !important;
      color: var(--font) !important;
    }

    /* --- Popups --- */
    .popup {
      background: var(--accent) !important;
      border-radius: 12px !important;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4) !important;
      font-weight: 500;
    }

    /* --- Scrollbar --- */
    body::-webkit-scrollbar-thumb {
      background: var(--font_tint) !important;
      border-radius: 6px !important;
    }

    body::-webkit-scrollbar {
      width: 5px !important;
      height: 5px !important;
    }

    /* --- Log --- */
    .log {
      font-size: 14px !important;
      line-height: 1.5;
    }

    .log .info {
      color: var(--font) !important;
    }

    .log .err {
      color: var(--error) !important;
    }

    .log .warn {
      color: #fbbf24 !important;
    }

    /* --- Gauge --- */
    .gauge {
      border-radius: 10px !important;
      background: linear-gradient(90deg, var(--accent) var(--value), var(--dark) 0) !important;
    }

    .gauge .value {
      color: var(--font) !important;
    }

    /* --- File System --- */
    .fs_row:nth-child(odd) {
      background: var(--shadow_light) !important;
    }

    .fs_info {
      color: var(--font_tint);
      border-top: 1px solid var(--border);
    }

    /* --- Table --- */
    .table th {
      background: var(--accent) !important;
      color: var(--font_inv) !important;
    }

    .table tr:nth-child(odd) {
      background: var(--shadow_light) !important;
    }

    /* --- Navigation Arrows --- */
    .nav > .arrow_cont > .arrow {
      background-color: var(--font_tint);
    }

    /* --- LED --- */
    .led {
      box-shadow: 0 0 6px var(--color);
    }

    /* --- Slider2 (dual range) --- */
    .slider2::-webkit-slider-thumb {
      background: var(--accent) !important;
      border: 3px solid var(--tab) !important;
      box-shadow: 0 0 0 2px var(--accent), 0 2px 8px rgba(0, 0, 0, 0.3) !important;
    }

    .slider2::-moz-range-thumb {
      background: var(--accent) !important;
      border: 3px solid var(--tab) !important;
      box-shadow: 0 0 0 2px var(--accent), 0 2px 8px rgba(0, 0, 0, 0.3) !important;
    }

    /* --- Header --- */
    .header {
      padding: 14px 12px !important;
    }

    /* --- Group Info Widget --- */
    .group_info .widget_label {
      font-size: 15px;
    }

    /* --- Spinner buttons --- */
    .spin_btn {
      color: var(--accent) !important;
      font-weight: 700;
    }

    /* --- Color picker --- */
    .color_out {
      background: var(--tab);
      border: 1px solid var(--border);
      border-radius: 6px;
    }

    /* --- Subtle animation for interactive elements --- */
    .button, .switch, .slider::-webkit-slider-thumb, .tab {
      transition: all 0.2s ease;
    }

    /* --- Menu icons --- */
    .menu_icon {
      border-radius: 10px;
      transition: background 0.2s ease;
    }

    .menu_icon:active {
      background: var(--accent_soft);
    }

    /* --- Hide WS indicator --- */
    .header .ws {
      display: none !important;
    }
  `;
  document.head.appendChild(s);
})();