#include "birth/birth-select-realm.h"
#include "birth/birth-util.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-util.h"
#include <optional>
#include <string>

constexpr auto TOTAL_REALM_NUM = std::ssize(MAGIC_REALM_RANGE) + std::ssize(TECHNIC_REALM_RANGE);

struct birth_realm_type {
    birth_realm_type();
    int cs = 0;
    int n = 0;
    char p2 = ')';
    char sym[TOTAL_REALM_NUM];
    RealmType picks[TOTAL_REALM_NUM];
    std::string cur;
    int k = -1;
    int os = 0;
};

birth_realm_type::birth_realm_type()
{
    for (auto i = 0; i < TOTAL_REALM_NUM; i++) {
        this->sym[i] = '\0';
        this->picks[i] = RealmType::NONE;
    }
}

static void impose_first_realm(PlayerType *player_ptr, RealmChoices &choices)
{
    PlayerRealm pr(player_ptr);
    if (!pr.realm1().is_available()) {
        return;
    }

    if (!PlayerClass(player_ptr).equals(PlayerClassType::PRIEST)) {
        return;
    }

    if (pr.realm1().is_good_attribute()) {
        choices.reset({ RealmType::DEATH, RealmType::DAEMON });
    } else {
        choices.reset({ RealmType::LIFE, RealmType::CRUSADE });
    }
}

static void analyze_realms(PlayerType *player_ptr, RealmType selecting_realm, const RealmChoices &choices, birth_realm_type *birth_realm_ptr)
{
    PlayerRealm pr(player_ptr);
    for (auto realm : EnumRange(RealmType::LIFE, RealmType::MAX)) {
        if (choices.has_not(realm) || pr.realm1().equals(realm)) {
            continue;
        }

        if (realm == selecting_realm) {
            birth_realm_ptr->cs = birth_realm_ptr->n;
        }

        birth_realm_ptr->sym[birth_realm_ptr->n] = I2A(birth_realm_ptr->n);

        const auto buf = format("%c%c %s", birth_realm_ptr->sym[birth_realm_ptr->n], birth_realm_ptr->p2, PlayerRealm::get_name(realm).data());
        put_str(buf, 12 + (birth_realm_ptr->n / 5), 2 + 15 * (birth_realm_ptr->n % 5));
        birth_realm_ptr->picks[birth_realm_ptr->n++] = realm;
    }
}

static void move_birth_realm_cursor(birth_realm_type *birth_realm_ptr)
{
    if (birth_realm_ptr->cs == birth_realm_ptr->os) {
        return;
    }

    c_put_str(TERM_WHITE, birth_realm_ptr->cur, 12 + (birth_realm_ptr->os / 5), 2 + 15 * (birth_realm_ptr->os % 5));

    if (birth_realm_ptr->cs == birth_realm_ptr->n) {
        birth_realm_ptr->cur = format("%c%c %s", '*', birth_realm_ptr->p2, _("ランダム", "Random"));
    } else {
        const auto realm = birth_realm_ptr->picks[birth_realm_ptr->cs];
        const auto &realm_name = PlayerRealm::get_name(realm);
        birth_realm_ptr->cur = format("%c%c %s", birth_realm_ptr->sym[birth_realm_ptr->cs], birth_realm_ptr->p2,
            realm_name.data());
        c_put_str(TERM_L_BLUE, realm_name, 3, 40);
        prt(_("の特徴", ": Characteristic"), 3, 40 + realm_name->length());
        prt(PlayerRealm::get_subinfo(realm), 4, 40);
    }

    c_put_str(TERM_YELLOW, birth_realm_ptr->cur, 12 + (birth_realm_ptr->cs / 5), 2 + 15 * (birth_realm_ptr->cs % 5));
    birth_realm_ptr->os = birth_realm_ptr->cs;
}

static void interpret_realm_select_key(birth_realm_type *birth_realm_ptr, char c)
{
    if (c == 'Q') {
        birth_quit();
    }

    if (c == '8') {
        if (birth_realm_ptr->cs >= 5) {
            birth_realm_ptr->cs -= 5;
        }
    }

    if (c == '4') {
        if (birth_realm_ptr->cs > 0) {
            birth_realm_ptr->cs--;
        }
    }

    if (c == '6') {
        if (birth_realm_ptr->cs < birth_realm_ptr->n) {
            birth_realm_ptr->cs++;
        }
    }

    if (c == '2') {
        if ((birth_realm_ptr->cs + 5) <= birth_realm_ptr->n) {
            birth_realm_ptr->cs += 5;
        }
    }
}

