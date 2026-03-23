#include "ui_internal.h"

void _settings_open_cb(void *data, Evas_Object *obj, void *event_info);

void
_settings_save_cb(void *data, Evas_Object *obj, void *event_info);

void
_right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *ev = event_info;
    if (ev->button != 3) return;

    Player_State *ps = data;

    Evas_Object *menu = elm_menu_add(obj);
    elm_menu_item_add(menu, NULL, NULL, "Settings…", _settings_open_cb, ps);
    elm_menu_move(menu, ev->canvas.x, ev->canvas.y);
    evas_object_show(menu);
}

void
_settings_open_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    Evas_Object *popup = elm_popup_add(obj);
    elm_object_part_text_set(popup, "title,text", "Settings");

    evas_object_data_set(popup, "player_state", ps);

    Evas_Object *box = elm_box_add(popup);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_horizontal_set(box, EINA_FALSE);
    evas_object_show(box);

    Evas_Object *label = elm_label_add(box);
    elm_object_text_set(label, "Music directory:");
    evas_object_show(label);
    elm_box_pack_end(box, label);

    Evas_Object *entry = elm_entry_add(box);
    elm_entry_single_line_set(entry, EINA_TRUE);
    elm_object_text_set(entry, ps->settings->music_folder);
    evas_object_show(entry);
    elm_box_pack_end(box, entry);

    elm_object_content_set(popup, box);

    elm_popup_item_append(popup, "OK", NULL, _settings_save_cb, entry);
    elm_popup_item_append(popup, "Cancel", NULL, NULL, NULL);

    evas_object_show(popup);
}

void
_settings_save_cb(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *entry = data;
    Player_State *ps = evas_object_data_get(obj, "player_state");

    const char *newdir = elm_object_text_get(entry);
    if (newdir && newdir[0]) {
        free(ps->settings->music_folder);
        ps->settings->music_folder = strdup(newdir);
    }

    evas_object_del(obj);
}
