class _ {
    static css = `
/* ============================================
   MODERN DARK THEME - CSS Overrides
   ============================================ */

body.theme_light,
body.theme_dark {
  /* === Основные цвета === */
  --accent: #0559c7d0;
  --back: #080a10;
  --tab: #1a1d27;
  --font: #e8eaed;
  --font_tint: #8b8fa3;
  --font_inv: #ffffff;
  --shadow: rgba(0, 0, 0, 0.4);
  --shadow_light: rgba(255, 255, 255, 0.03);
  --dark: #2a2d3a;
  --error: #ff6b6b;

  /* === Шрифты === */
  --font_fam: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
  --font_size_base: 14px;
  --font_weight_label: 400;
  --font_weight_bold: 600;

  /* === Кнопки === */
  --btn_bg: #00b4d8;
  --btn_color: #ffffff;
  --btn_shadow: rgba(0, 180, 216, 0.25);
  --btn_radius: 10px;

  /* === Границы === */
  --border_color: rgba(255, 255, 255, 0.06);
  --border_radius: 14px;
  --border_radius_sm: 12px;
  --border_radius_xs: 8px;

  /* === Скроллбар === */
  --scrollbar_width: 5px;
  --scrollbar_radius: 6px;
}

body {
  background: var(--back) !important;
}

.main { max-width: 480px; }

.group_col {
  background: var(--tab) !important;
  border: 1px solid var(--border_color);
  border-radius: var(--border_radius) !important;
  box-shadow: 0 2px 16px var(--shadow) !important;
  margin: 0 8px !important;
  overflow: hidden;
}

.group_col > .row,
.group_col > .widget {
  border-color: var(--border_color) !important;
}

.group_title span {
  color: var(--font_tint) !important;
  font-size: 13px !important;
  font-weight: var(--font_weight_bold) !important;
  letter-spacing: 0.8px;
  text-transform: uppercase;
  padding: 14px 14px 6px !important;
}

.page > .row > .group_row {
  background: transparent !important;
  box-shadow: none !important;
  border-radius: 0 !important;
  margin: 0 !important;
  padding: 10px 14px !important;
}

.widget_row label {
  color: var(--font) !important;
  font-weight: var(--font_weight_label);
}

.value {
  color: var(--font_tint) !important;
}

.value.bold {
  color: var(--font) !important;
  font-weight: var(--font_weight_bold) !important;
}

.button {
  background: var(--btn_bg) !important;
  color: var(--btn_color) !important;
  border-radius: var(--btn_radius) !important;
  font-weight: var(--font_weight_bold) !important;
  box-shadow: 0 2px 8px var(--btn_shadow) !important;
  border: none !important;
}

.button:active {
  filter: brightness(0.85) !important;
  transform: scale(0.98);
}

.buttons > .button {
  border-radius: var(--btn_radius) !important;
}

.group_col > .buttons:last-child:not(:only-child) {
  background: transparent !important;
  box-shadow: none !important;
  border-top: 1px solid var(--border_color) !important;
  gap: 0 !important;
  padding: 0 !important;
}

.group_col > .buttons:last-child:not(:only-child) > .button {
  background: transparent !important;
  box-shadow: none !important;
  color: var(--btn_bg) !important;
  border-radius: 0 !important;
}

.slider {
  background: var(--dark) !important;
  background-image: linear-gradient(var(--accent), var(--accent)) !important;
  border-radius: 4px !important;
  height: 5px !important;
}

.slider::-webkit-slider-thumb {
  background: var(--accent) !important;
  border: 3px solid var(--tab) !important;
  box-shadow: 0 0 0 2px var(--accent), 0 2px 8px var(--shadow) !important;
  height: 22px !important;
  width: 22px !important;
}

.switch {
  background-color: var(--dark) !important;
  border-radius: 16px !important;
  height: 28px !important;
  width: 48px !important;
}

.switch::before {
  background: var(--font_tint) !important;
  height: 22px !important;
  width: 22px !important;
  top: 3px !important;
  left: 3px !important;
}

.switch:checked {
  background-color: var(--accent) !important;
}

.switch:checked::before {
  background: var(--font_inv) !important;
  left: 23px !important;
}

.select {
  background: var(--tab);
  border-radius: var(--border_radius_xs);
  border: 1px solid var(--border_color);
}

.page > .widget {
  background: var(--tab) !important;
  border: 1px solid var(--border_color) !important;
  border-radius: var(--border_radius_sm) !important;
}

.tab {
  border-radius: var(--border_radius_xs) !important;
  font-weight: 500;
}

.tab.active {
  background: var(--accent) !important;
  color: var(--font_inv) !important;
}

.dialog {
  background: var(--tab) !important;
  border: 1px solid var(--border_color) !important;
  border-radius: 16px !important;
}

.dialog_back {
  backdrop-filter: blur(12px) brightness(0.5) !important;
}

.popup {
  background: var(--accent) !important;
  border-radius: var(--border_radius_sm) !important;
  font-weight: 500;
}

body::-webkit-scrollbar {
  width: var(--scrollbar_width) !important;
  height: var(--scrollbar_width) !important;
}

body::-webkit-scrollbar-thumb {
  background: var(--font_tint) !important;
  border-radius: var(--scrollbar_radius) !important;
}

.log {
  color: var(--accent) !important;
  background: color-mix(in srgb, var(--btn_bg) 5%, transparent) !important;
  border: 1px solid var(--border_color);
  border-radius: var(--border_radius_xs) !important;
  font-size: var(--font_size_base) !important;
}

.header .ws {
  display: none !important;
}
`;
}