static bool get_a_realm(PlayerType *player_ptr, birth_realm_type *birth_realm_ptr)
{
    birth_realm_ptr->os = birth_realm_ptr->n;
    while (true) {
        move_birth_realm_cursor(birth_realm_ptr);
        if (birth_realm_ptr->k >= 0) {
            break;
        }

        const auto buf = format(_("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "),
            birth_realm_ptr->sym[0], birth_realm_ptr->sym[birth_realm_ptr->n - 1]);

        put_str(buf, 10, 10);
        char c = inkey();
        interpret_realm_select_key(birth_realm_ptr, c);
        if (c == 'S') {
            return true;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (birth_realm_ptr->cs == birth_realm_ptr->n) {
                birth_realm_ptr->k = randint0(birth_realm_ptr->n);
                break;
            } else {
                birth_realm_ptr->k = birth_realm_ptr->cs;
                break;
            }
        }

        if (c == '*') {
            birth_realm_ptr->k = randint0(birth_realm_ptr->n);
            break;
        }

        birth_realm_ptr->k = (islower(c) ? A2I(c) : -1);
        if ((birth_realm_ptr->k >= 0) && (birth_realm_ptr->k < birth_realm_ptr->n)) {
            birth_realm_ptr->cs = birth_realm_ptr->k;
            continue;
        }

        birth_realm_ptr->k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((birth_realm_ptr->k >= 26) && (birth_realm_ptr->k < birth_realm_ptr->n)) {
            birth_realm_ptr->cs = birth_realm_ptr->k;
            continue;
        } else {
            birth_realm_ptr->k = -1;
        }

        birth_help_option(player_ptr, c, BirthKind::REALM);
    }

    return false;
}

/*!
 * @brief プレイヤーの魔法領域を選択する / Choose from one of the available magical realms
 * @param selecting_realm 現在選択中の魔法領域
 * @param choices 選択可能な魔法領域のビット配列
 * @return 選択した魔法領域のID
 * @details 領域数が0 (戦士等)or 1 (観光客等)なら自動での値を返す
 */
static std::optional<RealmType> select_realm(PlayerType *player_ptr, RealmType selecting_realm, RealmChoices choices)
{
    clear_from(10);
    if (choices.count() <= 1) {
        return choices.first().value_or(RealmType::NONE);
    }

    impose_first_realm(player_ptr, choices);
    put_str(_("注意：魔法の領域の選択によりあなたが習得する呪文のタイプが決まります。", "Note: The realm of magic will determine which spells you can learn."),
        23, 5);

    birth_realm_type birth_realm;
    analyze_realms(player_ptr, selecting_realm, choices, &birth_realm);
    birth_realm.cur = format("%c%c %s", '*', birth_realm.p2, _("ランダム", "Random"));
    if (get_a_realm(player_ptr, &birth_realm)) {
        return std::nullopt;
    }

    clear_from(10);
    return birth_realm.picks[birth_realm.k];
}

static void cleanup_realm_selection_window(void)
{
    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);
}

/*!
 * @brief 選んだ魔法領域で本当に良いか問い合わせる
 * @param count 魔法領域の数
 * @return 選んだ魔法領域で良ければTRUE、再選択ならばFALSE
 */
static bool check_realm_selection(PlayerType *player_ptr, int count)
{
    if (count < 2) {
        prt(_("何かキーを押してください", "Hit any key."), 0, 0);
        (void)inkey();
        prt("", 0, 0);
        return true;
    } else if (input_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
        return true;
    }

    return false;
}

static std::optional<RealmType> process_choose_realm(PlayerType *player_ptr, RealmChoices choices)
{
    auto selecting_realm = RealmType::NONE;
    while (true) {
        const auto selected_realm = select_realm(player_ptr, selecting_realm, choices);
        if (!selected_realm || *selected_realm == RealmType::NONE) {
            return selected_realm;
        }

        cleanup_realm_selection_window();
        display_wrap_around(PlayerRealm::get_explanation(*selected_realm), 74, 12, 3);

        if (check_realm_selection(player_ptr, choices.count())) {
            return selected_realm;
        }
        selecting_realm = *selected_realm;
    }
}

static void print_choosed_realms(PlayerType *player_ptr)
{
    put_str(_("魔法        :", "Magic       :"), 6, 1);

    PlayerRealm pr(player_ptr);
    std::string choosed_realms;
    if (pr.realm2().is_available()) {
        choosed_realms = format("%s, %s", pr.realm1().get_name().data(), pr.realm2().get_name().data());
    } else {
        choosed_realms = pr.realm1().get_name();
    }

    c_put_str(TERM_L_BLUE, choosed_realms, 6, 15);
}

/*!
 * @brief 選択した魔法領域の解説を表示する / Choose the magical realms
 * @return ユーザが魔法領域の確定を選んだらTRUEを返す。
 */
bool get_player_realms(PlayerType *player_ptr)
{
    /* Clean up infomation of modifications */
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);
    put_str("                                   ", 6, 40);

    PlayerRealm pr(player_ptr);
    pr.reset();

    if (PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        const auto realm = select_element_realm(player_ptr);
        if (!realm) {
            return false;
        }
        player_ptr->element_realm = *realm;

        put_str(_("魔法        :", "Magic       :"), 6, 1);
        c_put_str(TERM_L_BLUE, get_element_title(player_ptr->element_realm), 6, 15);
        return true;
    }

    /* Select the first realm */
    const auto realm1 = process_choose_realm(player_ptr, PlayerRealm::get_realm1_choices(player_ptr->pclass));
    if (!realm1) {
        return false;
    }
    if (*realm1 == RealmType::NONE) {
        return true;
    }
    pr.set(*realm1);
    print_choosed_realms(player_ptr);

    /* Select the second realm */
    const auto realm2 = process_choose_realm(player_ptr, PlayerRealm::get_realm2_choices(player_ptr->pclass));
    if (!realm2) {
        return false;
    }
    if (*realm2 == RealmType::NONE) {
        return true;
    }
    pr.set(*realm1, *realm2);
    print_choosed_realms(player_ptr);

    return true;
}
