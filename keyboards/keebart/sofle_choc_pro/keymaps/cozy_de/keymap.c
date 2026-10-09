// The layers of `init_vial/sofle/cozy_de.vil`, compiled in as the defaults Vial starts from.
// Macros, key overrides, the combo and the settings live only in the .vil.

#include QMK_KEYBOARD_H
#include "keymap_german.h"

enum layer_names {
    L_BASE,
    L_COMBINE,
    L_ALTGR,
    L_FN,
};

// de(e1) keys that keymap_german.h does not define
#define DE_DTIL ALGR(DE_I)     // ~ dead tilde
#define DE_DCED ALGR(DE_J)     // ¸ dead cedilla
#define DE_DDIA ALGR(DE_Z)     // ¨ dead diaeresis
#define DE_DSTR ALGR(DE_ADIA)  // / dead stroke
#define DE_LVL5 ALGR(DE_F)     // level-5 latch
#define DE_IEXL ALGR(DE_5)     // ¡
#define DE_IQUE ALGR(DE_6)     // ¿
#define DE_MUL  ALGR(DE_CIRC)  // ×
#define DE_NDSH ALGR(DE_N)     // –

// Vial macro slots, as defined in the .vil
#define MX_EACU QK_MACRO_0  // é
#define MX_EGRV QK_MACRO_1  // è
#define MX_AGRV QK_MACRO_2  // à
#define MX_NTIL QK_MACRO_3  // ñ
#define MX_CCED QK_MACRO_4  // ç
#define MX_HAT  QK_MACRO_5  // ^
#define MX_BTIC QK_MACRO_6  // `
#define MX_CENT QK_MACRO_7  // ¢
#define MX_PND  QK_MACRO_8  // £

#define L2_Y    LT(L_ALTGR, DE_Y)
#define L2_MINS LT(L_ALTGR, DE_MINS)
#define L2_DEL  LT(L_ALTGR, KC_DEL)
#define L2_INS  LT(L_ALTGR, KC_INS)
#define L3_ESC  LT(L_FN, KC_ESC)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L_BASE] = LAYOUT_split_4x6_5(
        L3_ESC , KC_1   , KC_2   , KC_3   , KC_4   , KC_5   ,                       KC_6   , KC_7   , KC_8   , KC_9   , KC_0   , KC_BSPC,
        DE_ADIA, KC_Q   , KC_W   , KC_B   , KC_F   , DE_ODIA,                       DE_Z   , KC_K   , KC_U   , KC_O   , KC_P   , DE_UDIA,
        KC_LSFT, KC_A   , KC_S   , KC_D   , KC_R   , KC_G   ,                       KC_H   , KC_N   , KC_I   , KC_L   , KC_T   , KC_RSFT,
        KC_LCTL, L2_Y   , KC_X   , KC_C   , KC_V   , DE_QUOT, KC_LGUI,     CW_TOGG, KC_J   , KC_M   , DE_COMM, DE_DOT , L2_MINS, KC_ENT ,
                          KC_PGUP, KC_PGDN, KC_LALT, L2_DEL , KC_SPC ,     KC_E   , L2_INS , G(KC_TAB), MO(L_FN), KC_RCTL
    ),
    [L_COMBINE] = LAYOUT_split_4x6_5(
        TO(0)  , KC_NO  , KC_NO  , DE_SECT, DE_DCED, DE_DSTR,                       DE_CIRC, DE_DDIA, DE_ACUT, DE_GRV , DE_DTIL, TO(0)  ,
        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , TO(0)  ,                       KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , MX_AGRV, DE_SS  , KC_NO  , KC_NO  , KC_NO  ,                       KC_NO  , MX_NTIL, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_NO  , MX_CCED, KC_NO  , KC_NO  , KC_NO  ,     KC_NO  , KC_NO  , DE_MICR, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
                          KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,     MX_EACU, MX_EGRV, KC_NO  , KC_NO  , KC_NO
    ),
    [L_ALTGR] = LAYOUT_split_4x6_5(
        KC_NO  , DE_IEXL, MX_CENT, MX_PND , DE_EURO, KC_NO  ,                       MX_HAT , DE_PIPE, DE_LBRC, DE_RBRC, DE_IQUE, KC_DEL ,
        KC_TAB , S(KC_TAB), C(KC_LEFT), KC_UP, C(KC_RGHT), OSL(L_COMBINE),          DE_LVL5, DE_BSLS, DE_LCBR, DE_RCBR, DE_TILD, DE_DEG ,
        KC_LSFT, KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_END ,                       MX_BTIC, DE_SLSH, DE_LPRN, DE_RPRN, DE_SCLN, KC_RSFT,
        KC_LCTL, KC_PGUP, MS_WHLD, MS_WHLU, KC_PGDN, KC_ENT , KC_LGUI,     KC_NO  , DE_MUL , DE_EQL , DE_LABK, DE_RABK, DE_NDSH, KC_INS ,
                          MS_WHLD, MS_WHLU, KC_LALT, KC_TRNS, KC_TRNS,     KC_NO  , KC_TRNS, KC_LGUI, KC_NO  , KC_RCTL
    ),
    [L_FN] = LAYOUT_split_4x6_5(
        KC_NO  , KC_F1  , KC_F2  , KC_F3  , KC_F4  , KC_F5  ,                       KC_F6  , KC_F7  , KC_F8  , KC_F9  , KC_F10 , EE_CLR ,
        KC_NO  , KC_F11 , KC_F12 , G(C(KC_LEFT)), G(C(KC_RGHT)), KC_NO,             RM_TOGG, RM_HUED, RM_SPDD, RM_SATD, RM_VALD, QK_BOOT,
        OS_LSFT, KC_MPRV, G(S(KC_S)), KC_NO, KC_NO , C(KC_E),                       RM_NEXT, RM_HUEU, RM_SPDU, RM_SATU, RM_VALU, OS_RSFT,
        OS_LCTL, QK_BOOT, KC_NO  , KC_NO  , KC_NO  , KC_NO  , OS_LGUI,     OS_RGUI, KC_MSTP, KC_MPLY, KC_VOLD, KC_VOLU, KC_MUTE, KC_NO  ,
                          KC_NO  , KC_NO  , OS_LALT, OS_RALT, KC_NO  ,     KC_NO  , OS_RALT, OS_RCTL, KC_NO  , KC_NO
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [L_BASE] = {ENCODER_CCW_CW(KC_NO, KC_NO), ENCODER_CCW_CW(KC_NO, KC_NO)},
};
#endif
