#include "window/display-sub-window-items.h"
#include "flavor/flavor-util.h"
#include "game-option/text-display-options.h"
#include "object-enchant/special-object-flags.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/quarks.h"
#include "util/string-processor.h"

void display_short_flavors(flavor_type *flavor_ptr)
{
    flavor_ptr->tmp_val2[0] = '\0';
    if ((abbrev_extra || abbrev_all) && flavor_ptr->o_ptr->is_fully_known()) {
        if (!flavor_ptr->o_ptr->inscription || !angband_strchr(quark_str(flavor_ptr->o_ptr->inscription), '%')) {
            bool kanji = _(true, false);
            get_ability_abbreviation(flavor_ptr->tmp_val2, flavor_ptr->o_ptr, kanji, abbrev_all);
        }
    }

    if (flavor_ptr->o_ptr->inscription == 0) {
        return;
    }

    char buff[1024] = "";
    if (flavor_ptr->tmp_val2[0]) {
        strcat(flavor_ptr->tmp_val2, ", ");
    }

    get_inscription(buff, flavor_ptr->o_ptr);
    angband_strcat(flavor_ptr->tmp_val2, buff, sizeof(flavor_ptr->tmp_val2));
}

void display_item_discount(flavor_type *flavor_ptr)
{
    if ((flavor_ptr->o_ptr->discount == 0) || (flavor_ptr->tmp_val2[0] && ((flavor_ptr->o_ptr->ident & IDENT_STORE) == 0))) {
        return;
    }

    char discount_num_buf[4];
    if (flavor_ptr->fake_insc_buf[0]) {
        strcat(flavor_ptr->fake_insc_buf, ", ");
    }

    (void)object_desc_num(discount_num_buf, flavor_ptr->o_ptr->discount);
    strcat(flavor_ptr->fake_insc_buf, discount_num_buf);
    strcat(flavor_ptr->fake_insc_buf, _("%引き", "% off"));
}

/*!
 * @brief 呪いの有無や割引率等を表示する
 * @param flavor_ptr アイテム表記への参照ポインタ
 */
void display_item_fake_inscription(flavor_type *flavor_ptr)
{
    if ((flavor_ptr->fake_insc_buf[0] == '\0') && (flavor_ptr->tmp_val2[0] == '\0')) {
        return;
    }

    flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
    flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->c1);
    if (flavor_ptr->fake_insc_buf[0]) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, flavor_ptr->fake_insc_buf);
    }

    if ((flavor_ptr->fake_insc_buf[0] != '\0') && (flavor_ptr->tmp_val2[0] != '\0')) {
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ',');
        flavor_ptr->t = object_desc_chr(flavor_ptr->t, ' ');
    }

    if (flavor_ptr->tmp_val2[0]) {
        flavor_ptr->t = object_desc_str(flavor_ptr->t, flavor_ptr->tmp_val2);
    }

    flavor_ptr->t = object_desc_chr(flavor_ptr->t, flavor_ptr->c2);
}
